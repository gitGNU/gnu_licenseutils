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
#include "licensing_priv.h"
#include "extra.h"
#include "gettext-more.h"
#include "read-file.h"

static struct argp_option argp_options[] = 
{
    {"remove", 'r', NULL, 0, 
      N_("remove the extra line from the current working boilerplate.")},
    {"quiet", 'q', NULL, 0, N_("don't show diagnostic messages")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_extra_options_t *opt = NULL;
  if (state)
    opt = (struct lu_extra_options_t*) state->input;
  switch (key)
    {
    case 'q':
      opt->quiet = 1;
      break;
    case 'r':
      remove (get_config_file ("extra-line"));
      if (opt->quiet == 0)
        fprintf (stderr, "Removed.\n");
      exit (0);
      break;
    case ARGP_KEY_ARG:
        argz_add (&opt->text, &opt->text_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->text = NULL;
      opt->text_len = 0;
      opt->quiet = 0;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef EXTRA_DOC
#define EXTRA_DOC N_("Add some pre-license text to the working boilerplate." "\v" "Long lines will be automatically wrapped with the `fmt' command.")
static struct argp argp = { argp_options, parse_opt, "TEXT", EXTRA_DOC};

int 
lu_extra_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_extra_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_extra (state, &opts);
  else
    return err;
}

int 
lu_extra (struct lu_state_t *state, struct lu_extra_options_t *options)
{
  if (options->text)
    {
      argz_stringify (options->text, options->text_len, ' ');
      char *file = get_config_file ("extra-line");
      if (file)
        {
          FILE *fp = fopen (file, "w");
          if (fp)
            {
              fprintf (fp, "%s\n", options->text);
              fclose (fp);
            }
          free (file);
        }
      free (options->text);
      if (options->quiet == 0)
        fprintf (stderr, "Added.\n");
    }
  else
    {
      char *file = get_config_file ("extra-line");
      if (file)
        {
          FILE *fp = fopen (file, "r");
          if (fp)
            {
              size_t data_len;
              char *data = fread_file (fp, &data_len);
              if (data)
                {
                  luprintf (state, "%s", data);
                  free (data);
                }
              fclose (fp);
            }
          free (file);
        }
    }
  return 0;
}

struct lu_command_t extra = 
{
  .name         = N_("extra"),
  .doc          = EXTRA_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_extra_parse_argp
};
