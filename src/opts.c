/*  Copyright (C) 2011 Ben Asselstine

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

//keep it going man.

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <argz.h>
#include <unistd.h>
#include "xvasprintf.h"
#include "gettext-more.h"
#include "opts.h"
#include "licensing_priv.h"

#define FULL_VERSION PROGRAM " " PACKAGE_VERSION

const char *argp_program_version = FULL_VERSION;
const char *argp_program_bug_address = "<" PACKAGE_BUGREPORT ">";

struct arguments_t arguments;
static struct argp_option options[] = 
{
    { "quiet", OPT_QUIET, NULL, 0, N_("don't show the welcome message") },
    { "generate-bashrc", OPT_BASH, NULL, OPTION_HIDDEN, N_("generate a bashrc file and exit") },
    { 0 }
};

static void
init_options (struct lu_options_t *app)
{
  app->quiet = -1;
  app->command_on_argv = NULL;
  app->command_on_argv_len = 0;
  return;
}

static error_t 
parse_opt (int key, char *arg, struct argp_state *state) 
{
  struct arguments_t *arguments = (struct arguments_t *) state->input;

  switch (key) 
    {
    case OPT_QUIET:
      arguments->lu.quiet = 1;
      break;
    case ARGP_KEY_INIT:
      init_options (&arguments->lu);
      break;
    case ARGP_KEY_FINI:
      if (arguments->lu.command_on_argv)
        argz_stringify (arguments->lu.command_on_argv, 
                        arguments->lu.command_on_argv_len, ' ');
      break;
    case ARGP_KEY_ARG:
      if (arguments->lu.command_on_argv == NULL)
        state->quoted = state->next; //ignore options after this.
      arguments->lu.quiet = 1;
      argz_add (&arguments->lu.command_on_argv, 
                &arguments->lu.command_on_argv_len, arg);
      break;
    case OPT_BASH:
      lu_generate_bashrc_file (stdout);
      exit(0);
      break;
    case ARGP_KEY_END:
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static char *
help_filter (int key, const char *text, void *input)
{
  if (key == ARGP_KEY_HELP_PRE_DOC)
    {
      char *argz = NULL;
      size_t len = 0;
      argz_add (&argz, &len, text);
      argz_add (&argz, &len, "");
      argz_add (&argz, &len, "The most commonly used commands are:");
      char *commands = lu_list_of_commands_for_help(0);
      argz_add (&argz, &len, commands);
      free (commands);
      argz_add (&argz, &len, "");
      argz_add (&argz, &len, "Options:");
      argz_stringify (argz, len, '\n');
      return argz;
    }
  return (char *) text;
}

struct argp argp = { options, parse_opt, "[COMMAND [OPTION...]]",
  N_("A command-line interface for free software licensing.") "\v"
    N_("To see a list of all commands type `" PROGRAM " help'."),
  0, help_filter, PACKAGE }; 

static void 
set_default_options (struct lu_options_t *app)
{
  if (app->quiet == -1)
    app->quiet = atoi (DEFAULT_QUIET_VALUE);
  return;
}

void 
parse_opts (int argc, char **argv, struct arguments_t *arguments)
{
  int retval;
  setenv ("ARGP_HELP_FMT", "no-dup-args-note", 1);
  char *argz = NULL;
  size_t argz_len = 0;
  if (argc > 2)
    {
      if (strchr (argv[1], ' '))
        argz_create_sep (argv[1], ' ', &argz, &argz_len);
    }
  retval = argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, arguments); 
  if (retval < 0)
    argp_help (&argp, stdout, ARGP_HELP_EXIT_ERR|ARGP_HELP_SEE,PACKAGE_NAME);
  set_default_options (&arguments->lu);

  argp_program_version = 0;
  argp_program_bug_address = 0;
  return;
}
