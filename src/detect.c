/*  Copyright (C) 2014 Ben Asselstine

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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <argz.h>
#include "licensing_priv.h"
#include "detect.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "boilerplate.h"
#include "opts.h"
#include "read-file.h"
#include "trim.h"
#include "error.h"
#include "copy-file.h"
#include "styles.h"

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_detect_options_t *opt = NULL;
  if (state)
    opt = (struct lu_detect_options_t*) state->input;
  switch (key)
    {
    case ARGP_KEY_ARG:
      if (opt->input_file)
        {
          argp_failure (state, 0, 0, N_("can only specify one file."));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      else
        opt->input_file = strdup (arg);
      break;
    case ARGP_KEY_INIT:
      opt->input_file = NULL;
      break;
    case ARGP_KEY_FINI:
      if (opt->input_file == NULL)
        opt->input_file = strdup ("-");
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef DETECT_DOC
#define DETECT_DOC \
  N_("Try to statistically determine the license notice of a file.") "\v"\
  N_("With no FILE, or when FILE is -, it is read from standard input.")
static struct argp argp = { 0, parse_opt, "[FILE]", DETECT_DOC};

int 
lu_detect_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_detect_options_t opts;
  opts.state = state;
  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_detect (state, &opts);
  else
    return err;
}

static int
detect_licenses (struct lu_state_t *state, char *filename)
{
  char *argz = NULL;
  size_t argz_len = 0;
  char *licenses = lu_list_of_license_keyword_commands ();
  argz_create_sep (licenses, '\n', &argz, &argz_len);
  char *license = NULL;
  while ((license = argz_next (argz, argz_len, license)))
    {
      char *cmd = strchr (license, ' ');
      cmd++;
      char *license_filename = lu_dump_command_to_file (state, cmd);
      float results = 0.0; //sherlock (filename, license_filename);
      remove (license_filename);
      free (license_filename);
      cmd = strchr (license, ' ');
      *cmd = '\0';
      luprintf (state, "%s: %5.2f%%\n", license, results);
      *cmd = ' ';
    }
  free (argz);
  free (licenses);
  return 0;
}

static int
detect_stdin (struct lu_state_t *state, struct lu_detect_options_t *options)
{
  int err = 0;
  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp(tmp);
  close (fd);
  FILE *fileptr = fopen (tmp, "w");
  if (fileptr)
    {
      char *line = NULL;
      size_t len = 0;
      ssize_t read;

      while ((read = getline(&line, &len, stdin)) != -1)
        fprintf (fileptr, "%s", line);

      free (line);
      fflush (fileptr);
      fsync (fileno (fileptr));
      fclose (fileptr);
      detect_licenses (state, tmp);
      remove (tmp);
    }
  else
    err = -1;
  return err;
}

static int
detect_boilerplate (struct lu_state_t *state, struct lu_detect_options_t *options)
{
  struct lu_boilerplate_options_t boilerplate_options;
  memset (&boilerplate_options, 0, sizeof (boilerplate_options));
  argz_add (&boilerplate_options.input_files, &boilerplate_options.input_files_len, options->input_file);

  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp(tmp);
  close (fd);
  FILE *fileptr = fopen (tmp, "w");
  FILE *oldout = state->out;
  state->out = fileptr;
  int err = lu_boilerplate (state, &boilerplate_options);
  state->out = oldout;
  fclose (fileptr);
  detect_licenses (state, tmp);
  remove (tmp);
  return err;
}

int 
lu_detect (struct lu_state_t *state, struct lu_detect_options_t *options)
{
  int err = 0;
  if (strcmp (options->input_file, "-") == 0)
    err = detect_stdin (state, options);
  else
    err = detect_boilerplate (state, options);
  return err;
}

struct lu_command_t detect = 
{
  .name         = N_("detect"),
  .doc          = DETECT_DOC,
  .flags        = SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_detect_parse_argp
};
