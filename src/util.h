/*  Copyright (C) 2013, 2014 Ben Asselstine

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
#ifndef LU_UTIL_H
#define LU_UTIL_H 1

#include <stdio.h>
#include "licensing.h"
#include "comment-style.h"
int show_lines_after (struct lu_state_t *state, char *text, const char *match, int lines, int replace_flag, char *search, char *replace);
int is_a_file_where_hash_includes_are_not_comments (char *filename);
void replace_html_entities (char *text);
int can_apply(char *progname);
char * get_comment_by_regex (char *data, const char *expr);
char * create_block_comment (char *text, char *open_delimiter, char *close_delimiter);
void uncomment_comments (char **argz, size_t *len, char *delimiters, char *synonymous_delimiter, int whitespace, int first_literal, int second_literal);
char * create_line_comment (char *text, char *delimiter);
void get_hashbang_or_rewind (FILE *fp, char **hashbang);
int get_comment_blocks (FILE *fp, char **argz, size_t *len, char **hashbang, char *regex);
char * get_comments_and_whitespace (FILE *fp, char *file, struct lu_comment_style_t *style);
char * get_lines (char *text, const char *match, int lines);
int text_replace (char *text, char *search, char *replace);
void replace_fsf_address (char **chunk, int fsf_address, char *license, int num_spaces);
#endif
