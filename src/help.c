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
#include "help.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "error.h"
#include "opts.h"

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_help_options_t *opt = NULL;
  if (state)
    opt = (struct lu_help_options_t*) state->input;
  switch (key)
    {
    case ARGP_KEY_ARG:
      if (opt->command == NULL)
        opt->command = arg;
      else
        {
          argp_failure (state, 0, 0, N_("Too many arguments"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    case ARGP_KEY_INIT:
      opt->command = NULL;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef HELP_DOC
#define HELP_DOC N_("Show some explanatory text for commands.") "\v"\
  N_("If no COMMAND is specified then a list of commands is shown.")
static struct argp argp = { NULL, parse_opt, "[COMMAND]", HELP_DOC};

int 
lu_help_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_help_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_help (state, &opts);
  else
    return err;
}

int 
lu_help (struct lu_state_t *state, struct lu_help_options_t *options)
{
  if (options->command)
    {
      if (lu_is_command (options->command))
        {
          char *cmd = xasprintf ("%s --help", options->command);
          lu_parse_command (state, cmd);
          free (cmd);
          free (options->command);
        }
      else
        error (0, 0, N_("unknown command `%s'"), options->command);
    } 
  else
    {
      fprintf (state->out, _("List of Commands that show Licenses and License Notices:\n"));
      char *lines = lu_list_of_commands_for_help(3);
      fprintf (state->out, "%s\n\n", lines);
      free (lines);
      lines = lu_list_of_commands_for_help(2);
      fprintf (state->out, _("List of Commands:\n"));
      fprintf (state->out, "%s\n", lines);
      free (lines);
      fprintf (state->out, "\n%s\n", 
               N_("For more information on a command, type `help COMMAND'."));
    }
  return 0;
}

struct lu_command_t help = 
{
  .name         = N_("help"),
  .doc          = HELP_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | DO_NOT_SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_help_parse_argp
};
