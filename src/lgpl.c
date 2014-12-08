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
#include "lgpl.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "error.h"
#include "util.h"

enum lgpl_version_opts
{
  OPT_V20 = -211,
  OPT_V21,
  OPT_V3,
};
static struct argp_option argp_options[] = 
{
    {"full-html", 'h', NULL, 0, 
      N_("show full license in html instead of text")},
    {"full", 'f', NULL, 0, N_("show the full license text")},
    {"v2", OPT_V20, NULL, 0, N_("show version 2.0 of the lgpl")},
    {"v2.1", OPT_V21, NULL, 0, N_("show version 2.1 of the lgpl")},
    {"v3", OPT_V3, NULL, 0, N_("show version 3 of the lgpl")},
    {"jerkwad", 'j', NULL, 0, N_("remove the or-any-later-version clause")},
    {"list-licenses", 'l', NULL, OPTION_HIDDEN, N_("show licenses and exit")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_lgpl_options_t *opt = NULL;
  if (state)
    opt = (struct lu_lgpl_options_t*) state->input;
  switch (key)
    {
    case 'l':
        {
          char **license = lgpl.licenses;
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
    case OPT_V20:
      opt->version = 0;
      break;
    case OPT_V21:
      opt->version = 1;
      break;
    case OPT_V3:
      opt->version = 3;
      break;
    case ARGP_KEY_INIT:
      opt->html = 0;
      opt->full = 0;
      opt->version = 3;
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

#undef LGPL_DOC
#define LGPL_DOC N_("Show the GNU Lesser General Public License notice.")
static struct argp argp = { argp_options, parse_opt, "", LGPL_DOC, parsers};

int 
lu_lgpl_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_lgpl_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_lgpl (state, &opts);
  else
    return err;
}

static char *
get_lgplv3_boilerplate ()
{
    return xasprintf ("%s",
              "\
    This library is free software: you can redistribute it and/or modify\n\
    it under the terms of the GNU Lesser General Public License as published\n\
    by the Free Software Foundation, either version 3 of the License, or\n\
    (at your option) any later version.\n\
\n\
    This program is distributed in the hope that it will be useful,\n\
    but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
    GNU General Public License for more details.\n\
\n\
    You should have received a copy of the GNU Lesser General Public License\n\
    along with this program.  If not, see <http://www.gnu.org/licenses/>.");
}

int
show_lu_lgpl(struct lu_state_t *state, struct lu_lgpl_options_t *options)
{
  char *file;
  if (options->version == 0)
    file = strdup ("old-licenses/lgpl-2.0");
  else if (options->version == 1)
    file = strdup ("old-licenses/lgpl-2.1");
  else
    file = strdup ("lgpl-3.0");
  char *url = xasprintf ("%s/licenses/%s%s.%s", GNU_SITE, file,
                         options->html ? "-standalone" : "",
                         options->html ? "html" : "txt");
  free (file);
  int err = 0;
  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp(tmp);
  close (fd);
  FILE *fileptr = fopen (tmp, "wb");
  curl_easy_setopt (state->curl, CURLOPT_HTTPGET, 1);
  curl_easy_setopt (state->curl, CURLOPT_URL, url);
  curl_easy_setopt (state->curl, CURLOPT_WRITEDATA, fileptr);
  curl_easy_perform(state->curl);
  fflush (fileptr);
  fsync (fileno (fileptr));
  fclose (fileptr);
  int response = 0;
  curl_easy_getinfo (state->curl, CURLINFO_RESPONSE_CODE, &response);
  if (response != 200)
    {
      remove (tmp);
      error (0, 0, N_("got unexpected response code %d from %s"), response,
             url);
      err = 1;
      return err;
    }
  free (url);
  fileptr = fopen (tmp, "r");
  size_t data_len = 0;
  char *data = fread_file (fileptr, &data_len);
  fclose (fileptr);
  remove (tmp);
  if (options->html || options->full)
    luprintf (state, "%s\n", data);
  else
    {
      char *chunk = NULL;
      switch (options->version)
        {
        case 0:
          chunk = get_lines (data, "    This library is free software;", 13);
          break;
        case 1:
          chunk = get_lines (data, "    This library is free software;", 13);
          break;
        case 3:
          chunk = get_lgplv3_boilerplate (state);
          break;
        }

      if (!options->future_versions)
        {
          switch (options->version)
            {
            case 0:
              err = text_replace (chunk, "either\n    version 2 of the License, or (at your option) any later version.", "version 2\n    of the License.");
              break;
            case 1:
              err = text_replace (chunk, "either\n    version 2.1 of the License, or (at your option) any later version.", "version 2.1\n    of the License.");
              break;
            case 3:
              err = text_replace (chunk, "either version 3 of the License, or\n(at your option) any later version.", "version 3 of the License.");
              break;
            }
        }
      if (options->fsf_address)
        replace_fsf_address (&chunk, options->fsf_address, "Lesser ", 4);
      luprintf (state, "%s\n", chunk);
      free (chunk);
    }
  free (data);
  return err;
}

int 
lu_lgpl (struct lu_state_t *state, struct lu_lgpl_options_t *options)
{
  int err = 0;
  err = show_lu_lgpl(state, options);
  return err;
}
struct lu_command_t lgpl = 
{
  .name         = N_("lgpl"),
  .doc          = LGPL_DOC,
  .flags        = IS_A_LICENSE | DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_lgpl_parse_argp,
  .licenses     =
    {
      "lgplv3+ lgpl --v3",
      "lgplv3 lgpl --v3 --jerkwad",
      "lgplv2+ lgpl --v2.1",
      "lgplv2 lgpl --v2.1 --jerkwad",
      "lgplv1+ lgpl --v2",
      "lgplv1 lgpl --v2 --jerkwad",
      "lgplv1+temple lgpl --v2 --temple",
      "lgplv1temple lgpl --v2 --jerkwad --temple",
      "lgplv2+temple lgpl --v2.1 --temple",
      "lgplv2temple lgpl --v2.1 --jerkwad --temple",
      "lgplv3+franklin lgpl --v3 --franklin",
      "lgplv3franklin lgpl --v3 --jerkwad --franklin",
      NULL
    }
};
