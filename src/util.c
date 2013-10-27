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
#include <string.h>
#include <glib.h>
#include <stdlib.h>
#include <argz.h>
#include <sys/stat.h>
#include "licensing_priv.h"
#include "util.h"
#include "trim.h"
#include "read-file.h"
#include "xvasprintf.h"
#include "styles.h"

static int
find_start_of_matching_text (char *text, const char *match)
{
  int found = 0;
  GMatchInfo *matches = NULL;
  GError *err = NULL;
  GRegex * regex = g_regex_new(match, 0, 0, &err);
  gboolean matched = g_regex_match (regex, text, 0, &matches);
  if (matched)
    {
      int n = g_match_info_get_match_count (matches);
      for (int i = 0; i < n; i++)
        {
          gint start_pos = 0, end_pos = 0;
          g_match_info_fetch_pos (matches, i, &start_pos, &end_pos);
          return start_pos;
        }
      g_match_info_free(matches);
    }
  return found;
}

int
show_lines_after (struct lu_state_t *state, char *text, const char *match, int lines, int replace_flag, char *search, char *replace)
{
  //search must be longer than replace
  int err = 0;
  int start = find_start_of_matching_text (text, match);
  if (start)
    {
      char *ptr = &text[start];
      for (int i = 0; i < lines; i++)
        ptr = strchr (++ptr, '\n');
      ptr[0] = '\0';
      if (replace_flag)
        {
          char *orlater = strstr (&text[start], search);
          if (orlater)
            {
              memmove (orlater, orlater+strlen(search), strlen(orlater+strlen(search)-1));
              memmove (orlater+strlen(replace), orlater, strlen (orlater)+1);
              memcpy (orlater, replace, strlen(replace));
            }
          luprintf (state, "%s\n", &text[start]);
        }
      else
        luprintf (state, "%s\n", &text[start]);
    }
  else
    err = -1;
  return err;
}

void
get_hashbang_or_rewind (FILE *fp, char **hashbang)
{
  if (hashbang)
    {
      char *line = NULL;
      size_t linelen = 0;
      if (getline (&line, &linelen, fp) == -1)
        return;
      if (strncmp (line, "#!", 2) == 0)
        {
          if (*hashbang)
            free (*hashbang);
          *hashbang = strdup (line);
        }
      else
        {
          free (line);
          rewind (fp);
        }
    }
}

char *
get_comment_by_regex (char *data, const char *expr)
{
  char *comment = NULL;
  GMatchInfo *matches = NULL;
  GError *err = NULL;
  GRegex * regex = 
    g_regex_new (expr, G_REGEX_MULTILINE | G_REGEX_EXTENDED, 0, &err);
  gboolean matched = g_regex_match (regex, data, 0, &matches);
  if (matched)
    {
      int n = g_match_info_get_match_count (matches);
      for (int i = 0; i < n; i++)
        {
          gint start_pos = 0, end_pos = 0;
          g_match_info_fetch_pos (matches, i, &start_pos, &end_pos);
          if (start_pos == 0)
            {
              if (strlen (g_match_info_fetch (matches, i)) > 0)
                {
                  comment = strdup (g_match_info_fetch (matches, i));
                  break;
                }
            }
        }
      g_match_info_free(matches);
    }
  return comment;
}

static int
replace_strings (char *text, char *search, char *replace)
{
  //search must be longer than replace
  char *found = strstr (text, search);
  if (found)
    {
      memmove (found, found + strlen (search), 
               strlen (found + strlen (search) - 1));
      memmove (found+strlen (replace), found, strlen (found) + 1);
      memcpy (found, replace, strlen (replace));
    }
  return found ? 1 : 0;
}

void
replace_html_entities (char *text)
{
  while (replace_strings (text, "&gt;", ">") != 0)
    ;
  while (replace_strings (text, "&lt;", "<") != 0)
    ;
  while (replace_strings (text, "&apos;", "'") != 0)
    ;
  while (replace_strings (text, "&amp;", "&") != 0)
    ;
  while (replace_strings (text, "&quot;", "\"") != 0)
    ;
}

int
can_apply(char *progname)
{
  char *file = get_config_file ("license-notice");
  if (is_a_file (file) == 0)
    {
      free (file);
      fprintf (stderr, "%s: No license chosen\n", progname);
      fprintf (stderr, "Try 'choose --help' for more information.\n");
      return 0;
    }
  free (file);
  file = get_config_file ("copyright-holders");
  if (is_a_file (file) == 0)
    {
      free (file);
      fprintf (stderr, "%s: No copyright holders specified\n", progname);
      fprintf (stderr, "Try 'copyright --help' for more information.\n");
      return 0;
    }
  free (file);
  return 1;
}

char *
create_block_comment (char *text, char *open_delimiter, char *close_delimiter)
{
  if (text == NULL)
    return xasprintf ("%s %s", open_delimiter, close_delimiter);
  char *argz = NULL;
  size_t len = 0;
  //size_t length=0;
  //size_t length = strspn (text, " \t\r\n\v");
  //text+=length;
  //add 3 spaces to the start of every line
  char *nl = strchr (text, '\n');
  //if (nl)
    {
      //but treat the first line specially. 
      if(nl)
        nl[0] = '\0';
      //char *trimmed = trim (text);
      char *line = xasprintf ("%s %s", open_delimiter, text);
      //free (trimmed);
      argz_add (&argz, &len, line);
      free (line);
      if (nl)
        text = ++nl;
    }
  while ((nl = strchr (text, '\n')))
    {
      nl[0] = '\0';
      char *line = xasprintf ("   %s", text);
      argz_add (&argz, &len, line);
      free (line);
      text = ++nl;
    }
  if (argz)
    {
      //we treat the last line specially too.
      //add the closing delimiter if it will fit.
      int lines = argz_count (argz, len);
      char *last_line = NULL;
      while (lines-- != 0)
        last_line = argz_next (argz, len, last_line);
      if (strlen (last_line) + strlen (close_delimiter) < 78)
        {
          char *l = trim_trailing (last_line);
          argz_delete (&argz, &len, last_line);
          char *line = xasprintf ("%s %s\n", l, close_delimiter);
          free (l);
          argz_add (&argz, &len, line);
          free (line);
        }
      else if (strlen (last_line) + strlen (close_delimiter) < 79)
        {
          //squeeze the space if we have to
          char *l = trim_trailing (last_line);
          argz_delete (&argz, &len, last_line);
          char *line = xasprintf ("%s%s\n", l, close_delimiter);
          free (l);
          argz_add (&argz, &len, line);
          free (line);
        }
      else
        argz_add (&argz, &len, "*/\n");

      argz_stringify (argz, len, '\n');
      return argz;
    }
  return NULL;
}

