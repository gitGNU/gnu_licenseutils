/*  Copyright (C) 2013 Ben Asselstine

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied copyright of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/
#include <config.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <argz.h>
#include "licensing_priv.h"
#include "uncomment.h"
#include "gettext-more.h"
#include "read-file.h"
#include "util.h"
#include "styles.h"

enum
{
  OPT_WHITESPACE = -611,
};

static struct argp_option argp_options[] = 
{
    {"trim", OPT_WHITESPACE, NULL, 0, N_("remove leading and trailing whitespace on lines")},
    {0}
};
static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_uncomment_options_t *opt = NULL;
  if (state)
    opt = (struct lu_uncomment_options_t*) state->input;
  switch (key)
    {
    case OPT_WHITESPACE:
      opt->trim = 1;
      break;
    case ARGP_KEY_ARG:
      argz_add (&opt->input_files, &opt->input_files_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->input_files = NULL;
      opt->input_files_len = 0;
      opt->trim = 0;
      opt->style = NULL;
      state->child_inputs[0] = &opt->style;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp_child parsers[]=
{
    { &styles_argp, 0, N_("Commenting Style Options:"), 0 },
    { 0 }
};

#undef UNCOMMENT_DOC
#define UNCOMMENT_DOC \
  N_("Remove comment delimiters but leave the comment text.") "\v"\
  N_("Comment style is auto-detected if a style option is not provided.") "  "\
  N_("Files are not modified, the uncommented files are shown on the standard output.") "  "\
  N_("Only comment blocks at the start of the file are uncommented.") "  "\
  N_("With no FILE, or when FILE is -, read from standard input.")
static struct argp argp = { argp_options, parse_opt, "[FILE...]", 
  UNCOMMENT_DOC, parsers};

int 
lu_uncomment_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_uncomment_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_uncomment (state, &opts);
  else
    return err;
}

static int
lu_uncomment_file (struct lu_state_t *state, struct lu_uncomment_options_t *opts, char *file)
{
  char *comment_blocks = NULL;
  size_t len = 0;
  FILE *fp = fopen (file, "r");
  if (!fp)
    return 0;
  if (is_a_file (file) == 0)
    {
      if (errno == EISDIR)
        fprintf (stderr, N_("%s: %s: %s\n"),
                 uncomment.name, file, strerror (errno));
      else
        fprintf (stderr, N_("%s: could not open `%s' for reading: %s\n"),
                 uncomment.name, file, strerror (errno));
      return 0;
    }
  if (opts->style == NULL)
    opts->style = auto_detect_comment_blocks (file, fp, &comment_blocks, &len, NULL);
  else
    opts->style->get_initial_comment (fp, &comment_blocks, &len, NULL);

  if (opts->style)
    opts->style->uncomment (&comment_blocks, &len, opts->trim);

  if (comment_blocks)
    {
      char *c = NULL;
      while ((c = argz_next (comment_blocks, len, c)))
        luprintf (state, "%s\n", c);
      free (comment_blocks);
    }
  if (!feof (fp))
    {
      char *line = NULL;
      size_t linelen = 0;
      ssize_t read = getline (&line, &linelen, fp);
      if (read != -1 && strspn (line, "\n\r\t\v ") != strlen (line))
        luprintf (state, "%s", line);
      if (read != -1)
        while ((read = getline (&line, &linelen, fp)) != -1) 
          luprintf (state , "%s", line);
      free (line);
    }
  fclose (fp);

  return 0;
}

static int
lu_uncomment_from_stdin (struct lu_state_t *state, struct lu_uncomment_options_t *options)
{
  int err = 0;
  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp(tmp);
  close (fd);
  FILE *fileptr = fopen (tmp, "w");
  if (fileptr)
    {
      char *line = NULL;
      size_t len = 0;
      ssize_t read;

      while ((read = getline(&line, &len, stdin)) != -1)
        fprintf (fileptr, "%s", line);

      free (line);
      fflush (fileptr);
      fsync (fileno (fileptr));
      fclose (fileptr);
      err = lu_uncomment_file (state, options, tmp);
      remove (tmp);
    }
  else
    err = -1;
  return err;
}

static int
lu_uncomment_files (struct lu_state_t *state, struct lu_uncomment_options_t *options)
{
  int err = 0;
  char *f = NULL;
  while ((f = argz_next (options->input_files, options->input_files_len, f)))
    {
      if (strcmp (f, "-") == 0)
        lu_uncomment_from_stdin (state, options);
      else
        err = lu_uncomment_file (state, options, f);
      if (err)
        break;
    }
  return err;
}

int 
lu_uncomment (struct lu_state_t *state, struct lu_uncomment_options_t *options)
{
  int err = 0;
  if (options->input_files == NULL)
    err = lu_uncomment_from_stdin (state, options);
  else
    err = lu_uncomment_files (state, options);
  free (options->input_files);
  return err;
}

struct lu_command_t uncomment = 
{
  .name         = N_("uncomment"),
  .doc          = UNCOMMENT_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_uncomment_parse_argp
};
