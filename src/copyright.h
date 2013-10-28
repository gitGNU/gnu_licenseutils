/*  Copyright (C) 2013 Ben Asselstine

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY COPYRIGHT; without even the implied copyright of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/
#ifndef LU_COPYRIGHT_H
#define LU_COPYRIGHT_H 1

#include <config.h>
#include <argp.h>
#include "licensing.h"

#define COPYRIGHT_MAX_YEARS 1000
struct lu_copyright_options_t
{
  struct lu_state_t *state;
  int abbreviate_years;
  char *yearspec;
  int unicode_sign;
  int small_c;
  int one_per_line;
  int years[COPYRIGHT_MAX_YEARS]; //from 1900
  char *name;
  size_t name_len;
  int dry_run;
  int quiet;
  char *remove;
  size_t remove_len;
  int remove_all;
  int accept_year_args;
};

int lu_copyright_parse_argp (struct lu_state_t *, int argc, char **argv);
int lu_copyright (struct lu_state_t *, struct lu_copyright_options_t *);
extern struct lu_command_t copyright;
#endif
