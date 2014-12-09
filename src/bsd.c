/*  Copyright (C) 2013, 2014 Ben Asselstine

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
#include "bsd.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "error.h"
#include "util.h"
#include "url-downloader.h"

static struct argp_option argp_options[] = 
{
    {"2-clause", '2', NULL, 0, N_("show the 2-clause bsd license")},
    {"3-clause", '3', NULL, 0, N_("show the 3-clause bsd license")},
    {"list-licenses", 'l', NULL, OPTION_HIDDEN, N_("show licenses and exit")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_bsd_options_t *opt = NULL;
  if (state)
    opt = (struct lu_bsd_options_t*) state->input;
  switch (key)
    {
    case '2':
      opt->clause=2;
      break;
    case '3':
      opt->clause=3;
      break;
    case 'l':
        {
          char **license = bsd.licenses;
          while (*license)
            fprintf (stdout, "%s\n", *license++);
          exit (0);
        }
      break;
    case ARGP_KEY_INIT:
      opt->clause = 3;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef BSD_DOC
#define BSD_DOC N_("Show the Berkeley Software Distribution license.")
static struct argp argp = { argp_options, parse_opt, "", BSD_DOC};

int 
lu_bsd_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_bsd_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_bsd (state, &opts);
  else
    return err;
}

int
show_lu_bsd(struct lu_state_t *state, struct lu_bsd_options_t *options)
{
  char *url;
  if (options->clause == 3)
    url = strdup ("http://directory.fsf.org/wiki/License:BSD_3Clause");
  else if (options->clause == 2)
    url = strdup ("http://directory.fsf.org/wiki?title=License:FreeBSD");
  else 
    url = strdup ("http://directory.fsf.org/wiki/License:BSD_3Clause");
  char *data = NULL;
  int err = download (state, url, &data);
  free (url);

  replace_html_entities (data);

  if (options->clause == 3)
    err = show_lines_after (state, data, 
                            "Redistribution and use in source and ", 
                            27, 0, NULL, NULL);
  else if (options->clause == 2)
    err = show_lines_after (state, data, 
                            "Redistribution and use in source and ", 
                            21, 0, NULL, NULL);
  else
    err = show_lines_after (state, data, 
                            "Redistribution and use in source and ", 
                            27, 0, NULL, NULL);
  free (url);
  free (data);
  return err;
}

int 
lu_bsd (struct lu_state_t *state, struct lu_bsd_options_t *options)
{
  int err = 0;
  err = show_lu_bsd(state, options);
  return err;
}

struct lu_command_t bsd = 
{
  .name         = N_("bsd"),
  .doc          = BSD_DOC,
  .flags        = IS_A_LICENSE | DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_bsd_parse_argp,
  .licenses     = 
    { 
      "bsd3clause bsd",
      "bsd2clause bsd --2-clause",
      NULL
    }
};
