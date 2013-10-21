/*  Copyright (C) 2013 Ben Asselstine

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
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
#include <argz.h>
#include "licensing_priv.h"
#include "cbb.h"
#include "gettext-more.h"
#include "util.h"
#include "styles.h"

static struct argp_option argp_options[] = 
{
    {"lines", 'l', NULL, 0, 
      N_("count the number of lines in the boilerplate")},
    {"blocks", 'b', NULL, 0, 
      N_("count the number of sections in the boilerplate")},
    { 0 }
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_cbb_options_t *opt = NULL;
  if (state)
    opt = (struct lu_cbb_options_t*) state->input;
  switch (key)
    {
    case 'l':
      opt->lines = 1;
      break;
    case 'b':
      opt->blocks = 1;
      break;
    case ARGP_KEY_ARG:
      argz_add (&opt->input_files, &opt->input_files_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->style = NULL;
      state->child_inputs[0] = &opt->style;
      opt->input_files = NULL;
      opt->input_files_len = 0;
      opt->blocks = 0;
      opt->lines = 0;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp_child parsers[]=
{
    { &styles_argp, 0, "Comment Style Options:", 0 },
    { 0 }
};

#undef CBB_DOC
#define CBB_DOC N_("Count boilerplate blocks in a file.") "\v"\
  N_("Comment style is auto-detected if a style option is not provided.") "  "\
  N_("With no FILE, or when FILE is -, read from standard input.")
static struct argp argp = { argp_options, parse_opt, "[FILE...]", CBB_DOC, 
  parsers};

int 
lu_cbb_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_cbb_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_cbb (state, &opts);
  else
    return err;
}

static int 
count_lines (char *comment_blocks, size_t len)
{
  int lines = 0;
  char *c = NULL;
  int end_in_newline = 0;
  while ((c = argz_next (comment_blocks, len, c)))
    {
      char *text = c;
      char *nl;
      while ((nl = strchr (text, '\n')))
        {
          text = ++nl;
          lines++;
        }
      if (c[strlen (c)-1] == '\n')
        end_in_newline = 1;
      else
        end_in_newline = 0;
    }
  return lines + !end_in_newline;
}

static void 
show_results (struct lu_state_t *state, struct lu_cbb_options_t *options, char *comment_blocks, size_t len, char *file)
{
  int show_all = 0;
  if (options->lines == 0 && options->blocks == 0)
    show_all = 1;
  if (options->blocks || show_all)
    {
      if (comment_blocks)
        luprintf (state, "%d", argz_count (comment_blocks, len));
      else
        luprintf (state, "0");
    }

  if (options->lines || show_all)
    {
      if (options->blocks || show_all)
        luprintf (state, " ");
      if (comment_blocks)
        luprintf (state, "%d", count_lines (comment_blocks, len));
      else
        luprintf (state, "0");
    }

  luprintf (state, "%s%s\n", file ? " ": "", file ? file : "");
}

static int
count_boilerplate_blocks (struct lu_state_t *state, struct lu_cbb_options_t *options, char *file, int showfile)
{
  char *comment_blocks = NULL;
  size_t len = 0;
  if (is_a_file (file) == 0)
    {
      if (errno == EISDIR)
        fprintf (stderr, N_("%s: %s: %s\n"),
                 cbb.name, file, strerror (errno));
      else
        fprintf (stderr, N_("%s: could not open `%s' for reading: %s\n"),
                 cbb.name, file, strerror (errno));
      return 0;
    }
  FILE *fp = fopen (file, "r");
  if (!fp)
    return 0;
  if (options->style == NULL)
    auto_detect_comment_blocks (file, fp, &comment_blocks, &len, NULL);
  else
    options->style->get_initial_comment (fp, &comment_blocks, &len, NULL);
  fclose (fp);
  show_results (state, options, comment_blocks, len, showfile ? file: NULL);
  return 0;
}

static int 
lu_cbb_from_stdin (struct lu_state_t *state, struct lu_cbb_options_t *options)
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
      err = count_boilerplate_blocks (state, options, tmp, 0);
      remove (tmp);
    }
  else
    err = -1;
  return err;
}

int 
lu_cbb (struct lu_state_t *state, struct lu_cbb_options_t *options)
{
  int err = 0;
  if (options->input_files == NULL)
    err = lu_cbb_from_stdin (state, options);
  else
    {
      char *f = NULL;
      while ((f = argz_next (options->input_files, options->input_files_len, f)))
        {
          if (strcmp (f, "-") == 0)
            err = lu_cbb_from_stdin (state, options);
          else
            err = count_boilerplate_blocks (state, options, f, 1);
          if (err)
            break;
        }
    }
  err = 0;
  return err;
}

struct lu_command_t cbb = 
{
  .name         = N_("cbb"),
  .doc          = CBB_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_cbb_parse_argp
};
