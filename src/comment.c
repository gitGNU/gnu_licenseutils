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
#include "comment.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "error.h"
#include "util.h"

#include "styles.h"

static error_t 
parse_arg (int key, char *arg, struct argp_state *state)
{
  struct lu_comment_options_t *opt = NULL;
  if (state)
    opt = (struct lu_comment_options_t*) state->input;
  switch (key)
    {
    case ARGP_KEY_ARG:
      argz_add (&opt->input_files, &opt->input_files_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->input_files = NULL;
      opt->input_files_len = 0;
      opt->style = NULL;
      state->child_inputs[0] = &opt->style;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static const struct argp_child
parsers[] =
{
    { &styles_argp },
    { 0 },
};

#undef COMMENT_DOC
#define COMMENT_DOC \
  N_("Create a comment block out of some arbitrary text.") "\v"\
  N_("By default, comments are created in the C style.") "  "\
  N_("With no FILE, or when FILE is -, read from standard input.")
static struct argp argp = { NULL, parse_arg, "[FILE...]", COMMENT_DOC, parsers};

int 
lu_comment_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_comment_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_comment (state, &opts);
  else
    return err;
}

char *
lu_create_comment (struct lu_state_t *state, struct lu_comment_options_t *options, char *text)
{
  if (options->style)
    return options->style->comment (text);

  struct lu_comment_style_t *default_style = get_default_commenting_style();
  if (default_style)
    return default_style->comment(text);

  return strdup (text);
}

static int
create_comment_from_file (struct lu_state_t *state, struct lu_comment_options_t *options, char *filename)
{
  FILE *fp = fopen (filename, "r");
  if (!fp)
    return -1;
  size_t data_len = 0;
  char *data = fread_file (fp, &data_len);
  char *comment = lu_create_comment (state, options, data);
  luprintf (state, "%s", comment);
  free (comment);
  free (data);
  fclose (fp);
  return 0;
}

static int
lu_comment_from_stdin (struct lu_state_t *state, struct lu_comment_options_t*options)
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
      err = create_comment_from_file (state, options, tmp);
      remove (tmp);
    }
  else
    err = -1;
  return err;
}

static int
create_comment_from_files (struct lu_state_t *state, struct lu_comment_options_t *options)
{
  int err = 0;
  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp(tmp);
  close (fd);
  FILE *out = fopen (tmp, "w");
  char *f = NULL;
  while ((f = argz_next (options->input_files, options->input_files_len, f)))
    {
      FILE *fp;
      if (is_a_file (f) == 0 && strcmp (f, "-") != 0)
        {
          if (errno == EISDIR)
            error (0, errno, "%s", f);
          else
            error (0, errno, N_("could not open `%s' for reading"), f);
          continue;
        }
      if (strcmp (f, "-") == 0)
        fp = stdin;
      else
        fp = fopen (f, "r");
      if (fp)
        {
          size_t data_len = 0;
          char *data = fread_file (fp, &data_len);
          fprintf (out, "%s", data);
          free (data);
          if (fp != stdin)
            fclose (fp);
        }
    }
  fclose (out);
  err = create_comment_from_file (state, options, tmp);
  remove (tmp);
  return err;
}

int 
lu_comment (struct lu_state_t *state, struct lu_comment_options_t *options)
{
  int err = 0;
  if (options->input_files == NULL)
    lu_comment_from_stdin (state, options);
  else
    err = create_comment_from_files(state, options);
  free (options->input_files);
  return err;
}

struct lu_command_t comment = 
{
  .name         = N_("comment"),
  .doc          = COMMENT_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_comment_parse_argp
};

