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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <argz.h>
#include "licensing_priv.h"
#include "detect.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "boilerplate.h"
#include "uncomment.h"
#include "opts.h"
#include "read-file.h"
#include "error.h"
#include "copy-file.h"
#include "fstrcmp.h"
#include "findprog.h"

enum detect_options_enum_t
{
  OPT_DIFF_PROGRAM = -333,
};

static struct argp_option argp_options[] = 
{
    { "show-diff", 's', 0, 0, N_("show the differences between the most similar license notice and the uncommented boilerplate of FILE")},
    { "diff-program", OPT_DIFF_PROGRAM, "PROGRAM", 0, 
      N_("use PROGRAM to show comparisons (default 'diff')") },
    { 0 }
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_detect_options_t *opt = NULL;
  if (state)
    opt = (struct lu_detect_options_t*) state->input;
  switch (key)
    {
    case 's':
      opt->show = 1;
      break;
    case OPT_DIFF_PROGRAM:
      opt->diff_program = arg;
      break;
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
      opt->diff_program = getenv ("LU_DIFF");
      opt->show = 0;
      break;
    case ARGP_KEY_FINI:
      if (opt->input_file == NULL)
        opt->input_file = strdup ("-");
      if (!opt->diff_program)
        opt->diff_program = "diff";
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#undef DETECT_DOC
#define DETECT_DOC \
  N_("Statistically determine the license notice of a file.") "\v"\
  N_("With no FILE, or when FILE is -, it is read from standard input.") "  " \
  N_("When FILE is given on the command line it is passed through the boilerplate command, and the uncomment command, while the standard input is not.") "  " \
  N_("The LU_DIFF environment variable overrides the default value of --diff-program.") "  " \
  N_("To pass options to diff, use the LU_DIFF_OPTS environment variable.")
static struct argp argp = { argp_options, parse_opt, "[FILE]", DETECT_DOC};

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

struct license_result_t
{
  char *license;
  char *cmd;
  float result;
};

int
compare_license_results (const void *lhs, const void *rhs)
{
  struct license_result_t *r = (struct license_result_t*) rhs;
  struct license_result_t *l = (struct license_result_t*) lhs;
  if (r->result == l->result)
    return 0;
  else if (l->result < r->result)
    return 1;
  else
    return -1;
}

//remove whitespace from string
static char *
squeeze (char *s)
{
  int i = 0;
  char *letter;
  char *result = strdup (s);
  result[0] = '\0';
  for (letter = &s[0]; *letter != '\0'; letter++)
    {
      if (isspace (*letter))
        continue;
      if (ispunct (*letter))
        continue;
      result[i] = *letter;
      i++;
    }
  result[i] = '\0';
  return result;
}

static float
sherlock (char *license_filename, char *filename)
{
  double results;
  size_t s1_len, s2_len;
  FILE *f1 = fopen (license_filename, "r");
  FILE *f2 = fopen (filename, "r");
  char *s1 = fread_file (f1, &s1_len);
  char *s2 = fread_file (f2, &s2_len);
  fclose (f1);
  fclose (f2);
  char *ss1 = squeeze (s1);
  char *ss2 = squeeze (s2);
  free (s1);
  free (s2);
  if (strstr (ss2, ss1) == NULL)
    results = fstrcmp (ss1, ss2);
  else
    results = 1;
  free (ss1);
  free (ss2);
  return results;
}

static int 
visual_diff (char *diff_program, char *diff_options, char* license_filename, char *filename)
{
  const char *prog;
  if (strchr (diff_program, '/'))
    prog = strdup (diff_program);
  else
    prog = find_in_path (diff_program);

  if (!prog)
    return 0;
  char *opts = "";
  if (strcmp (diff_program, "diff") == 0 && diff_options == NULL)
    opts = "-uNrd";
  else if (diff_options)
    opts = diff_options;
  char *cmd = xasprintf ("%s %s %s %s", prog, opts, license_filename, filename);
  free ( (char *) prog);
  system (cmd);
  free (cmd);
  return 0;
}

static int
detect_licenses (struct lu_state_t *state, struct lu_detect_options_t *options, char *filename)
{
  char *argz = NULL;
  size_t argz_len = 0;
  char *licenses = lu_list_of_license_keyword_commands ();
  argz_create_sep (licenses, '\n', &argz, &argz_len);
  int n = argz_count (argz, argz_len);
  struct license_result_t *m = malloc (n * sizeof (struct license_result_t));
  memset (m, 0, n * sizeof (struct license_result_t));
  int i = 0;
  char *license = NULL;
  //collect results
  while ((license = argz_next (argz, argz_len, license)))
    {
      char *cmd = strchr (license, ' ');
      cmd++;
      char *license_filename = lu_dump_command_to_file (state, cmd);
      float results = sherlock (license_filename, filename) * 100;
      remove (license_filename);
      free (license_filename);
      cmd = strchr (license, ' ');
      *cmd = '\0';
      m[i].license = strdup (license);
      m[i].cmd = strdup (++cmd);
      --cmd;
      m[i].result = results;
      *cmd = ' ';
      i++;
    }
  free (argz);
  free (licenses);
  //sort and display results
  if (n)
    {
      qsort (m, n, sizeof (struct license_result_t), compare_license_results);
      if (options->show)
        {
          char *license_filename = lu_dump_command_to_file (state, m[0].cmd);

          visual_diff (options->diff_program, getenv ("LU_DIFF_OPTS"),
                       license_filename, filename);
          remove (license_filename);
        }
      else
        {
          for (i = 0; i < n; i++)
            {
              if (m[i].result == 0.0)
                break;
              if (m[i].license)
                luprintf (state, "%-20s %6.3f%%\n", m[i].license, m[i].result);
            }
        }
    }

  for (i = 0; i < n; i++)
    {
      free (m[i].license);
      free (m[i].cmd);
    }
  free (m);
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
      detect_licenses (state, options, tmp);
      remove (tmp);
    }
  else
    err = -1;
  return err;
}

static int
detect_uncommented_boilerplate (struct lu_state_t *state, struct lu_detect_options_t *options)
{
  struct lu_boilerplate_options_t boilerplate_options;
  memset (&boilerplate_options, 0, sizeof (boilerplate_options));
  argz_add (&boilerplate_options.input_files, &boilerplate_options.input_files_len, options->input_file);

  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp(tmp);
  close (fd);
  char *tmpext = xasprintf ("%s.%s", tmp, basename (options->input_file));
  rename (tmp, tmpext);
  FILE *fileptr = fopen (tmpext, "w");
  FILE *oldout = state->out;
  state->out = fileptr;
  int err = lu_boilerplate (state, &boilerplate_options);
  state->out = oldout;
  fclose (fileptr);

  struct lu_uncomment_options_t uncomment_options;
  memset (&uncomment_options, 0, sizeof (uncomment_options));
  argz_add (&uncomment_options.input_files, &uncomment_options.input_files_len, tmpext);
  char tmp2[sizeof(PACKAGE) + 13];
  snprintf (tmp2, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  fd = mkstemp(tmp2);
  close (fd);
  fileptr = fopen (tmp2, "w");
  oldout = state->out;
  state->out = fileptr;
  err = lu_uncomment (state, &uncomment_options);
  state->out = oldout;
  fclose (fileptr);

  detect_licenses (state, options, tmp2);
  remove (tmpext);
  free (tmpext);
  remove (tmp2);
  return err;
}

int 
lu_detect (struct lu_state_t *state, struct lu_detect_options_t *options)
{
  int err = 0;
  if (strcmp (options->input_file, "-") == 0)
    err = detect_stdin (state, options);
  else
    err = detect_uncommented_boilerplate (state, options);
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
