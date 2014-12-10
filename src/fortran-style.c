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
#include <argz.h>
#include "read-file.h"
#include "gettext-more.h"
#include "fortran-style.h"
#include "util.h"

enum {
  OPT_FORTRAN = -117,
};

static struct argp_option argp_options[] = 
{
    {"fortran-style", OPT_FORTRAN, NULL, OPTION_HIDDEN, 
      N_("fortran style comments  e.g. C foo")},
    {0},
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_comment_style_t **style = NULL;
  if (state)
    style = (struct lu_comment_style_t **) state->input;
  switch (key)
    {
    case OPT_FORTRAN:
      *style = &fortran_style;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp fortran_style_argp = { argp_options, parse_opt, "", "", 0};

static char *
comment (char *text)
{
  return create_line_comment (text, "C");
}

static void
uncomment (char **argz, size_t *len, int trim)
{
  uncomment_comments (argz, len, "C", "!", trim, 1, 1);
  return;
}

static int
get_comment (FILE *fp, char **argz, size_t *len, char **hashbang)
{
  return get_comment_blocks (fp, argz, len, hashbang, 
     "((^C[ ]|^\\![ ]).*[\\r\\n]|(^C[\r\n]|^\\![\r\n]))*");
}

struct lu_comment_style_t fortran_style=
{
  .name                = "fortran",
  .argp                = &fortran_style_argp,
  .get_initial_comment = get_comment,
  .comment             = comment,
  .uncomment           = uncomment,
  .support_file_exts   = ".f .for .f90 .f95",
  .avoid_file_exts     = NULL, // avoid no file extensions
};
