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
#include "apache.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "error.h"
#include "util.h"
#include "url-downloader.h"

static struct argp_option argp_options[] = 
{
    {"full", 'f', NULL, 0, N_("show the full license text")},
    {"list-licenses", 'l', NULL, OPTION_HIDDEN, N_("show licenses and exit")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_apache_options_t *opt = NULL;
  if (state)
    opt = (struct lu_apache_options_t*) state->input;
  switch (key)
    {
    case 'f':
      opt->full = 1;
      break;
    case 'l':
        {
          char **license = apache.licenses;
          while (*license)
            fprintf (stdout, "%s\n", *license++);
          exit (0);
        }
      break;
    case ARGP_KEY_INIT:
      opt->full = 0;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef APACHE_DOC
#define APACHE_DOC N_("Show the Apache License notice.")
static struct argp argp = { argp_options, parse_opt, "", APACHE_DOC};

int 
lu_apache_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_apache_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_apache (state, &opts);
  else
    return err;
}

int
show_lu_apache(struct lu_state_t *state, struct lu_apache_options_t *options)
{
  char *url = strdup ("http://directory.fsf.org/wiki/License:Apache2.0");
  char *data = NULL;
  int err = download (state, url, &data);
  free (url);

  replace_html_entities (data);
  if (options->full)
    err = show_lines_after (state, data, "<pre>", 202, 1, "<pre>", "");
  else
    err = show_lines_after (state, data, "   Licensed under the Apache License", 11, 0, NULL, NULL);
  free (data);
  return err;
}

int 
lu_apache (struct lu_state_t *state, struct lu_apache_options_t *options)
{
  int err = 0;
  err = show_lu_apache(state, options);
  return err;
}

struct lu_command_t apache = 
{
  .name         = N_("apache"),
  .doc          = APACHE_DOC,
  .flags        = IS_A_LICENSE | DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_apache_parse_argp,
  .licenses     = 
    { 
      "apachev2 apache",
      NULL
    }
};
