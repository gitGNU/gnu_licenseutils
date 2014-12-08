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
#include "gpl.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "error.h"
#include "util.h"

static struct argp_option argp_options[] = 
{
    {"full-html", 'h', NULL, 0, 
      N_("show full license in html instead of text")},
    {"full", 'f', NULL, 0, N_("show the full license text")},
    {"v1", '1', NULL, 0, N_("show version 1 of the gpl")},
    {"v2", '2', NULL, 0, N_("show version 2 of the gpl")},
    {"v3", '3', NULL, 0, N_("show version 3 of the gpl")},
    {"jerkwad", 'j', NULL, 0, N_("remove the or-any-later-version clause")},
    {"list-licenses", 'l', NULL, OPTION_HIDDEN, N_("show licenses and exit")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_gpl_options_t *opt = NULL;
  if (state)
    opt = (struct lu_gpl_options_t*) state->input;
  switch (key)
    {
    case 'l':
        {
          char **license = gpl.licenses;
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
    case '1':
      opt->version = 1;
      break;
    case '2':
      opt->version = 2;
      break;
    case '3':
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

#undef GPL_DOC
#define GPL_DOC N_("Show the GNU General Public License notice.")

static struct argp argp = { argp_options, parse_opt, "",  GPL_DOC, parsers};

int 
lu_gpl_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_gpl_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_gpl (state, &opts);
  else
    return err;
}

int
show_lu_gpl(struct lu_state_t *state, struct lu_gpl_options_t *options)
{
  char *file;
  if (options->version == 1)
    file = strdup ("gpl-1.0");
  else if (options->version == 2)
    file = strdup ("gpl-2.0");
  else
    file = strdup ("gpl");
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
  if (options->full || options->html)
    luprintf (state, "%s\n", data);
  else
    {
      char *chunk = NULL;
      switch (options->version)
        {
        case 1:
          chunk = get_lines (data, "    This program is free software;", 13);
          break;
        case 2:
          chunk = get_lines (data, "    This program is free software;", 13);
          break;
        case 3:
          chunk = get_lines (data, "    This program is free software:", 12);
          break;
        }

      if (!options->future_versions)
        {
          switch (options->version)
            {
            case 1:
              err = text_replace (chunk, "either version 1, or (at your option)\n    any later version.", "version 1 of the License.");
              break;
            case 2:
              err = text_replace (chunk, "either version 2 of the License, or\n    (at your option) any later version.", "version 2 of the License.");
              break;
            case 3:
              err = text_replace (chunk, "either version 3 of the License, or\n    (at your option) any later version.", "version 3 of the License.");
              break;
            }
        }

      if (options->fsf_address)
        replace_fsf_address (&chunk, options->fsf_address, "", 4);

      luprintf (state, "%s\n", chunk);
      free (chunk);
    }
  free (data);
  return err;
}

int 
lu_gpl (struct lu_state_t *state, struct lu_gpl_options_t *options)
{
  int err = 0;
  err = show_lu_gpl(state, options);
  return err;
}

struct lu_command_t gpl = 
{
  .name         = N_("gpl"),
  .doc          = GPL_DOC,
  .flags        = IS_A_LICENSE | DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_gpl_parse_argp,
  .licenses     =
    {
      "gplv3+ gpl --v3",
      "gplv3  gpl --v3 --jerkwad",
      "gplv2+ gpl --v2",
      "gplv2  gpl --v2 --jerkwad",
      "gplv1+ gpl --v1",
      "gplv1  gpl --v1 --jerkwad",
      "gplv1mass  gpl --v1 --jerkwad --mass",
      "gplv1+mass  gpl --v1 --mass",
      "gplv2temple  gpl --v2 --jerkwad --temple",
      "gplv2+temple gpl --v2 --temple",
      "gplv2franklin gpl --v2 --jerkwad --franklin",
      "gplv2+franklin gpl --v2 --franklin",
      "gplv3franklin gpl --v3 --jerkwad --franklin",
      "gplv3+franklin gpl --v3 --franklin",
      NULL
    }
};
