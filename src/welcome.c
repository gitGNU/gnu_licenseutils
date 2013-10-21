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
#include "licensing_priv.h"
#include "welcome.h"
#include "warranty.h"
#include "help.h"
#include "gettext-more.h"

#undef WELCOME_DOC
#define WELCOME_DOC N_("Show the greeting.")
static struct argp argp = { NULL, NULL, "", WELCOME_DOC};

int 
lu_welcome_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_welcome_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_welcome (state, &opts);
  else
    return err;
}

int 
lu_welcome (struct lu_state_t *state, struct lu_welcome_options_t *options)
{
  fprintf (state->out, "%s %s\n", PACKAGE, VERSION);
  fprintf (state->out, "Copyright (C) 2013 Ben Asselstine\n");
  fprintf (state->out, "%s\n", 
           _("This is free software with ABSOLUTELY NO WARRANTY."));
  fprintf (state->out, _("For warranty details type `%s'.\n"), 
           warranty.name);
  fprintf (state->out, _("For a list of commands type `%s'.\n"), help.name);
  return 0;
}

struct lu_command_t welcome = 
{
  .name         = N_("welcome"),
  .doc          = WELCOME_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | DO_NOT_SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_welcome_parse_argp
};
