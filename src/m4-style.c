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
#include <argz.h>
#include "read-file.h"
#include "gettext-more.h"
#include "m4-style.h"
#include "util.h"

enum {
  OPT_M4 = -113,
};

static struct argp_option argp_options[] = 
{
    {"m4-style", OPT_M4, NULL, OPTION_HIDDEN, 
      N_("m4 style comments  e.g. dnl foo")},
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
    case OPT_M4:
      *style = &m4_style;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp m4_style_argp = { argp_options, parse_opt, "", "", 0};

static char *
comment (char *text)
{
  return create_line_comment (text, "dnl");
}

static void
uncomment (char **argz, size_t *len, int trim)
{
  uncomment_comments (argz, len, "dnl", NULL, trim, 1, 0);
  return;
}

static int
get_comment (FILE *fp, char **argz, size_t *len, char **hashbang)
{
  return get_comment_blocks 
    (fp, argz, len, hashbang, 
     "((^[Dd][Nn][Ll][ \t]|^\\#).*[\\r\\n]|^[Dd][Nn][Ll][\r\n])*");
}

struct lu_comment_style_t m4_style=
{
  .name                = "m4",
  .argp                = &m4_style_argp,
  .get_initial_comment = get_comment,
  .comment             = comment,
  .uncomment           = uncomment,
  .support_file_exts   = ".m4 .ac",
  .avoid_file_exts     = NULL,
};
