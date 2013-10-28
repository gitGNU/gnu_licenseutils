/*  Copyright (C) 2011, 2013 Ben Asselstine

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
#include <string.h>
#include <argz.h>
#include <argp.h>
#include <unistd.h>
#include <curl/curl.h>
#include <dirent.h>
#include <sys/stat.h>
#include "licensing_priv.h"
#include "gettext-more.h"
#include "trim.h"
#include "xvasprintf.h"
#include "progname.h"

#include "opts.h"
#include "gpl.h"
#include "lgpl.h"
#include "agpl.h"
#include "fdl.h"
#include "boilerplate.h"
#include "png-boilerplate.h"
#include "png-apply.h"
#include "warranty.h"
#include "help.h"
#include "welcome.h"
#include "copyright.h"
#include "cbb.h"
#include "comment.h"
#include "uncomment.h"
#include "choose.h"
#include "top.h"
#include "project.h"
#include "extra.h"
#include "apply.h"
#include "all-permissive.h"
#include "bsd.h"
#include "mit.h"
#include "apache.h"
#include "isc.h"
#include "styles.h"
#include "prepend.h"

enum 
{
  NOTICE = 0, GPL, LGPL, AGPL, FDL, BOILERPLATE, HELP, WARRANTY, WELCOME, 
  COPYRIGHT, CBB, COMMENT, UNCOMMENT, PREPEND, CHOOSE, TOP, PROJECT,
  PREVIEW, APPLY, NEW_BOILERPLATE, ALL_PERMISSIVE, BSD, APACHE, MIT, 
  EXTRA, PNG_BOILERPLATE, PNG_APPLY, ISC, THE_END
};

struct lu_command_t new_boilerplate = 
{
  .name         = N_("new-boilerplate"),
  .doc          = 
    N_("Clear the current working boilerplate."),
  .flags        = DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = NULL,
  .parser       = NULL 
};

struct lu_command_t preview = 
{
  .name         = N_("preview"),
  .doc          = 
    N_("Show the current working boilerplate."),
  .flags        = SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = NULL,
  .parser       = NULL 
};

struct lu_command_t notice = 
{
  .name         = N_("notice"),
  .doc          = 
    N_("A simple script to write license notices to files."),
  .flags        = SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = NULL,
  .parser       = NULL 
};

struct lu_command_t* lu_commands[]=
{
  [GPL]             = &gpl,
  [LGPL]            = &lgpl,
  [AGPL]            = &agpl,
  [FDL]             = &fdl,
  [BOILERPLATE]     = &boilerplate,
  [HELP]            = &help,
  [WARRANTY]        = &warranty,
  [WELCOME]         = &welcome,
  [COPYRIGHT]       = &copyright,
  [COMMENT]         = &comment,
  [UNCOMMENT]       = &uncomment,
  [CBB]             = &cbb,
  [PREPEND]         = &prepend,
  [CHOOSE]          = &choose,
  [TOP]             = &top,
  [PROJECT]         = &project,
  [PREVIEW]         = &preview,
  [APPLY]           = &apply,
  [NEW_BOILERPLATE] = &new_boilerplate,
  [PNG_BOILERPLATE] = &png_boilerplate,
  [PNG_APPLY]       = &png_apply,
  [ALL_PERMISSIVE]  = &all_permissive,
  [BSD]             = &bsd,
  [MIT]             = &mit,
  [APACHE]          = &apache,
  [EXTRA]           = &extra,
  [ISC]             = &isc,
  [NOTICE]          = &notice,
  [THE_END]     = NULL
};

static struct lu_command_t *get_command (char *line)
{
  char *cmd = NULL;
  struct lu_command_t *command = NULL;
  if (sscanf (line, "%ms", &cmd) == 1)
    {
      for (int i = 0; i < THE_END; i++)
        {
          if (strcmp (cmd, lu_commands[i]->name) == 0)
            {
              command = lu_commands[i];
              break;
            }
        }
      free (cmd);
    }
  return command;
}

static void 
make_command_line (char *cmd, int *argc, char ***argv)
{
  char *s1 = strdup (cmd);
  if (s1)
    {
      char *term;
      *argv = NULL;
      *argc = 0;
      for (term = strtok (s1, " \t"); term != NULL; term = strtok (NULL, " \t"))
        {
          *argv = (char **) realloc (*argv, sizeof (char *) * ((*argc) + 1));
          if (*argv)
            (*argv)[*argc] = strdup (term);
          (*argc)++;
        }
      *argv = (char **) realloc (*argv, sizeof (char *) * ((*argc) + 1));
      if (*argv)
        (*argv)[(*argc)] = 0;
      free (s1);

      char *bash_env = getenv("BASH_ENV");
      if (bash_env && strstr (bash_env, ".lushrc"))
        set_program_name (strdup((*argv)[0]));
      else
        set_program_name (xasprintf ("%s %s", PROGRAM, (*argv)[0]));
      free ((*argv)[0]);
      (*argv)[0] = (char*) program_name;
    }
}

int
lu_parse_command (struct lu_state_t *state, char *line)
{
  int err = 0;

  if (strcmp (line, "\n") == 0 || strcmp (line, "\r\n") == 0 || *line == 0)
    return 0;

  state->command = line;
  struct lu_command_t *cmd = get_command (line);
  if (cmd)
    {
      int argc = 0;
      char **argv = NULL;
      make_command_line (line, &argc, &argv);
      if (cmd->parser)
        err = cmd->parser(state, argc, argv);
      else
        {
          char *cmd = xasprintf ("%s/%s", PKGLIBEXECDIR, line);
          system (cmd);
        }
      for (int i = 0; i < argc; i++)
        free (argv[i]);
      free (argv);
    }
  else
    {
      char *cmd = NULL;
      sscanf (state->command, "%ms", &cmd);
      luprintf (state, _("%s: unrecognized command `%s'\n"), PROGRAM, cmd);
      fprintf (stderr, "Try '%s help' for more information.\n", PROGRAM);
      free (cmd);
    }
  
  return err;
}

static int
lu_parse_untrimmed_command (struct lu_state_t *state, char *line)
{
  int err = 0;
  if (!line)
    {
      fprintf (state->out, "\n");
      return 0;
    }
  if (strcmp (line, "\n") == 0 || strcmp (line, "\r\n") == 0 || *line == 0)
    return 0;
  char *trimmed_line = trim (line);
  if (trimmed_line[0] != '#') //it's not a commented out line
    err = lu_parse_command (state, trimmed_line);
  free (trimmed_line);
  return err;
}

struct lu_state_t *
lu_init (struct lu_options_t *arguments)
{
  struct lu_state_t *state;
  state = malloc(sizeof(struct lu_state_t));
  if (!state)
    return NULL;
  memset (state, 0, sizeof (struct lu_state_t));

  curl_global_init (CURL_GLOBAL_ALL);
  state->curl = curl_easy_init();

  state->out = stdout;

  char *dir = xasprintf ("%s/.%s", getenv("HOME"), PACKAGE);
  DIR *d = opendir (dir);
  if (d)
    closedir (d);
  else
    mkdir (dir, 0755);
  free (dir);

  return state;
}

char * 
get_config_file (char *file)
{
  return xasprintf ("%s/.%s/%s", getenv ("HOME"), PACKAGE, file);
}

void
lu_destroy(struct lu_state_t *state)
{
  curl_easy_cleanup(state->curl);
  curl_global_cleanup();
  free (state);
}

int 
licensing (struct lu_options_t *arguments)
{
  int run_lush = 0;
  int err = 0;
  struct lu_state_t *state = lu_init (arguments);
  if (!state)
    return -1;

  if (arguments->command_on_argv)
    err = lu_parse_untrimmed_command (state, arguments->command_on_argv);
  else
    run_lush = 1;

  int quiet = arguments->quiet;
  lu_destroy(state);
  if (run_lush)
    {
      if (is_a_file (INTERPRETER_PATH))
        {
          if (quiet)
            execl (INTERPRETER_PATH, INTERPRETER, "", (char*)0);
          else
            execl (INTERPRETER_PATH, INTERPRETER, (char*)0);
        }
      else
        {
          fprintf (stderr, "%s: Could not run %s (%s)\n", PROGRAM, 
                   INTERPRETER_PATH, strerror (errno));
          err = 1;
        }
    }
  return err;
}

char *
lu_list_of_license_keyword_commands()
{
  char *argz = NULL;
  size_t len = 0;
  for (int i = 0; i < THE_END; i++)
    {
      if ((lu_commands[i]->flags & IS_A_LICENSE) != 0)
        {
          char **license = lu_commands[i]->licenses;
          while (*license)
            {
              argz_add (&argz, &len, *license);
              license++;
            }
        }
    }
  argz_stringify (argz, len, '\n');
  return argz;
}

char *
lu_list_of_license_keywords()
{
  char *argz = NULL;
  size_t len = 0;
  for (int i = 0; i < THE_END; i++)
    {
      if ((lu_commands[i]->flags & IS_A_LICENSE) != 0)
        {
          char **license = lu_commands[i]->licenses;
          while (*license)
            {
              char *keyword = NULL;
              char *cmd = NULL;
              if (sscanf (*license, "%ms %ms", &keyword, &cmd) == 2)
                {
                  //first license of every set is an abbreviation
                  if (*license == *lu_commands[i]->licenses)
                    {
                      if (strcmp (cmd, keyword) != 0)
                        argz_add (&argz, &len, cmd);
                    }
                  argz_add (&argz, &len, keyword);
                  free (keyword);
                  free (cmd);
                }
              license++;
            }
        }
    }
  argz_stringify (argz, len, ' ');
  return argz;
}

char *
lu_list_of_commands_for_help (int control)
{
  char *argz = NULL;
  size_t len = 0;
  for (int i = 0; i < THE_END; i++)
    {
      int show = 0;
      switch (control)
        {
        case 0: //show commands that are SHOW_IN_HELP
          if ((lu_commands[i]->flags & SHOW_IN_HELP) != 0)
            show = 1;
          break;
        case 1: //show 'em all
          show = 1;
          break;
        case 2: //show commands that are not IS_A_LICENSE
          if ((lu_commands[i]->flags & IS_A_LICENSE) == 0)
            show = 1;
          break;
        case 3: //show commands that are IS_A_LICENSE
          if ((lu_commands[i]->flags & IS_A_LICENSE) != 0)
            show = 1;
          break;
        }
      if (!show)
        continue;
      char *doc = NULL;
      if (lu_commands[i]->doc)
        {
          doc = strdup (lu_commands[i]->doc);
          char *vert_sep = strchr (doc, '\v');
          if (vert_sep)
            vert_sep[0] = '\0';
        }
      char *cmd = xasprintf ("  %-15s %s", lu_commands[i]->name, 
                             doc ? doc : "");
      free (doc);
      argz_add (&argz, &len, cmd);
      free (cmd);
    }
  argz_stringify (argz, len, '\n');
  return argz;
}

void 
lu_generate_bashrc_file (FILE *fp)
{

  char *choose_keywords = lu_list_of_license_keywords();
  char *comment_style_keywords = lu_list_of_comment_styles();
  fprintf (fp, "\
_licensing_options()\n\
{\n\
    local cur prev opts\n\
    COMPREPLY=()\n\
    cur=\"${COMP_WORDS[COMP_CWORD]}\"\n\
    prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n\
    command=`echo ${COMP_LINE} | cut -f1 -d' '`\n\
    moreopts=\"\"\n\
    if [[ \"$command\" == choose ]]; then\n\
      moreopts=\"%s no-license %s no-style\"\n\
    elif [[ \"$command\" == help ]]; then\n\
      moreopts=`licensing help | grep \"^  [a-z]\" | cut -f3 -d' ' | tr  '\n,' ' '`\n\
    fi\n\
\n\
    if [[ \"${cur}\" == -* ]] ; then\n\
        opts=`licensing help $command | egrep  \"(^  -., |^      --)\" | cut -c7-29 | sed -e 's/=[A-Z]* //g' | tr  '\\n,' ' ' | tr -s \" \"`\n\
	COMPREPLY=($(compgen -W \"${opts}\" -- \"${cur}\"))\n\
    elif [[ \"${moreopts}\" != \"\" ]] ; then\n\
	COMPREPLY=($(compgen -W \"${moreopts}\" -- \"${cur}\"))\n\
    elif [[ \"${cur}\" == * ]] ; then\n\
        COMPREPLY=($(compgen -A file -- \"${cur}\"))\n\
	return 0\n\
    fi\n\
\n\
    return 0\n\
}\n\n", choose_keywords, comment_style_keywords);
  free (choose_keywords);
  free (comment_style_keywords);

  for (int i = 0; i < THE_END; i++)
    {
      fprintf (fp, "alias %s=\"%s %s\"\n", lu_commands[i]->name, PROGRAM, 
               lu_commands[i]->name);
      fprintf (fp, "complete -F _licensing_options %s\n", lu_commands[i]->name);
    }
  fprintf (fp, "\
_create_prompt() {\n\
  p=\"%s\"\n\
  if [[ -f ~/.%s/selected-licenses && -f ~/.%s/selected-comment-style ]]; then\n\
    style=`eval cat ~/.%s/selected-comment-style`\n\
    p=`cat ~/.%s/selected-licenses | %s comment --style=$style`\n\
  elif [[ -f ~/.%s/selected-licenses ]]; then\n\
    p=`cat ~/.%s/selected-licenses`\n\
  elif [[ -f ~/.%s/selected-comment-style ]]; then\n\
    style=`eval cat ~/.%s/selected-comment-style`\n\
    p=`echo \"no-license\" | %s comment --style=$style`\n\
  fi\n\
  echo \"$p> \"\n\
}\n\n", PROGRAM, PACKAGE, PACKAGE, PACKAGE, 
   PACKAGE, PROGRAM, PACKAGE, PACKAGE, PACKAGE, PACKAGE, PROGRAM);

  fprintf (fp, "if [ -f ~/.bashrc ]; then\n");
  fprintf (fp, "  source ~/.bashrc\n");
  fprintf (fp, "fi\n");
  fprintf (fp, "export HISTFILE=~/.%s-history\n", INTERPRETER);
  fprintf (fp, "shopt -s expand_aliases\n");
  fprintf (fp, "export PS1='$(_create_prompt)'\n");
  return;
}

int 
luprintf (struct lu_state_t *state, char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  int r = vfprintf (state->out, fmt, ap);
  va_end(ap);
  return r;
}

int
is_a_file (char *filename)
{
  FILE *fp = fopen (filename, "r");
  if (!fp)
    return 0;
  struct stat st;
  int retval = fstat (fileno (fp), &st);
  fclose (fp);
  if (retval != 0)
    return 0;
   
  if (S_ISREG (st.st_mode) == 0)
    {
      errno = EISDIR;
      return 0;
    }
  return 1;
}