//remove delimiters from the start and ends of lines in *COMMENT.
static void
uncomment_comment (char **comment, char *delimiters, char *synonymous_delimiter, int whitespace, int first_literal, int second_literal)
{
  char *argz = NULL;
  size_t len = 0;
  char *c = *comment;
  //add a newline if necessary
  if (c[strlen (c)-1] != '\n')
    {
      char *newc = xasprintf("%s\n", c);
      free (c);
      c = newc;
    }
  else
    {
      c = strdup (*comment);
      free (*comment);
    }

  while (*c)
    {
      char *nl = strchr (c, '\n');
      if (nl)
        nl[0] = '\0';
      char *t = trim (c);
      if (t)
        {
          size_t start = 0;
          if (!first_literal)
            start = strspn (t, delimiters);
          else
            {
              if (strncmp (t, delimiters, strlen (delimiters)) == 0)
                start = strlen (delimiters);
            }
          if (start == 0 && synonymous_delimiter)
            {
              if (!second_literal)
                start = strspn (t, synonymous_delimiter);
              else
                {
                  if (strncmp (t, synonymous_delimiter, 
                               strlen (synonymous_delimiter)) == 0)
                    start = strlen (synonymous_delimiter);
                }
            }
          char *rev = strdup (t+start);
          g_strreverse (rev);
          size_t end = 0;
          if (!first_literal)
            end = strspn (rev, delimiters);
          if (!second_literal && end == 0 && synonymous_delimiter)
            end = strspn (rev, synonymous_delimiter);
          free (rev);
          t[strlen (t)-end] = '\0';
          argz_add (&argz, &len, t+start);
          free (t);
        }
      c = ++nl;
    }

  argz_stringify (argz, len, '\n');
  *comment = argz;
  if (whitespace)
    uncomment_comment (comment, " \t", NULL, 0, 0, 0);
}

void
uncomment_comments (char **argz, size_t *len, char *delimiters, char *synonymous_delimiter, int whitespace, int first_literal, int second_literal)
{
  char *new_argz = NULL;
  size_t new_len = 0;

  char *comment = NULL;
  while ((comment = argz_next (*argz, *len, comment)))
    {
      char *c = strdup(comment);
      uncomment_comment (&c, delimiters, synonymous_delimiter, whitespace, 
                         first_literal, second_literal);
      argz_add (&new_argz, &new_len, c);
      free (c);
    }
  free (*argz);
  *argz = new_argz;
  *len = new_len;
}

char *
create_line_comment (char *text, char *delimiter)
{
  if (text == NULL)
    return xasprintf ("%s ", delimiter);
  char *argz = NULL;
  size_t len = 0;
  size_t length = strspn (text, " \t\r\n\v");
  text+=length;
  char *nl;
  while ((nl = strchr (text, '\n')))
    {
      nl[0] = '\0';
      char *line = xasprintf ("%s %s", delimiter, text);
      argz_add (&argz, &len, line);
      free (line);
      text = ++nl;
    }
  if (argz)
    {
      argz_add (&argz, &len, "");
      argz_stringify (argz, len, '\n');
      return argz;
    }
  else
    return xasprintf ("%s %s", delimiter, text);
  return NULL;
}

int
get_comment_blocks (FILE *fp, char **argz, size_t *len, char **hashbang, char *regex)
{
  long startpos = ftell (fp);
  size_t data_len;
  char *data = fread_file (fp, &data_len);
  size_t length = 0;
  char *c;
  size_t whitespace = strspn (&data[length], "\n\t \v");
  if (whitespace)
    length += whitespace;
  while ((c = get_comment_by_regex (&data[length], regex)))
    {
      argz_add (argz, len, c);
      length += strlen (c);
      fseek (fp, startpos+length, SEEK_SET);
      whitespace = strspn (&data[length], "\n\t \v");
      if (whitespace)
        {
          length += whitespace;
          fseek (fp, startpos+length, SEEK_SET);
        }
    }
  free (data);
  return *len > 0;
}

char *
get_comments_and_whitespace (FILE *fp, char *file, struct lu_comment_style_t *style)
{
  long start = ftell (fp);
  char *argz = NULL;
  size_t len = 0;
  if (style == NULL)
    auto_detect_comment_blocks (file, fp, &argz, &len, NULL);
  else
    style->get_initial_comment (fp, &argz, &len, NULL);

  free (argz);
  //fp now points to after the boilerplate if there is any.
  long pos = ftell (fp);
  fseek (fp, start, SEEK_SET);
  if (len == 0)
    return NULL;
  size_t data_len;
  char *data = fread_file (fp, &data_len);
  data[pos-start]='\0';
  char *comments = strdup (data);
  free (data);
  fseek (fp, pos, SEEK_SET);

  return comments;
}
