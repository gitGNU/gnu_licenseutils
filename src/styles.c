/*  Copyright (C) 2013 Ben Asselstine

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
#include <argz.h>
#include <argp.h>
#include <stdlib.h>
#include "styles.h"
#include "gettext-more.h"
#include "licensing_priv.h"
#include "c-ctype.h"
#include "read-file.h"

#ifdef SUPPORT_C_STYLE
#include "c-style.h"
#endif
#ifdef SUPPORT_CPLUSPLUS_STYLE
#include "c++-style.h"
#endif
#ifdef SUPPORT_FORTRAN_STYLE
#include "fortran-style.h"
#endif
#ifdef SUPPORT_GETTEXT_STYLE
#include "gettext-style.h"
#endif
#ifdef SUPPORT_GROFF_STYLE
#include "groff-style.h"
#endif
#ifdef SUPPORT_HASKELL_STYLE
#include "haskell-style.h"
#endif
#ifdef SUPPORT_M4_STYLE
#include "m4-style.h"
#endif
#ifdef SUPPORT_PASCAL_STYLE
#include "pascal-style.h"
#endif
#ifdef SUPPORT_SCHEME_STYLE
#include "scheme-style.h"
#endif
#ifdef SUPPORT_SHELL_STYLE
#include "shell-style.h"
#endif
#ifdef SUPPORT_TEXINFO_STYLE
#include "texinfo-style.h"
#endif

struct lu_comment_style_t* lu_styles[]=
{
  /* okay folks, listen up. 
     here's a reason these aren't alphabetically listed.
     here the ordering of these styles matters for auto-detection. */
#ifdef SUPPORT_C_STYLE
    &c_style,
#endif
#ifdef SUPPORT_CPLUSPLUS_STYLE
    &cplusplus_style,
#endif
#ifdef SUPPORT_SHELL_STYLE
    &shell_style,
#endif
#ifdef SUPPORT_SCHEME_STYLE
    &scheme_style,
#endif
#ifdef SUPPORT_TEXINFO_STYLE
    &texinfo_style,
#endif
#ifdef SUPPORT_M4_STYLE
    &m4_style,
#endif
#ifdef SUPPORT_HASKELL_STYLE
    &haskell_style,
#endif
#ifdef SUPPORT_GROFF_STYLE
    &groff_style,
#endif
#ifdef SUPPORT_GETTEXT_STYLE
    &gettext_style,
#endif
#ifdef SUPPORT_FORTRAN_STYLE
    &fortran_style,
#endif
#ifdef SUPPORT_PASCAL_STYLE
    &pascal_style,
#endif
    NULL,
};

static const struct argp_child
parsers[] =
{
#ifdef SUPPORT_C_STYLE
    {&c_style_argp},
#endif
#ifdef SUPPORT_CPLUSPLUS_STYLE
    {&cplusplus_style_argp},
#endif
#ifdef SUPPORT_FORTRAN_STYLE
    {&fortran_style_argp},
#endif
#ifdef SUPPORT_GETTEXT_STYLE
    {&gettext_style_argp},
#endif
#ifdef SUPPORT_GROFF_STYLE
    {&groff_style_argp},
#endif
#ifdef SUPPORT_HASKELL_STYLE
    {&haskell_style_argp},
#endif
#ifdef SUPPORT_M4_STYLE
    {&m4_style_argp},
#endif
#ifdef SUPPORT_PASCAL_STYLE
    {&pascal_style_argp},
#endif
#ifdef SUPPORT_SCHEME_STYLE
    {&scheme_style_argp},
#endif
#ifdef SUPPORT_SHELL_STYLE
    {&shell_style_argp},
#endif
#ifdef SUPPORT_TEXINFO_STYLE
    {&texinfo_style_argp},
#endif
    { 0 },

};

enum 
{
  OPT_STYLE = -110,
  OPT_VIEW_ALL,
};
static struct argp_option argp_options[] = 
{
    {"style", OPT_STYLE, "NAME", OPTION_HIDDEN, N_("specify style by name")},
    {"help-all-styles", OPT_VIEW_ALL, NULL, 0, N_("show more commenting style options and exit")},
    {0}
};

