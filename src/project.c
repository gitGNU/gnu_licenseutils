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
#include "project.h"
#include "gettext-more.h"
#include "read-file.h"

enum {
  OPT_BELONGS = -120,
};

static struct argp_option argp_options[] = 
{
    {"remove", 'r', NULL, 0, 
      N_("remove the project line from the current working boilerplate.")},
    {"belongs", OPT_BELONGS, NULL, 0, N_("use `this file belongs' alternative text")},
    {"quiet", 'q', NULL, 0, N_("don't show diagnostic messages")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_project_options_t *opt = NULL;
  if (state)
    opt = (struct lu_project_options_t*) state->input;
  switch (key)
    {
    case OPT_BELONGS:
      opt->belongs = 1;
      break;
    case 'q':
      opt->quiet = 1;
      break;
    case 'r':
      remove (get_config_file ("project-line"));
      if (opt->quiet == 0)
        fprintf (stderr, "Removed.\n");
      exit (0);
      break;
    case ARGP_KEY_ARG:
        argz_add (&opt->text, &opt->text_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->quiet = 0;
      opt->belongs = 0;
      opt->text = NULL;
      opt->text_len = 0;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}
#undef PROJECT_DOC
#define PROJECT_DOC N_("Add a project name to the current working boilerplate.") "\v" N_("This command causes a line identifying the project to be added to the current working boilerplate.  It has the form: `This file is part of NAME.'")
static struct argp argp = { argp_options, parse_opt, "NAME", PROJECT_DOC};

int 
lu_project_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_project_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_project (state, &opts);
  else
    return err;
}

int 
lu_project (struct lu_state_t *state, struct lu_project_options_t *options)
{
  if (options->text)
    {
      argz_stringify (options->text, options->text_len, ' ');
      char *file = get_config_file ("project-line");
      if (file)
        {
          FILE *fp = fopen (file, "w");
          if (fp)
            {
              if (options->belongs)
                fprintf (fp, N_("This file belongs to the %s project.\n"), 
                         options->text);
              else
                fprintf (fp, N_("This file is part of %s.\n"), options->text);
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
      char *file = get_config_file ("project-line");
      if (file)
        {
          FILE *fp = fopen (file, "r");
          if (fp)
            {
              size_t data_len;
              char *data = fread_file (fp, &data_len);
              if (data)
                {
                  printf("%s", data);
                  free (data);
                }
              fclose (fp);
            }
          free (file);
        }
    }
  return 0;
}

struct lu_command_t project = 
{
  .name         = N_("project"),
  .doc          = PROJECT_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_project_parse_argp
};
