/*  Copyright (C) 2011, 2014 Ben Asselstine

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
#ifndef LU_LICENSING_H
#define LU_LICENSING_H 1

#include <glib.h>
#include <curl/curl.h>
#include <argp.h>

// types
struct lu_options_t
{
  int quiet;
  char *command_on_argv; //run a single command and exit
  size_t command_on_argv_len;
};

struct lu_state_t
{
  int argp_flags; //use ARGP_NO_EXIT here when calling from library.
  CURL *curl;
  FILE *out;
  int columns;
  char *command; //current command we're working on.
};

//the main loop
int licensing (struct lu_options_t *opts);

//alternate usage, without a main loop
struct lu_state_t * lu_init(struct lu_options_t *arguments);
void lu_destroy(struct lu_state_t *state);

enum lu_command_flags_t
{
  NONE = 0,
  DO_NOT_SHOW_IN_HELP = 0,
  DO_NOT_SAVE_IN_HISTORY = 0,
  SHOW_IN_HELP,
  SAVE_IN_HISTORY,
  IS_A_LICENSE=4,
  IS_A_COMMENT_STYLE=8,
};

struct lu_command_t
{
  char *name;
  char *doc;
  int flags;
  const struct argp *argp;
  int (*parser) (struct lu_state_t *, int, char **);
  char *licenses[];
};
char *lu_list_of_license_keyword_commands();
char * lu_list_of_license_keywords();
char * lu_dump_command_to_file (struct lu_state_t *state, char *command);
#define GNU_SITE             "www.gnu.org" //no prefix, no slashes.
#endif
