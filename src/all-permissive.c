/*  Copyright (C) 2013, 2014 Ben Asselstine

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
#include "all-permissive.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "error.h"
#include "util.h"
#include "url-downloader.h"

static struct argp_option argp_options[] = 
{
    {"list-licenses", 'l', NULL, OPTION_HIDDEN, N_("show licenses and exit")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'l':
        {
          char **license = all_permissive.licenses;
          while (*license)
            fprintf (stdout, "%s\n", *license++);
          exit (0);
        }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef ALL_PERMISSIVE_DOC
#define ALL_PERMISSIVE_DOC N_("Show the GNU All-Permissive License.")
static struct argp argp = { argp_options, parse_opt, "", ALL_PERMISSIVE_DOC};

int 
lu_all_permissive_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_all_permissive_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_all_permissive (state, &opts);
  else
    return err;
}

int
show_lu_all_permissive(struct lu_state_t *state, struct lu_all_permissive_options_t *options)
{
  char *url = xasprintf ("%s/prep/maintain/html_node/License-Notices-for-Other-Files.html", GNU_SITE);
  char *data = NULL;
  int err = download (state, url, &data);
  free (url);

  err = show_lines_after (state, data, "<pre class=\"smallexample\">", 4, 1, 
                          "<pre class=\"smallexample\">", "");
  free (data);
  return err;
}

int 
lu_all_permissive (struct lu_state_t *state, struct lu_all_permissive_options_t *options)
{
  int err = 0;
  err = show_lu_all_permissive(state, options);
  return err;
}

struct lu_command_t all_permissive = 
{
  .name         = N_("all-permissive"),
  .doc          = ALL_PERMISSIVE_DOC,
  .flags        = IS_A_LICENSE | DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_all_permissive_parse_argp,
  .licenses     = 
    { 
      "all-permissive all-permissive",
      NULL
    }
};
