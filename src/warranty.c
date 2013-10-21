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
#include "warranty.h"
#include "gettext-more.h"

#undef WARRANTY_DOC
#define WARRANTY_DOC N_("Show a disclaimer on use of this program.")
static struct argp argp = { NULL, NULL, "", WARRANTY_DOC};

int 
lu_warranty_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_warranty_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_warranty (state, &opts);
  else
    return err;
}

int 
lu_warranty (struct lu_state_t *state, struct lu_warranty_options_t *options)
{
  fprintf (state->out, "%s %s\n", PACKAGE, VERSION);
  fprintf (state->out, "Copyright (C) 2013 Ben Asselstine\n");
  fprintf (state->out, "\n");
  fprintf (state->out, _("\
    This program is free software; you can redistribute it and/or modify\n\
    it under the terms of the GNU General Public License as published by\n\
    the Free Software Foundation; either version 2 of the License, or\n\
    (at your option) any later version.\n\
\n\
    This program is distributed in the hope that it will be useful,\n\
    but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
    GNU General Public License for more details.\n\
\n\
    You should have received a copy of the GNU General Public License \n\
    along with this program. If not, see <http://www.gnu.org/licenses/>.\n\
\n\
    The Free Software Foundation, Inc.\n\
    51 Franklin Street, Fifth Floor\n\
    Boston, MA 02110-1301  USA\n\n\
"));
  return 0;
}

struct lu_command_t warranty = 
{
  .name         = N_("warranty"),
  .doc          = WARRANTY_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | DO_NOT_SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_warranty_parse_argp
};
