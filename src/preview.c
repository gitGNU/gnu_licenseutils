/*  Copyright (C) 2013 Ben Asselstine

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/
#include <config.h>
#include <stdlib.h>
#include <unistd.h>
#include <argz.h>
#include <glib.h>
#include "licensing_priv.h"
#include "preview.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "trim.h"
#include "comment.h"
#include "styles.h"

enum {
  OPT_NO_STYLE=-1011,
};

static struct argp_option argp_options[] = 
{
    {"no-commenting-style", OPT_NO_STYLE, NULL, 0, 
      N_("show the boilerplate without comment delimiters")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_preview_options_t *opt = NULL;
  if (state)
    opt = (struct lu_preview_options_t*) state->input;
  switch (key)
    {
    case OPT_NO_STYLE:
      opt->no_style = 1;
      break;
    case ARGP_KEY_INIT:
      opt->no_style = 0;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef PREVIEW_DOC
#define PREVIEW_DOC N_("Show the current working boilerplate.")
static struct argp argp = { argp_options, parse_opt, "", PREVIEW_DOC};

int 
lu_preview_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_preview_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_preview (state, &opts);
  else
    return err;
}

static int
dump_file (char *file, FILE *out)
{
  FILE *fp = fopen (file, "r");
  if (!fp)
    return -1;
  size_t data_len = 0;
  char *data = fread_file (fp, &data_len);
  if (data)
    {
      fprintf (out, "%s", data);
      fflush (out);
      free (data);
    }
  fclose (fp);
  return 0;
}

static int
format_and_dump_file (char *file, char *fmt, FILE *out)
{
  FILE *fp = fopen (file, "r");
  if (!fp)
    return -1;
  fclose (fp);
  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp(tmp);
  close (fd);
  char *cmd = xasprintf ("cat %s | %s > %s", file, fmt, tmp);
  if (cmd)
    {
      system (cmd);
      free (cmd);
      dump_file (tmp, out);
      remove (tmp);
    }
  return 0;
}

static int
trim_and_dump_file (char *f, FILE *out)
{
  int err = 0;
  FILE *fp = fopen (f, "r");
  if (fp)
    {
      char *line = NULL;
      size_t len = 0;
      ssize_t read;

      while ((read = getline(&line, &len, fp)) != -1)
        {
          char *trimmed = trim_leading (line);
          if (trimmed)
            {
              if (strlen (trimmed) > 0)
                fprintf (out, "%s", trimmed);
              else
                fprintf (out, "\n");
              free (trimmed);
            }
        }
      free (line);
      fclose (fp);
    }
  else
    err = -1;
  return err;
}

static int 
generate_uncommented_boilerplate (struct lu_state_t *state, struct lu_preview_options_t *options, char *fmt, FILE *out)
{
  int err;
  char *f = get_config_file ("top-line");
  if (f)
    {
      err = format_and_dump_file (f, fmt, out);
      free (f);
      if (!err)
        fprintf (out, "\n");
    }
  f = get_config_file ("copyright-holders");
  if (f)
    {
      err = dump_file (f, out);
      free (f);
      if (!err)
        fprintf (out, "\n");
    }
  f = get_config_file ("project-line");
  if (f)
    {
      err = format_and_dump_file (f, fmt, out);
      free (f);
      if (!err)
        fprintf (out, "\n");
    }
  f = get_config_file ("license-notice");
  if (f)
    {
      err = trim_and_dump_file (f, out);
      free (f);
    }
  return 0;
}

int
generate_boilerplate (struct lu_state_t *state, struct lu_preview_options_t *options, FILE *out)
{
  int err = 0;
  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp(tmp);
  close (fd);
  FILE *fp = fopen (tmp, "w");
  if (!fp)
    {
      remove (tmp);
      return -1;
    }

  char *fmt = g_find_program_in_path ("fmt");
  if (!fmt)
    {
      remove (tmp);
      return -2;
    }
  err = generate_uncommented_boilerplate (state, options, fmt, fp);
  free (fmt);
  fclose (fp);
  if (err)
    {
      remove (tmp);
      return -3;
    }
  struct lu_comment_style_t * style = lu_get_current_commenting_style();
  if (options->no_style || !style)
    {
      FILE *uncommented_boilerplate = fopen (tmp, "r");
      if (uncommented_boilerplate)
        {
          size_t data_len = 0;
          char *data = fread_file (uncommented_boilerplate, &data_len);
          if (data)
            {
              fprintf (out, "%s", data);
              fprintf (out, "\n");
              free (data);
            }
          fclose (uncommented_boilerplate);
        }
    }
  else
    {
      FILE *uncommented_boilerplate = fopen (tmp, "r");
      if (uncommented_boilerplate)
        {
          size_t data_len = 0;
          char *data = fread_file (uncommented_boilerplate, &data_len);
          if (data)
            {
              struct lu_comment_options_t comment_options;
              memset (&comment_options, 0, sizeof (comment_options));
              argz_add (&comment_options.input_files,
                        &comment_options.input_files_len, tmp);
              comment_options.style = style;
              char *commented_boilerplate = 
                lu_create_comment (state, &comment_options, data);
              free (data);
              if (commented_boilerplate)
                {
                  fprintf (out, "%s", commented_boilerplate);
                  free (commented_boilerplate);
                }
            }
          fclose (uncommented_boilerplate);
        }
    }
  remove (tmp);
  return err;
}

int 
lu_preview (struct lu_state_t *state, struct lu_preview_options_t *options)
{
  int err = generate_boilerplate (state, options, stdout);
  return err;
}

struct lu_command_t preview = 
{
  .name         = N_("preview"),
  .doc          = PREVIEW_DOC,
  .flags        = SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_preview_parse_argp
};
