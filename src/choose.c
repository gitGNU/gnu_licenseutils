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
#include "choose.h"
#include "gettext-more.h"
#include "xvasprintf.h"
#include "opts.h"
#include "comment.h"
#include "read-file.h"
#include "trim.h"
#include "error.h"
#include "copy-file.h"
#include "styles.h"

static struct argp_option argp_options[] = 
{
    { "force", 'f', NULL, 0, 
      N_("force the selection of an unrecommended license") },
    {"quiet", 'q', NULL, 0, N_("don't show diagnostic messages")},
    { 0 }
};

static int lookup (char *arg, char **match);
static int needs_force (char *arg);
static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  char *match = NULL;
  struct lu_choose_options_t *opt = NULL;
  if (state)
    opt = (struct lu_choose_options_t*) state->input;
  switch (key)
    {
    case 'q':
      opt->quiet = 1;
      break;
    case 'f':
      opt->force = 1;
      break;
    case ARGP_KEY_ARG:
      if (lookup (arg, &match))
        {
          if (needs_force (arg) == 0 || opt->force)
            argz_add (&opt->licenses, &opt->licenses_len, match);
          else
            argp_failure (state, 22, 0, 
                          N_("`%s' is not recommended!  "
                             "Use --force, "
                             "or maybe try `%s+'"), arg, arg);
          free (match);
        }
      else
        {
          argp_failure (state, 0, 0, 
                        N_("Unknown license or comment style `%s'"), arg);
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    case ARGP_KEY_INIT:
      opt->force = 0;
      opt->licenses = NULL;
      opt->licenses_len = 0;
      opt->quiet = 0;
      break;
    case ARGP_KEY_FINI:
      if (opt->licenses == NULL)
        {
          argp_failure (state, 0, 0, 
                        N_("No license or comment style specified"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static char *
help_filter (int key, const char *text, void *input)
{
  if (key == ARGP_KEY_HELP_PRE_DOC)
    {
      char *argz = NULL;
      size_t len = 0;
      if (text)
        {
          argz_add (&argz, &len, text);
          argz_add (&argz, &len, "");
        }
      char *licenses = lu_list_of_license_keywords ();
      char *l = xasprintf (N_("Supported Licenses: %s no-license"), licenses);
      argz_add (&argz, &len, l);
      free (l);
      free (licenses);
      argz_add (&argz, &len, "");
      char *styles = lu_list_of_comment_styles();
      l = xasprintf ("Supported Comment Styles: %s no-style", styles);
      argz_add (&argz, &len, l);
      free (l);
      free (styles);
      argz_add (&argz, &len, "");
      argz_add (&argz, &len, "Options:");
      argz_stringify (argz, len, '\n');
      return argz;
    }
  return (char *) text;
}
#undef CHOOSE_DOC
#define CHOOSE_DOC \
  N_("Pick license and comment style for the working boilerplate.")
static struct argp argp = { argp_options, parse_opt, "[LICENSE...] [COMMENT-STYLE]", 
  CHOOSE_DOC, 0, help_filter};

int 
lu_choose_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_choose_options_t opts;
  opts.state = state;
  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_choose (state, &opts);
  else
    return err;
}

static char *
get_command (char *argz, size_t len, char *license)
{
  char *l = NULL;
  while ((l = argz_next (argz, len, l)))
    {
      char *match = NULL;
      if (sscanf (l, "%ms", &match) == 1)
        {
          if (strcmp (license, match) == 0)
            {
              free (match);
              char *space = strchr (l, ' ');
              space++;
              while (*space == ' ' || *space == '\t')
                space++;
              return space;
            }
          free (match);
        }
    }
  return NULL;
}

static int 
needs_force (char *arg)
{
  /* check to see if --jerkward is used in this license */
  int need = 0;
  char *cmds = lu_list_of_license_keyword_commands();
  char *license_commands = NULL;
  size_t len = 0;
  argz_create_sep (cmds, '\n', &license_commands, &len);
  free (cmds);
  char *cmd = get_command(license_commands, len, arg);
  if (cmd && strstr (cmd, "--jerkwad"))
    need = 1;
  free (license_commands);
  return need;
}


static char *
get_license_by_command (char *argz, size_t len, char *command)
{
  char *l = NULL;
  while ((l = argz_next (argz, len, l)))
    {
      char *match = NULL;
      char *license = NULL;
      if (sscanf (l, "%ms %ms", &license, &match) == 2)
        {
          if (strcmp (command, match) == 0)
            {
              free (match);
              return license;
            }
          free (match);
          free (license);
        }
    }
  return NULL;
}

static int 
lookup (char *arg, char **match)
{
  int valid = 1;
  char *cmds = lu_list_of_license_keyword_commands();
  char *license_commands = NULL;
  size_t len = 0;
  argz_create_sep (cmds, '\n', &license_commands, &len);
  free (cmds);

  char *cmd = get_command (license_commands, len, arg);
  if (!cmd)
    {
      char *license = get_license_by_command (license_commands, len, arg);
      if (!license && !lu_is_a_comment_style (arg))
        valid = 0;
      if (license)
        *match = license;
      else
        *match = strdup (arg);
    }
  else
    *match = strdup (arg);

  if (strcasecmp (arg, "no-style") == 0)
    {
      *match = strdup (arg);
      valid = 1;
    }
  else if (strcasecmp (arg, "no-license") == 0)
    {
      *match = strdup (arg);
      valid = 1;
    }

  free (license_commands);
  return valid;
}

static char *
load_selected_comment_style ()
{
  char *data = NULL;
  char *f = get_config_file ("selected-comment-style");
  if (f)
    {
      FILE *fp = fopen (f, "r");
      if (fp)
        {
          size_t data_len = 0;
          data = fread_file (fp, &data_len);
          fclose (fp);
        }
      free (f);
    }
  return data;
}

static void
write_selected_comment_style (char *style)
{
  char *f = get_config_file ("selected-comment-style");
  if (f)
    {
      if (strcasecmp (style, "no-style") == 0)
        remove (f);
      else
        {
          FILE *fp = fopen (f, "w");
          if (fp)
            {
              fprintf (fp, "%s", style ? style : "");
              fclose (fp);
            }
        }
      free (f);
    }
}

static char *
load_selected_licenses ()
{
  char *data = NULL;
  char *f = get_config_file ("selected-licenses");
  if (f)
    {
      FILE *fp = fopen (f, "r");
      if (fp)
        {
          size_t data_len = 0;
          data = fread_file (fp, &data_len);
          fclose (fp);
        }
      free (f);
    }
  return data;
}

static int
write_selected_licenses (struct lu_state_t *state, struct lu_choose_options_t *options)
{
  int err = 0;
  if (options->licenses == NULL)
    {
      char *f = get_config_file ("selected-licenses");
      remove (f);
      f = get_config_file ("license-notice");
      remove (f);
      return err;
    }
  char *licenses = options->licenses;
  char *f = get_config_file ("license-notice");
  if (f)
    {
      char tmp[sizeof(PACKAGE) + 13];
      snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
      int fd = mkstemp(tmp);
      close (fd);
      FILE *fp = fopen (tmp, "w");
      if (fp)
        {
          char *cmds = lu_list_of_license_keyword_commands();
          char *license_commands = NULL;
          size_t license_len = 0;
          argz_create_sep (cmds, '\n', &license_commands, &license_len);
          free (cmds);
          char *argz = NULL;
          size_t argz_len = 0;
          argz_create_sep (options->licenses, ' ', &argz, &argz_len);
          char *l = NULL;
          size_t count = argz_count (argz, argz_len);
          while ((l = argz_next (argz, argz_len, l)))
            {
              char *cmd = get_command (license_commands, license_len, l);
              FILE *old_out = state->out;
              state->out = fp;
              err = lu_parse_command (state, cmd);
              if (err)
                {
                  error (0, 0, N_("failed to select license `%s' "
                                  "(network down or missing webpage?"), l);
                  state->out = old_out;
                  break;
                }
              if (count > 1)
                {
                  fprintf (fp, "\n");
                  fprintf (fp, "---\n");
                  fprintf (fp, "\n");
                }
              state->out = old_out;
              count--;
            }
          free (argz);
          fclose (fp);
          free (license_commands);
          //install it
          if (!err)
            {
              err = qcopy_file_preserving (tmp, f);
              if (err)
                error (0, errno, N_("could not copy %s -> %s"), tmp, f);
            }
        }
      free (f);
    }
  if (!err)
    {
      f = get_config_file ("selected-licenses");
      if (f)
        {
          FILE *fp = fopen (f, "w");
          if (fp)
            {
              fprintf (fp, "%s", licenses ? licenses: "");
              fclose (fp);
            }
          free (f);
        }
    }
  return err;
}

int 
lu_choose (struct lu_state_t *state, struct lu_choose_options_t *options)
{
  int err = 0;
  char *old_style = load_selected_comment_style ();
  char *old_license = load_selected_licenses ();
  char *l = NULL;
  char *style = NULL;
  int no_license = 0;
  while ((l = argz_next (options->licenses, options->licenses_len, l)))
    {
      if (lu_is_a_comment_style (l) || strcasecmp (l, "no-style") == 0)
        {
          style = strdup (l);
          argz_delete (&options->licenses, &options->licenses_len, l);
          l = NULL;
          if (options->licenses == NULL)
            break;
          continue;
        }
      if (strcasecmp (l, "no-license") == 0)
        no_license = 1;
    }
  if (no_license)
    {
      free (options->licenses);
      options->licenses = NULL;
      options->licenses_len = 0;
    }
  if (options->licenses)
    argz_stringify (options->licenses, options->licenses_len, ' ');
  else if (no_license)
    ;
  else
    {
      if (old_license)
        options->licenses = strdup (old_license);
    }
  struct lu_comment_options_t comment_options;
  memset (&comment_options, 0, sizeof (comment_options)); 
  if (!style)
    {
      if (old_style)
        style = strdup (old_style);
    }
  int found = 0;
  if (style)
    {
      comment_options.style = lu_lookup_comment_style (style);
      if (comment_options.style)
        found = 1;
    }
  char *p = NULL;

  if (options->licenses)
    {
      if (found)
        p = lu_create_comment (state, &comment_options, options->licenses);
      else if (style && strcasecmp (style, "no-style") == 0)
        ;
      else
        p = strdup (options->licenses);
      if (p)
        {
          char *nl = strchr (p, '\n');
          if (nl)
            nl[0]='\0';
        }
    }
  if (style)
    write_selected_comment_style (style);
  if (options->licenses)
    {
      if (old_license && strcmp (old_license, options->licenses) == 0)
        ;
      else
        err = write_selected_licenses (state, options);
    }
  else
    err = write_selected_licenses (state, options);
  free (p);
  free (style);
  free (options->licenses);
  free (old_style);
  free (old_license);
  if (options->quiet == 0 && !err)
    fprintf (stderr, "Selected.\n");
  return 0;
}

struct lu_command_t choose = 
{
  .name         = N_("choose"),
  .doc          = CHOOSE_DOC,
  .flags        = SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_choose_parse_argp
};
