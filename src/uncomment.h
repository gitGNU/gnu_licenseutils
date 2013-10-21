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
#ifndef LU_UNCOMMENT_H
#define LU_UNCOMMENT_H 1

#include <config.h>
#include <argp.h>
#include "licensing.h"
#include "comment-style.h"

struct lu_uncomment_options_t
{
  struct lu_state_t *state;
  char *input_files;
  size_t input_files_len;
  int trim;
  struct lu_comment_style_t  *style;
};

int lu_uncomment_parse_argp (struct lu_state_t *, int argc, char **argv);
int lu_uncomment (struct lu_state_t *, struct lu_uncomment_options_t *);
extern struct lu_command_t uncomment;
#endif
