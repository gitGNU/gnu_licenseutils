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
#include "agpl.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "error.h"
#include "util.h"
#include "url-downloader.h"

static struct argp_option argp_options[] = 
{
    {"full-html", 'h', NULL, 0, 
      N_("show full license in html instead of text")},
    {"full", 'f', NULL, 0, N_("show the full license text")},
    {"jerkwad", 'j', NULL, 0, N_("remove the or-any-later-version clause")},
    {"list-licenses", 'l', NULL, OPTION_HIDDEN, N_("show licenses and exit")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_agpl_options_t *opt = NULL;
  if (state)
    opt = (struct lu_agpl_options_t*) state->input;
  switch (key)
    {
    case 'l':
        {
          char **license = agpl.licenses;
          while (*license)
            fprintf (stdout, "%s\n", *license++);
          exit (0);
        }
      break;
    case 'j':
      opt->future_versions = 0;
      break;
    case 'f':
      opt->full = 1;
      break;
    case 'h':
      opt->html = 1;
      break;
    case ARGP_KEY_INIT:
      opt->html = 0;
      opt->full = 0;
      opt->future_versions = 1;
      opt->fsf_address = 0;
      state->child_inputs[0] = &opt->fsf_address;
      break;
    case ARGP_KEY_FINI:
      if (opt->future_versions == 0 && opt->html)
        {
          argp_failure (state, 0, 0, 
                        N_("--jerkwad cannot be used with --full-html"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      else if (opt->future_versions == 0 && opt->full)
        {
          argp_failure (state, 0, 0, 
                        N_("--jerkwad cannot be used with --full"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      else if (opt->fsf_address && opt->full)
        {
          argp_failure (state, 0, 0, 
                        N_("cannot use an address option with --full"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static const struct argp_child
parsers[] =
{
    {&fsf_addresses_argp},
    { 0 },
};

#undef AGPL_DOC
#define AGPL_DOC N_("Show the GNU Affero General Public License notice.")
static struct argp argp = { argp_options, parse_opt, "", AGPL_DOC, parsers};

int 
lu_agpl_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_agpl_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_agpl (state, &opts);
  else
    return err;
}

int
show_lu_agpl(struct lu_state_t *state, struct lu_agpl_options_t *options)
{
  char *url = xasprintf ("%s/licenses/agpl-3.0%s.%s", GNU_SITE, 
                         options->html ? "-standalone" : "",
                         options->html ? "html" : "txt");
  char *data = NULL;
  int err = download (state, url, &data);
  free (url);

  if (options->full || options->html)
    luprintf (state, "%s\n", data);
  else
    {
      char *chunk = NULL;
      chunk = get_lines (data, "    This program is free software:", 12);
      if (!options->future_versions)
        err = text_replace (chunk, "either version 3 of the License, or\n    (at your option) any later version.", "version 3 of the License.");
      if (options->fsf_address)
        replace_fsf_address (&chunk, options->fsf_address, "Affero ", 4);
      luprintf (state, "%s\n", chunk);
      free (chunk);
    }
  free (data);
  return err;
}

int 
lu_agpl (struct lu_state_t *state, struct lu_agpl_options_t *options)
{
  int err = 0;
  err = show_lu_agpl(state, options);
  return err;
}

struct lu_command_t agpl = 
{
  .name         = N_("agpl"),
  .doc          = AGPL_DOC,
  .flags        = IS_A_LICENSE | DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_agpl_parse_argp,
  .licenses     = 
    { 
      "agplv3+ agpl",
      "agplv3 agpl --jerkwad",
      "agplv3+temple agpl --temple",
      "agplv3temple agpl --jerkwad --temple",
      "agplv3+franklin agpl --franklin",
      "agplv3franklin agpl --jerkwad --franklin",
      NULL
    }
};