char *
lu_list_of_comment_styles()
{
  char *argz = NULL;
  size_t len = 0;
  struct lu_comment_style_t **style = &lu_styles[0];
  while (*style)
    {
      argz_add (&argz, &len, (*style)->name);
      style++;
    }
  argz_stringify (argz, len, ' ');
  return argz;
}

static int 
match_file_extension (char *extensions, char *filename)
{
  int found = 0;
  char *ext = strrchr (filename, '.');
  if (!ext)
    return found;
  char *argz = NULL;
  size_t len = 0;
  argz_create_sep (extensions, ' ', &argz, &len);
  char *e = NULL;
  while ((e = argz_next (argz, len, e)))
    {
      if (strcasecmp (ext, e) == 0)
        {
          found = 1;
          break;
        }
    }
  free (argz);
  return found;
}

struct lu_comment_style_t *
auto_detect_comment_blocks (char *filename, FILE *fp, char **argz, size_t *len, char **hashbang)
{
  char *hashbang_throwaway = NULL;
  char **h = hashbang;
  if (!h)
    h = &hashbang_throwaway;
  struct lu_comment_style_t **style = &lu_styles[0];
  while (*style)
    {
      rewind (fp);
      int match = 0;
      if ((*style)->support_file_exts)
        {
          if (match_file_extension ((*style)->support_file_exts, filename))
            match = 1;
        }
      else
        match = 1;

      if ((*style)->avoid_file_exts && 
          match_file_extension ((*style)->avoid_file_exts, filename))
        match = 0;

      if (match && (*style)->get_initial_comment (fp, argz, len, h))
        break;

      style++;
    }
  if (hashbang_throwaway)
    free (hashbang_throwaway);
  return (*style);
}

struct lu_comment_style_t *
lu_lookup_comment_style (char *arg)
{
  struct lu_comment_style_t **style = &lu_styles[0];
  while (*style)
    {
      if (strcasecmp ((*style)->name, arg) == 0)
        return *style;
      style++;
    }
  return NULL;
}

struct lu_comment_style_t *
get_default_commenting_style()
{
  return lu_lookup_comment_style ("c");
}

int
lu_is_a_comment_style (char *arg)
{
  return lu_lookup_comment_style (arg) == NULL ? 0 : 1;
}

      
static void
show_all_comment_style_options()
{
  struct lu_comment_style_t **style = &lu_styles[0];
  printf (" Commenting Style Options:\n");
  while (*style)
    {
      const struct argp_option *options = &(*style)->argp->options[0];
      while (options->key)
        {
          char opt[2];
          opt[0]=options->key;
          opt[1]='\0';
          printf ("  %s%s%s    %s%-20s %s\n", 
                  isalnum(options->key) ? "-" : " ",
                  isalnum(options->key) ? opt : " ",
                  isalnum(options->key) ? "," : " ",
                  options->name ? "--" : "  ", options->name, options->doc);
          options++;
        }
      style++;
    }
}

struct lu_comment_style_t *
lu_get_current_commenting_style()
{
  struct lu_comment_style_t *style = NULL;
  char *f = get_config_file ("selected-comment-style");
  if (f)
    {
      FILE *fp = fopen (f, "r");
      if (fp)
        {
          size_t data_len = 0;
          char *data = fread_file (fp, &data_len);
          if (data)
            {
              style = lu_lookup_comment_style (data);
              free (data);
            }
          fclose (fp);
        }
      free (f);
    }
  return style;
}

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_comment_style_t **style = NULL;
  if (state)
    style = (struct lu_comment_style_t **) state->input;
  switch (key)
    {
    case OPT_VIEW_ALL:
      show_all_comment_style_options();
      exit (0);
      break;
    case OPT_STYLE:
      if (lu_is_a_comment_style (arg))
        *style = lu_lookup_comment_style (arg);
      else
        {
          argp_failure (state, 0, 0, N_("Unknown comment style `%s'"), arg);
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    case ARGP_KEY_INIT:
        {
          int i = 0;
          struct lu_comment_style_t **s = &lu_styles[0];
          while (*s)
            {
              state->child_inputs[i] = style;
              i++;
              s++;
            }
        }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp styles_argp = { argp_options, parse_opt, "", "", parsers};
