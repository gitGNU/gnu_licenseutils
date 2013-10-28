/*  Copyright (C) 2011 Ben Asselstine

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
#ifndef GNU_PRIV_H
#define GNU_PRIV_H 1

#include <argp.h>

#include "licensing.h"

int argp_help_check (int argc, char **argv);
int luprintf (struct lu_state_t *state, char *fmt, ...);
int lu_parse_command (struct lu_state_t *state, char *line);
char *lu_list_of_commands_for_help(int show_all);
void lu_generate_bashrc_file (FILE *fp);
char * get_config_file (char *file);
int is_a_file (char *filename);
int lu_is_command (char *line);
#endif
