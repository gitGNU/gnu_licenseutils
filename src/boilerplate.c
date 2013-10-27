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
#include <glib.h>
#include <sys/stat.h>
#include "licensing_priv.h"
#include "boilerplate.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "trim.h"
#include "util.h"
#include "styles.h"

enum
{
  OPT_UNCOMMENT = -211,
  OPT_WHITESPACE,
  OPT_COUNT,
};

static struct argp_option argp_options[] = 
{
    {"remove", 'r', NULL, 0, 
      N_("remove boilerplate from FILE instead of showing")},
    {"force", 'f', NULL, 0, N_("force the removal copyright notices")},
    {"blocks", 'b', "LIST", 0, 
      N_("select these comment blocks to show or remove")},
    {"no-backup", 'n', NULL, 0, N_("don't save .bak files when removing boilerplate")},
    {"quiet", 'q', NULL, 0, N_("don't show diagnostic messages")},
    {0}
};

static int 
parse_blockspec (char *blockspec, int *blocks)
{
  int err = 0;
  int dash = 0;
  char *ptr = blockspec;
  int prev_block = 0;
  while (*ptr)
    {
      char *next = NULL;
      unsigned long int block = strtoul (ptr, &next, 10);
      if ((int)block <= 0 || block > 1024 || next == ptr)
        {
          err = -1;
          break;
        }
      if (dash)
        {
          for (int i = prev_block; i <= block; i++)
            blocks[i] = 1;
        }
      else
        blocks[block] = 1;
      prev_block = block;
      if (*next == '-')
        dash = 1;
      else if (*next == ',')
        dash = 0;
      else if (*next == '\0')
        break;
      else
        {
          err = -2;
          break;
        }
        
      ptr = ++next;
    }
  return err;
}

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_boilerplate_options_t *opt = NULL;
  if (state)
    opt = (struct lu_boilerplate_options_t*) state->input;
  switch (key)
    {
    case 'q':
      opt->quiet = 1;
      break;
    case 'n':
      opt->no_backups = 1;
      break;
    case 'b':
      if (parse_blockspec (arg, opt->blocks) == 0)
        opt->blockspec = arg;
      else
        {
          argp_failure (state, 0, 0, N_("Malformed blockspec"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    case 'r':
      opt->remove = 1;
      break;
    case 'f':
      opt->force = 1;
      break;
    case ARGP_KEY_ARG:
      argz_add (&opt->input_files, &opt->input_files_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->input_files = NULL;
      opt->input_files_len = 0;
      opt->remove = 0;
      opt->force = 0;
      opt->blockspec = NULL;
      opt->no_backups = 0;
      opt->quiet = 0;
      memset (opt->blocks, '\0', sizeof (opt->blocks));
      opt->style = NULL;
      state->child_inputs[0] = &opt->style;
      break;
    case ARGP_KEY_FINI:
      if (opt->force && !opt->remove)
        {
          argp_failure (state, 0, 0, 
                        N_("--force can only be used with --remove"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      else if (opt->no_backups && !opt->remove)
        {
          argp_failure (state, 0, 0, 
                        N_("--no-backup can only be used with --remove"));
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
    { &styles_argp, 0, N_("Commenting Style Options:"), 0 },
    { 0 }
};
#undef BOILERPLATE_DOC
#define BOILERPLATE_DOC \
  N_("Show or remove the boilerplate text at the top of a file.") "\v"\
  N_("Comment style is auto-detected if a style option is not provided.") "  "\
  N_("With no FILE, or when FILE is -, read from standard input.") "  "\
  N_("Modified files are backed-up into files with a .bak suffix.") "  "\
  N_("LIST is a comma separated set of numbers, indicating comment blocks (starts counting at 1.)")
static struct argp argp = { argp_options, parse_opt, "FILE...", 
  BOILERPLATE_DOC, parsers };

int 
lu_boilerplate_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  struct lu_boilerplate_options_t opts;
  opts.state = state;

  int err = argp_parse (&argp, argc, argv, state->argp_flags, 0, &opts);
  if (!err)
    return lu_boilerplate (state, &opts);
  else
    return err;
}

static void        
remove_blocks_in_blocklist (char **comment_blocks, size_t *len, int *blocks, int val)
{
  int idx = 1;
  char *c = NULL;
  while ((c = argz_next (*comment_blocks, *len, c)))
    {
      if (blocks[idx] == val)
        {
          argz_delete (comment_blocks, len, c);
          if (len == 0)
            break;
          c--; 
        }
      idx++;
      if (idx >= MAX_COMMENT_BLOCKS)
        break;
    }
}

static size_t
get_max_block (int *blocks)
{
  size_t max = 0;
  for (size_t i = 1; i < MAX_COMMENT_BLOCKS; i++)
    {
      if (blocks[i] > 0)
        max = i;
    }
  return max;
}

static int
show_lu_boilerplate (struct lu_state_t *state, struct lu_boilerplate_options_t *options, char *file)
{
  char *comment_blocks = NULL;
  size_t len = 0;
  FILE *fp = fopen (file, "r");
  if (!fp)
    return 0;
  if (options->style == NULL)
    auto_detect_comment_blocks (file, fp, &comment_blocks, &len, NULL);
  else
    options->style->get_initial_comment (fp, &comment_blocks, &len, NULL);
  fclose (fp);
  if (comment_blocks)
    {
      if (options->blockspec && 
          get_max_block (options->blocks) > argz_count (comment_blocks, len))
        {
          luprintf (state, _("%s: invalid block id %d\n"), 
                     boilerplate.name, get_max_block (options->blocks));
        }
      else
        {
          if (options->blockspec)
            remove_blocks_in_blocklist (&comment_blocks, &len, 
                                        options->blocks, 0);
          if (comment_blocks)
            {
              argz_stringify (comment_blocks, len, '\n');
              luprintf (state, "%s", comment_blocks);
              if (comment_blocks[strlen (comment_blocks)-1] != '\n')
                luprintf (state, "\n");
              free (comment_blocks);
            }
        }
    }
  else
    {
      if (options->quiet == 0)
        luprintf (state, _("%s: no boilerplate found in `%s'\n"), 
                  boilerplate.name, file);
    }
  return 0;
}

static int
comments_contain_copyright_notice (char *argz, size_t len)
{
  char *c = NULL;
  while ((c = argz_next (argz, len, c)))
    if (g_regex_match_simple ("[Cc]opyright.*(19[0-9][0-9]|20[0-9][0-9])", c, 
                              G_REGEX_CASELESS, 0))
      return 1;
  return 0;
}

static int
remove_lu_boilerplate (struct lu_state_t *state, struct lu_boilerplate_options_t *options, char *filename, int from_stdout)
{
  struct stat st;
  char *comment_blocks = NULL;
  size_t len = 0;
  int err = 0;
  FILE *fp = fopen (filename, "r");
  if (!fp)
    return err;
  fstat (fileno (fp), &st);
  char *hashbang = NULL;
  if (options->style == NULL)
    auto_detect_comment_blocks (filename, fp, &comment_blocks, &len, &hashbang);
  else
    options->style->get_initial_comment (fp, &comment_blocks, &len, &hashbang);
  if (comment_blocks == NULL)
    {
      luprintf (state, _("%s: no boilerplate found in `%s'\n"), 
                 boilerplate.name, filename);
      return err;
    }
  if (comments_contain_copyright_notice (comment_blocks, len))
    {
      if (options->force == 0)
        {
          luprintf (state, _("%s: `%s' contains copyright notices.  use --force to remove them.'\n"), 
                     boilerplate.name, filename);
          free (comment_blocks);
          return 0;
        }
    }
  if (options->blockspec && 
      get_max_block (options->blocks) > argz_count (comment_blocks, len))
    {
      luprintf (state, _("%s: invalid block id %d\n"), 
                 boilerplate.name, get_max_block (options->blocks));
      free (comment_blocks);
      return 0;
    }
  //okay now we're going to remove the comments specified in the blockspec.
  if (options->blockspec)
    remove_blocks_in_blocklist (&comment_blocks, &len, options->blocks, 1);
  else
    {
      // remove them all ralphie boy
      free (comment_blocks);
      comment_blocks = NULL;
      len = 0;
    }


  char *swpfilename = xasprintf ("%s.swp", filename);
  char *bakfilename = xasprintf ("%s.bak", filename);
  FILE *out;
  if (from_stdout)
    out = stdout;
  else
    out = fopen (swpfilename, "w");
  if (out)
    {
      if (hashbang)
        {
          fprintf (out, "%s", hashbang);
          free (hashbang);
        }
      if (comment_blocks)
        {
          char *c = NULL;
          while ((c = argz_next (comment_blocks, len, c)))
            fprintf (out, "%s\n", c);
        }
      char *line = NULL;
      size_t linelen = 0;
      if (!feof (fp))
        {
          ssize_t read = getline (&line, &linelen, fp);
          if (read != -1 && strspn (line, "\n\r\t\v ") != strlen (line))
            fprintf (out, "%s", line);
          if (read != -1)
            while ((read = getline (&line, &linelen, fp)) != -1) 
              fprintf (out, "%s", line);
        }
      free(line);
      if (!from_stdout)
        fclose (out);
      err = chmod (swpfilename, st.st_mode);
      rename (filename, bakfilename);
      rename (swpfilename, filename);
      if (from_stdout || options->no_backups)
        remove (bakfilename);
    }
  else
    err = -1;
  free (swpfilename);
  free (bakfilename);
  free (comment_blocks);
  fclose (fp);
  return err;
}

static int
boilerplate_from_stdin (struct lu_state_t *state, struct lu_boilerplate_options_t *options)
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
      if (options->remove)
        err = remove_lu_boilerplate (state, options, tmp, 1);
      else
        err = show_lu_boilerplate (state, options, tmp);
      remove (tmp);
    }
  else
    err = -1;
  return err;
}

static int
show_lu_boilerplate_for_files(struct lu_state_t *state, struct lu_boilerplate_options_t *options)
{
  int err = 0;
  char *f = NULL;
  while ((f = argz_next (options->input_files, options->input_files_len, f)))
    {
      if (strcmp (f, "-") == 0)
        err = boilerplate_from_stdin (state, options);
      else
        {
          if (is_a_file (f) == 0)
            {
              if (options->quiet == 0)
                {
                  if (errno == EISDIR)
                    fprintf (stderr, N_("%s: %s: %s\n"),
                             boilerplate.name, f, strerror (errno));
                  else
                    fprintf (stderr, 
                             N_("%s: could not open `%s' for reading: %s\n"),
                             boilerplate.name, f, strerror (errno));
                }
              continue;
            }
          if (options->remove)
            {
              if (access (f, W_OK) != 0)
                {
                  if (options->quiet == 0)
                    fprintf (stderr, 
                             N_("%s: could not open `%s' for writing: %s\n"),
                             boilerplate.name, f, strerror (errno));

                  continue;
                }
              err = remove_lu_boilerplate (state, options, f, 0);
            }
          else
            err = show_lu_boilerplate (state, options, f);
        }
      if (err)
        break;
    }
  return err;
}

int 
lu_boilerplate (struct lu_state_t *state, struct lu_boilerplate_options_t *options)
{
  int err = 0;
  if (options->input_files == NULL)
    err = boilerplate_from_stdin (state, options);
  else
    err = show_lu_boilerplate_for_files (state, options);
  free (options->input_files);
  return err;
}

struct lu_command_t boilerplate = 
{
  .name         = N_("boilerplate"),
  .doc          = BOILERPLATE_DOC,
  .flags        = SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_boilerplate_parse_argp
};
