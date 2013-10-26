/*  Copyright (C) 2013 Ben Asselstine

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied copyright of
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
#include <time.h>
#include <string.h>
#include <argz.h>
#include "licensing_priv.h"
#include "copyright.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "read-file.h"
#include "trim.h"

static struct argp_option argp_options[] = 
{
    {"abbreviate-years", 'a', NULL, 0, 
      N_("abbreviate the years if possible")},
    {"unicode-sign", 'u', NULL, 0, 
      N_("use © instead of (C)")},
    {"small-sign", 'c', NULL, 0, N_("use (c) instead of (C)")},
    {"one-per-line", '1', NULL, 0, 
      N_("don't separate names with commas")},
    {"dry-run", 'd', NULL, 0, N_("do not modify the current working boilerplate")},
    {"remove", 'r', "LINENO", OPTION_ARG_OPTIONAL, 
      N_("remove copyright from the working boilerplate")},
    {"quiet", 'q', NULL, 0, N_("don't show diagnostic messages")},
    {0}
};

static int
get_current_year()
{
  struct tm tm;
  time_t now  = time(NULL);
  localtime_r (&now, &tm);
  return tm.tm_year + 1900;
}

static int 
parse_yearspec (char *yearspec, int *years)
{
  int err = 0;
  int dash = 0;
  char *ptr = yearspec;
  int prev_year = 0;
  while (*ptr)
    {
      char *next = NULL;
      unsigned long int year = strtoul (ptr, &next, 10);
      if (dash && year <= 9)
        year = (prev_year / 10 * 10) + year;
      else if (dash && year <= 99)
        year = (prev_year / 100 * 100) + year;
      if ((int)year < 1900 || year > get_current_year() || next == ptr)
        {
          err = -1;
          break;
        }
      if (dash)
        {
          for (int i = prev_year; i <= year; i++)
            years[i-1900] = 1;
        }
      else
        years[year-1900] = 1;
      prev_year = year;
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

static void
display_copyright()
{
  char *file = get_config_file ("copyright-holders");
  if (file)
    {
      FILE *fp = fopen (file, "r");
      if (fp)
        {
          size_t data_len = 0;
          char *data = fread_file (fp, &data_len);
          fclose (fp);
          fprintf (stdout, "%s", data);
          free (data);
        }
      free (file);
    }
}

static int
remove_copyright_line (int line, struct lu_copyright_options_t *options)
{
  int err = 0;
  char *data = NULL;
  char *file = get_config_file ("copyright-holders");
  if (file)
    {
      FILE *fp = fopen (file, "r");
      if (fp)
        {
          size_t data_len = 0;
          data = fread_file (fp, &data_len);
          fclose (fp);
          char *ptr = &data[0];
          for (int i = 0; i < line; i++)
            {
              ptr = strchr (++ptr, '\n');
              if (ptr == NULL)
                break;
            }
          if (ptr)
            {
              char *start = ptr;
              if (*start == '\n')
                start++;
              if (*start != '\0')
                {
                  ptr = strchr (++ptr, '\n');
                  if (ptr)
                    {
                      ptr++;
                      memmove (start, ptr, strlen (ptr) + 1);
                    }
                  else
                    *start = '\0';
                }
              else
                err = -1;
            }
          else
            err = -1;
        }
      else
        err = -1;
      free (file);
    }
  if (!err && data)
    {
      char *file = get_config_file ("copyright-holders");
      if (file)
        {
          FILE *fp = fopen (file, "w");
          if (fp)
            {
              fprintf (fp, "%s", data);
              fclose (fp);
            }
          free (file);
          if (options->quiet == 0)
            {
              display_copyright();
              fprintf (stderr, "Removed.\n");
            }
        }
      free (data);
    }
  return err;
}

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_copyright_options_t *opt = NULL;
  if (state)
    opt = (struct lu_copyright_options_t*) state->input;
  switch (key)
    {
    case 'q':
      opt->quiet = 1;
      break;
    case 'r':
        {
          if (arg)
            {
              char *end = NULL;
              unsigned long int lineno = strtoul (arg, &end, 10);
              if (end == NULL || *end != '\0' || (int)lineno < 0)
                {
                  argp_failure (state, 0, 0, 
                                N_("`%s' is an invalid line number"), arg);
                  argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
                }
              else
                {
                  int err = remove_copyright_line (lineno, opt);
                  if (err)
                    {
                      argp_failure (state, 0, 0, 
                                    N_("`%s' is an invalid line number"), arg);
                      argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
                    }
                  exit (err);
                }
            }
          else
            {
              char *file = get_config_file ("copyright-holders");
              remove (file);
              free (file);
              if (opt->quiet == 0)
                fprintf (stderr, "Removed.\n");
            }
        }
      break;
    case 'd':
      opt->dry_run = 1;
      break;
    case '1':
      opt->one_per_line = 1;
      break;
    case 'u':
      opt->unicode_sign = 1;
      break;
    case 'c':
      opt->small_c = 1;
      break;
    case 'a':
      opt->abbreviate_years = 1;
      break;
    case ARGP_KEY_ARG:
      if (strlen (arg) >= 4 && strspn (arg, "0123456789-,") == strlen (arg))
        {
          if (parse_yearspec (arg, opt->years) == 0)
            opt->yearspec = arg;
          else
            {
              argp_failure (state, 0, 0, N_("Malformed yearspec"));
              argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
            }
        }
      else
        argz_add (&opt->name, &opt->name_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->abbreviate_years = 0;
      opt->yearspec = NULL;
      opt->unicode_sign = 0;
      opt->small_c = 0;
      opt->one_per_line = 0;
      memset (opt->years, '\0', sizeof (opt->years));
      opt->name = NULL;
      opt->name_len = 0;
      opt->dry_run = 0;
      opt->quiet = 0;
      break;
    case ARGP_KEY_FINI:
      if (opt->yearspec == NULL)
        opt->years[get_current_year()-1900]=1;
      if (opt->name)
        argz_stringify (opt->name, opt->name_len, ' ');
      break;
    case ARGP_KEY_NO_ARGS:
      display_copyright();
      exit (0);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


#undef COPYRIGHT_DOC
#define COPYRIGHT_DOC \
  N_("Add or modify copyright notices in the working boilerplate.") "\v"\
  N_("If NAME is not specified, a series of names are read from standard input.")\
  "  " N_("If YEARSPEC is not specified, the current year is assumed.")\
  "  " N_("If both YEARSPEC and NAME are not given, the copyright holders in the current working boilerplate are displayed.") "  "\
  N_("LINENO begins counting at zero.")

static struct argp argp = { argp_options, parse_opt, "[YEARSPEC...] [NAME...]", 
  COPYRIGHT_DOC};

int 
lu_copyright_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_copyright_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_copyright (state, &opts);
  else
    return err;
}

static int 
add_copyright_years (char **argz, size_t *len, struct lu_copyright_options_t *options)
{
  for (int i = 0; i < COPYRIGHT_MAX_YEARS; i++)
    {
      if (options->years[i])
        {
          char *year = xasprintf (" %d", i + 1900);
          argz_add (argz, len, year);
          free (year);
        }
    }
  return 0;
}

static int 
add_copyright_years_abbreviated (char **argz, size_t *len, struct lu_copyright_options_t *options)
{
  int i = 0;
  while (i < COPYRIGHT_MAX_YEARS)
    {
      while (options->years[i] == 0 && i < COPYRIGHT_MAX_YEARS)
        i++;
      if (i < COPYRIGHT_MAX_YEARS)
        {
          int start = i;
          while (options->years[i] == 1 && i < COPYRIGHT_MAX_YEARS)
            i++;
          int end = i-1;
          if (start == end)
            {
              char *year = xasprintf (" %d", start + 1900);
              argz_add (argz, len, year);
              free (year);
            }
          else
            {
              char *yearpair = xasprintf (" %d-%d", start + 1900, end + 1900);
              argz_add (argz, len, yearpair);
              free (yearpair);
            }
        }
    }
  return 0;
}

static void
get_names (struct lu_state_t *state, char **names, size_t *names_len)
{
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  while ((read = getline (&line, &len, stdin)) != -1) 
    {
      char *trimmed_name = trim (line);
      if (strlen (trimmed_name) > 0)
        {
          argz_add (names, names_len, trimmed_name);
          free (trimmed_name);
        }
      else
        if (*names_len)
          break;
    }

  free(line);
}

int 
lu_copyright (struct lu_state_t *state, struct lu_copyright_options_t *options)
{
  char *argz = NULL;
  size_t len = 0;
  char *names = NULL;
  size_t names_len = 0;
  char *file = NULL;
  FILE *fp = stdout;
  if (!options->dry_run)
    {
      file = get_config_file ("copyright-holders");
      if (!file)
        return 0;
      fp = fopen (file, "a");
      if (!fp)
        return 0;
    }

  if (options->name == NULL)
    get_names (state, &names, &names_len);
  else
    argz_add (&names, &names_len, options->name);

  if (options->abbreviate_years == 0)
    add_copyright_years (&argz, &len, options);
  else
    add_copyright_years_abbreviated (&argz, &len, options);

  argz_stringify(argz, len, ',');

  //how many of the names can we fit on a line before reaching say 76 columns?
  int max_width = 76;
  char *c;
  if (options->unicode_sign)
    c = xasprintf ("Copyright ©%s", argz);
  else
    c = xasprintf ("Copyright (%c)%s", options->small_c ? 'c' : 'C', argz);
  char *name  = NULL;
  char *line = strdup (c);
  int remaining = max_width - strlen (line);
  int first = 1;
  while ((name = argz_next (names, names_len, name)))
    {
      int length = strlen (name)+1;
      if (remaining > length && options->one_per_line == 0)
        {
          char *l = xasprintf ("%s%s %s", line, first ? "" : ",",name);
          free (line);
          line = l;
          remaining -= length + first;
          first = 0;
        }
      else 
        {
          if (first)
            {
              char *l = xasprintf ("%s %s", line, name);
              free (line);
              line = l;
              fprintf (fp, "%s\n", line);
              free (line);
              line = strdup (c);
              remaining = max_width - strlen (line);
            }
          else
            {
              fprintf (fp, "%s\n", line);
              free (line);
              line = xasprintf ("%s %s", c, name);
              remaining = max_width - strlen (line);
              first = 0;
            }
        }
    }
  if (strcmp (c, line) != 0)
    fprintf (fp, "%s\n", line);

  free (names);
  free (argz);
  if (!options->dry_run)
    {
      fclose (fp);
      free (file);
      if (options->quiet == 0)
        {
          display_copyright();
          fprintf (stderr, "Added.\n");
        }
    }
  return 0;
}

struct lu_command_t copyright = 
{
  .name         = N_("copyright"),
  .doc          = COPYRIGHT_DOC,
  .flags        = SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_copyright_parse_argp
};
