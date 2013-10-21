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
#include <sys/stat.h>
#include "licensing_priv.h"
#include "prepend.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "util.h"
#include "read-file.h"

static struct argp_option argp_options[] = 
{
    {"no-backup", 'n', NULL, 0, 
      N_("don't retain original source file in a .bak file")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_prepend_options_t *opt = NULL;
  if (state)
    opt = (struct lu_prepend_options_t*) state->input;
  switch (key)
    {
    case 'n':
      opt->backup = 0;
      break;
    case ARGP_KEY_ARG:
      if (opt->dest)
        {
          opt->source = opt->dest;
          opt->dest = arg;
        }
      else
        opt->dest = arg;
      break;
    case ARGP_KEY_INIT:
      opt->source = NULL;
      opt->dest = 0;
      opt->backup = 1;
      break;
    case ARGP_KEY_FINI:
      if (opt->dest == NULL)
        {
          argp_failure (state, 0, 0, N_("no files specified"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


#undef PREPEND_DOC
#define PREPEND_DOC N_("Prepend SOURCE to the beginning DEST.  With no SOURCE, or if it is \"-\", read from the standard input.\vSOURCE is prepended after #! when it is present as a beginning line in DEST.")
static struct argp argp = { argp_options, parse_opt, "SOURCE DEST\nDEST", PREPEND_DOC};

int 
lu_prepend_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_prepend_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_prepend (state, &opts);
  else
    return err;
}

int 
lu_prepend (struct lu_state_t *state, struct lu_prepend_options_t *options)
{
  struct stat st;
  int err = 0;
  FILE *src = NULL;
  FILE *dst = NULL;
  memset (&st, 0, sizeof (st));
  if (strcmp (options->source, "-") == 0 || options->source == NULL)
    src = stdin;
  else if (is_a_file (options->source) != 0)
    src = fopen (options->source, "r");
  else
    {
      if (errno == EISDIR)
        fprintf (stderr, N_("%s: %s: %s\n"),
                 prepend.name, options->source, strerror (errno));
      else
        fprintf (stderr, N_("%s: could not open `%s' for reading: %s\n"),
                 prepend.name, options->source, strerror (errno));
      return 1;
    }
  size_t source_len = 0;
  char *source = NULL;

  if (src)
    source = fread_file (src, &source_len);

  if (src != stdin && src != NULL)
    fclose (src);
  
  size_t dest_len = 0;
  char *dest = NULL;
  if (is_a_file (options->dest) != 0)
    {
      dst = fopen (options->dest, "r");
      if (dst)
        dest = fread_file (dst, &dest_len);
    }
  else
    {
      if (errno == EISDIR)
        fprintf (stderr, N_("%s: %s: %s\n"),
                 prepend.name, options->dest, strerror (errno));
      else
        fprintf (stderr, N_("%s: could not open `%s' for reading: %s\n"),
                 prepend.name, options->dest, strerror (errno));
      fclose (dst);
      return 1;
    }

  char *hashbang = NULL;
  if (dst)
    {
      rewind (dst);
      get_hashbang_or_rewind (dst, &hashbang);
      if (hashbang)
        {
          memmove (dest, dest + strlen (hashbang), 
                   strlen (dest) - strlen (hashbang) + 1);
        }
      fstat (fileno (dst), &st);
      fclose (dst);
    }

  //okay we have source, and we have dest, and we have hashbang.
  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp (tmp);
  close (fd);
  FILE *out = fopen (tmp, "w");
  if (out)
    {
      if (hashbang)
        fprintf (out, "%s", hashbang);
      fprintf (out, "%s", source);
      fprintf (out, "%s", dest);
      fflush (out);
      fsync (fileno (out));
      fclose (out);

      err = chmod (tmp, st.st_mode);
      if (options->backup)
        {
          char *backup = xasprintf ("%s.bak", options->dest);
          rename (options->dest, backup);
          free (backup);
        }
      else
        remove (options->dest);

      char *cmd = xasprintf ("mv %s %s", tmp, options->dest);
      system (cmd);
      free (cmd);
    }
  free (source);
  free (dest);
  free (hashbang);
  return err;
}

struct lu_command_t prepend = 
{
  .name         = N_("prepend"),
  .doc          = N_("Put one file onto the start of another."),
  .flags        = SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_prepend_parse_argp
};
