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
#include <unistd.h>
#include "licensing_priv.h"
#include "fdl.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "util.h"

enum lgpl_version_opts
{
  OPT_V11 = -411,
  OPT_V12,
  OPT_V13,
};

static struct argp_option argp_options[] = 
{
    {"full-html", 'h', NULL, 0, 
      N_("show full license in html instead of text")},
    {"full", 'f', NULL, 0, N_("show the full license text")},
    {"v1.1", OPT_V11, NULL, 0, N_("show version 1.1 of the fdl")},
    {"v1.2", OPT_V12, NULL, 0, N_("show version 1.2 of the fdl")},
    {"v1.3", OPT_V13, NULL, 0, N_("show version 1.3 of the fdl")},
    {"jerkwad", 'j', NULL, 0, N_("remove the or-any-later-version clause")},
    {"list-licenses", 'l', NULL, OPTION_HIDDEN, N_("show licenses and exit")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_fdl_options_t *opt = NULL;
  if (state)
    opt = (struct lu_fdl_options_t*) state->input;
  switch (key)
    {
    case 'l':
        {
          char **license = fdl.licenses;
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
    case OPT_V11:
      opt->version = 1;
      break;
    case OPT_V12:
      opt->version = 2;
      break;
    case OPT_V13:
      opt->version = 3;
      break;
    case ARGP_KEY_INIT:
      opt->html = 0;
      opt->full = 0;
      opt->version = 3;
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
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef FDL_DOC
#define FDL_DOC N_("Show the GNU Free Documentation License notice.")
static struct argp argp = { argp_options, parse_opt, "", FDL_DOC};

int 
lu_fdl_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_fdl_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_fdl (state, &opts);
  else
    return err;
}

int
show_lu_fdl(struct lu_state_t *state, struct lu_fdl_options_t *options)
{
  char *file;
  if (options->version == 1)
    file = strdup ("old-licenses/fdl-1.1");
  else if (options->version == 2)
    file = strdup ("old-licenses/fdl-1.2");
  else
    file = strdup ("fdl-1.3");
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
  fileptr = fopen (tmp, "r");
  size_t data_len = 0;
  char *data = fread_file (fileptr, &data_len);
  fclose (fileptr);
  remove (tmp);
  if (options->html || options->full)
    luprintf (state, "%s\n", data);
  else
    {
      int replace = !options->future_versions;
      switch (options->version)
        {
        case 1:
          show_lines_after (state, data, 
                            "      Permission is granted to copy,", 7, replace,
                            "or any later version", "of the License as");
          break;
        case 2:
          show_lines_after (state, data, "    Permission is granted to copy,", 
                            6, replace, "or any later version", "of the License as");
          break;
        case 3:
          show_lines_after (state, data, "    Permission is granted to copy,", 
                            6, replace, "or any later version", "of the License as");
          break;
        }
    }
  free (url);
  free (data);
  return err;
}

int 
lu_fdl (struct lu_state_t *state, struct lu_fdl_options_t *options)
{
  int err = 0;
  err = show_lu_fdl(state, options);
  return err;
}
struct lu_command_t fdl = 
{
  .name         = N_("fdl"),
  .doc          = FDL_DOC,
  .flags        = IS_A_LICENSE | DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_fdl_parse_argp,
  .licenses     =
    {
      "fdlv13+ fdl --v3",
      "fdlv13  fdl --v3 --jerkwad",
      "fdlv12+ fdl --v2",
      "fdlv12  fdl --v2 --jerkwad",
      "fdlv11+ fdl --v1",
      "fdlv11  fdl --v1 --jerkwad",
      NULL
    }
};
