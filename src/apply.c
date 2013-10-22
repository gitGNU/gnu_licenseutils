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
#include <argz.h>
#include "licensing_priv.h"
#include "apply.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "util.h"
#include "prepend.h"
#include "styles.h"

static struct argp_option argp_options[] = 
{
    {"no-backup", 'n', NULL, 0, 
      N_("don't retain original source file in a .bak file")},
    {"quiet", 'q', NULL, 0, N_("don't show diagnostic messages")},
    {"after", 'a', NULL, 0,
      N_("prepend after existing boilerplate if any")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_apply_options_t *opt = NULL;
  if (state)
    opt = (struct lu_apply_options_t*) state->input;
  switch (key)
    {
    case 'a':
      opt->after = 1;
      break;
    case 'q':
      opt->quiet = 1;
      break;
    case 'n':
      opt->backup = 0;
      break;
    case ARGP_KEY_ARG:
      argz_add (&opt->input_files, &opt->input_files_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->input_files = NULL;
      opt->input_files_len = 0;
      opt->backup = 1;
      opt->quiet = 0;
      opt->style = NULL;
      opt->after = 0;
      state->child_inputs[0] = &opt->style;
      break;
    case ARGP_KEY_FINI:
      if (opt->input_files == NULL)
        {
          argp_failure (state, 0, 0, N_("no files specified"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      if (opt->style && opt->after == 0)
        {
          argp_failure (state, 0, 0, N_("did you mean to use --after?"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static struct argp_child parsers[]=
{
    { &styles_argp, 0, N_("Commenting Style Options (for use with --after):"), 0 },
    { 0 }
};
#undef APPLY_DOC
#define APPLY_DOC N_("Prepend the current working boilerplate to a file.")
static struct argp argp = { argp_options, parse_opt, "FILE...", APPLY_DOC,
parsers};

int 
lu_apply_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_apply_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_apply (state, &opts);
  else
    return err;
}

int 
lu_apply (struct lu_state_t *state, struct lu_apply_options_t *options)
{
  int err = 0;
  if (!can_apply(apply.name))
    return -1;

  char boilerplate[sizeof(PACKAGE) + 13];
  snprintf (boilerplate, sizeof boilerplate, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp (boilerplate);
  close (fd);
  char *generate_cmd = xasprintf ("%s/preview > %s", PKGLIBEXECDIR, 
                                  boilerplate);
  system (generate_cmd);

  char *f = NULL;
  while ((f = argz_next (options->input_files, options->input_files_len, f)))
    {
      if (is_a_file (f) == 0)
        {
          if (errno == EISDIR)
            fprintf (stderr, N_("%s: %s: %s\n"),
                     apply.name, f, strerror (errno));
          else
            fprintf (stderr, N_("%s: could not open `%s' for reading: %s\n"),
                     apply.name, f, strerror (errno));
          continue;
        }
      else
        {
          if (access (f, W_OK) != 0)
            {
              fprintf (stderr, N_("%s: could not open `%s' for writing: %s\n"),
                       apply.name, f, strerror (errno));
              continue;
            }
        }
      
      struct lu_prepend_options_t prepend_options;
      prepend_options.backup = options->backup;
      prepend_options.after = options->after;
      prepend_options.style = options->style;
      prepend_options.source = boilerplate;
      prepend_options.dest = f;
      err = lu_prepend (state, &prepend_options);
      if (!err)
        {
          if (options->quiet == 0)
            fprintf (stderr, "%s: %s -> Boilerplate applied.\n", apply.name, f);
        }
      if (err)
        break;
    }
  remove (boilerplate);
  return err;
}

struct lu_command_t apply = 
{
  .name         = N_("apply"),
  .doc          = APPLY_DOC,
  .flags        = SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_apply_parse_argp
};
