/*  Copyright (C) 2014 Ben Asselstine

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
#include <argp.h>
#include <stdlib.h>
#include "gettext-more.h"
#include "fsf-addresses.h"
#include "licensing_priv.h"
#include "xvasprintf.h"

char *
get_address (int address, char *license, int indent)
{
  char *s = NULL;
  char *i = malloc (indent + 1);
  memset (i, 0, indent + 1);
  memset (i, ' ', indent);
  if (address & FSF_ADDRESS_MASS)
      s = xasprintf ("%sYou should have received a copy of the GNU %sGeneral Public License\n"
                 "%salong with this program; if not, write to the Free Software\n"
                 "%sFoundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.", i, license, i, i);
  else if (address & FSF_ADDRESS_TEMPLE)
      s = xasprintf ("%sYou should have received a copy of the GNU %sGeneral Public License\n"
                 "%salong with this program; if not, write to the Free Software\n"
                 "%sFoundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.", i, license, i, i);
  else if (address & FSF_ADDRESS_FRANKLIN)
      s = xasprintf ("%sYou should have received a copy of the GNU %sGeneral Public License\n"
                 "%salong with this program; if not, write to the Free Software\n"
                 "%sFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.", i, license, i, i);
  else if (address & FSF_ADDRESS_LINK)
    return
      xasprintf ("%sYou should have received a copy of the GNU %sGeneral Public License\n"
                 "%salong with this program.  If not, see <http://www.gnu.org/licenses/>.", i, license, i);

  free (i);
  return s;
}

enum fsf_address_opts
{
  OPT_LINK = -333,
  OPT_FRANKLIN, 
  OPT_TEMPLE, 
  OPT_MASS, 
};

static struct argp_option argp_options[] = 
{
    {"address-temple", OPT_TEMPLE, NULL, 0, N_("show with the old Temple Place address")},
    {"temple", OPT_TEMPLE, NULL, OPTION_ALIAS | OPTION_HIDDEN },
    {"address-mass", OPT_MASS, NULL, 0, N_("show with the old Mass Ave address")},
    {"mass", OPT_MASS, NULL, OPTION_ALIAS | OPTION_HIDDEN },
    {"address-franklin", OPT_FRANKLIN, NULL, 0, N_("show with the Franklin Ave address")},
    {"franklin", OPT_FRANKLIN, NULL, OPTION_ALIAS | OPTION_HIDDEN },
    {"address-link", OPT_LINK, NULL, 0, N_("show with the online address (default)")},
    {"link", OPT_LINK, NULL, OPTION_ALIAS | OPTION_HIDDEN },
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  int *address = state->input;
  switch (key)
    {
    case OPT_TEMPLE:
      *address |= FSF_ADDRESS_TEMPLE;
      break;
    case OPT_LINK:
      *address |= FSF_ADDRESS_LINK;
      break;
    case OPT_MASS:
      *address |= FSF_ADDRESS_MASS;
      break;
    case OPT_FRANKLIN:
      *address |= FSF_ADDRESS_FRANKLIN;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp fsf_addresses_argp = { argp_options, parse_opt, "", ""};
