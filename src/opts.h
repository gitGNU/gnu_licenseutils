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
#ifndef LU_OPTS_H
#define LU_OPTS_H 1

#include "licensing.h"
// the options
enum app_command_line_options_t
{
  OPT_BASH = -511,
  OPT_QUIET = 'q',
};

struct arguments_t 
{
  struct lu_options_t lu;
  char *licenses;
  size_t licenses_len;
};

// external prototypes
void parse_opts (int argc, char **argv, struct arguments_t *arguments);

#define DEFAULT_QUIET_VALUE "0"
#define DEFAULT_PROXY_PORT 8080

#endif
