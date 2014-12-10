/*  Copyright (C) 2014 Ben Asselstine

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
#include "forget.h"
#include "help.h"
#include "url-downloader.h"
#include "gettext-more.h"

#undef FORGET_DOC
#define FORGET_DOC N_("Clear the downloaded files cache.")
static struct argp argp = { NULL, NULL, "", FORGET_DOC};

int 
lu_forget_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_forget_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_forget (state, &opts);
  else
    return err;
}

int 
lu_forget (struct lu_state_t *state, struct lu_forget_options_t *options)
{
  clear_download_cache ();
  return 0;
}

struct lu_command_t forget = 
{
  .name         = N_("forget"),
  .doc          = FORGET_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | DO_NOT_SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_forget_parse_argp
};
