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
#include "licensing_priv.h"
#include "new-boilerplate.h"
#include "gettext-more.h"

static struct argp_option argp_options[] = 
{
    {"quiet", 'q', NULL, 0, N_("don't show diagnostic messages")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_new_boilerplate_options_t *opt = NULL;
  if (state)
    opt = (struct lu_new_boilerplate_options_t*) state->input;
  switch (key)
    {
    case 'q':
      opt->quiet = 1;
      break;
    case ARGP_KEY_INIT:
      opt->quiet = 0;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef NEW_BOILERPLATE_DOC
#define NEW_BOILERPLATE_DOC N_("Clear the current working boilerplate.")
static struct argp argp = { argp_options, parse_opt, "", NEW_BOILERPLATE_DOC};

int 
lu_new_boilerplate_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_new_boilerplate_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_new_boilerplate (state, &opts);
  else
    return err;
}

int 
lu_new_boilerplate (struct lu_state_t *state, struct lu_new_boilerplate_options_t *options)
{
  char *f;
  f = get_config_file ("selected-comment-style");
  remove (f);
  free (f);
  f = get_config_file ("selected-licenses");
  remove (f);
  free (f);
  f = get_config_file ("copyright-holders");
  remove (f);
  free (f);
  f = get_config_file ("license-notice");
  remove (f);
  free (f);
  f = get_config_file ("top-line");
  remove (f);
  free (f);
  f = get_config_file ("project-line");
  remove (f);
  free (f);
  f = get_config_file ("extra-line");
  remove (f);
  free (f);
  if (options->quiet == 0)
    fprintf (stderr, "Removed.\n");
  return 0;
}

struct lu_command_t new_boilerplate = 
{
  .name         = N_("new-boilerplate"),
  .doc          = NEW_BOILERPLATE_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_new_boilerplate_parse_argp
};
