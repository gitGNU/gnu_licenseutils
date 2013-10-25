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
#include <png.h>
#include "licensing_priv.h"
#include "png-boilerplate.h"
#include "gettext-more.h"
#include "xvasprintf.h"

static struct argp_option argp_options[] = 
{
    {"remove", 'r', NULL, 0, N_("remove the comment in FILE")},
    {"no-backup", 'n', NULL, 0, N_("don't save .bak files when removing boilerplate")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_png_boilerplate_options_t *opt = NULL;
  if (state)
    opt = (struct lu_png_boilerplate_options_t*) state->input;
  switch (key)
    {
    case 'n':
      opt->no_backups = 1;
      break;
    case 'r':
      opt->remove = 1;
      break;
    case ARGP_KEY_ARG:
        argz_add (&opt->input_files, &opt->input_files_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->remove = 0;
      opt->no_backups = 0;
      opt->input_files = NULL;
      opt->input_files_len = 0;
      break;
    case ARGP_KEY_FINI:
      if (opt->no_backups && !opt->remove)
        {
          argp_failure (state, 0, 0, 
                        N_("--no-backup can only be used with --remove"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}
#undef NEW_BOILERPLATE_DOC
#define NEW_BOILERPLATE_DOC N_("Show or remove the comment in a png file.") "\v" N_("")
static struct argp argp = { argp_options, parse_opt, "FILE", NEW_BOILERPLATE_DOC};

int 
lu_png_boilerplate_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_png_boilerplate_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_png_boilerplate (state, &opts);
  else
    return err;
}

static char *
get_comment (FILE *fp, char *f)
{
  char *c = NULL;

  unsigned char signature[8];
  memset (signature, 0, sizeof (signature));
  if (fread (signature, 1, 8, fp) != 8)
    {
      fprintf(stderr, "%s: `%s' is not a PNG file\n", png_boilerplate.name, f);
      return NULL;
    }

  if (png_sig_cmp (signature, 0, 8) != 0)
    {
      fprintf(stderr, "%s: `%s' is not a PNG file\n", png_boilerplate.name, f);
      return NULL;
    }
  rewind (fp);
  png_structp read_ptr;
  read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_init_io (read_ptr, fp);

  png_textp text_ptr;
  int num_text = 0;
  png_infop read_info_ptr;
  read_info_ptr = png_create_info_struct (read_ptr);
  png_read_info (read_ptr, read_info_ptr);
  if (png_get_text (read_ptr, read_info_ptr, &text_ptr, &num_text) > 0)
    {
      png_text *comment = text_ptr;
      for (int i = 0; i < num_text; i++)
        {
          if (strcmp (comment->key, "Comment") == 0)
            {
              if (comment->text && strlen (comment->text) > 0)
                c = strdup (comment->text);
              break;
            }
          comment++;
        }
    }
  png_destroy_read_struct (&read_ptr, &read_info_ptr, NULL);
  return c;
}

static void 
nowarn (png_structp png, const char *msg)
{
  return;
}
static void
remove_comment (FILE *fp, FILE *out)
{
  //setup out file
  png_bytepp row_pointers = NULL;
  int i, rowbytes;
  png_structp outpng = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, nowarn);
  setjmp (png_jmpbuf (outpng));
  png_init_io (outpng, out);
  png_set_expand (outpng);

  //setup in file
  png_structp inpng = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop ininfo = png_create_info_struct (inpng);
  png_init_io (inpng, fp);

  //get data from in file
  unsigned char *raster = NULL;
  int channels = 0;
  unsigned int width = 0;
  unsigned int height = 0;
  int bitdepth = 0;
  int colourtype = 0;
  png_set_sig_bytes(inpng, 8);
  png_read_info (inpng, ininfo);
  png_get_IHDR (inpng, ininfo, &width, &height, &bitdepth, &colourtype, NULL, 
               NULL, NULL);

  if(bitdepth < 8)
    png_set_packing (inpng);

  if (colourtype == PNG_COLOR_TYPE_PALETTE)
    png_set_expand (inpng);

  //remove the comment!
  png_textp text_ptr;
  int num_text = 0;
  if (png_get_text (inpng, ininfo, &text_ptr, &num_text) > 0)
    {
      png_text *comment = text_ptr;
      for (int i = 0; i < num_text; i++)
        {
          if (strcmp (comment->key, "Comment") == 0)
            {
              if (comment->text)
                {
                  strcpy (comment->text, "\n");
                  comment->text_length = 1;
                  comment->itxt_length = 1;
                  break;
                }
            }
          comment++;
        }
    
      png_set_text (inpng, ininfo, text_ptr, num_text);
    }

  png_read_update_info (inpng, ininfo);
  channels = png_get_channels (inpng, ininfo);

  rowbytes = png_get_rowbytes (inpng, ininfo);
  row_pointers = malloc (height * sizeof (png_bytep));
  raster = (unsigned char *) malloc ((rowbytes * height) + 1);

  for (i = 0; i < height; ++i)
    row_pointers[i] = raster + (i * rowbytes);
  png_read_image (inpng, row_pointers);


  //now write it all out

  png_write_info (outpng, ininfo);
  rowbytes = bitdepth / 8;
  if (bitdepth % 8 != 0)
    rowbytes++;
  rowbytes *= channels;
  rowbytes *= width;

  row_pointers = malloc (height * sizeof (png_bytep));
  for (i = 0; i < height; ++i)
    row_pointers[i] = raster + (i * rowbytes);

  png_write_image (outpng, row_pointers);

  png_write_end (outpng, ininfo);
  png_destroy_write_struct (&outpng, NULL);
  png_destroy_read_struct (&inpng, &ininfo, NULL);
}

static void
lu_remove_comment (struct lu_state_t *state, struct lu_png_boilerplate_options_t *options, char *f)
{
  unsigned char signature[8];
  char tmp[sizeof(PACKAGE) + 13];
  snprintf (tmp, sizeof tmp, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp(tmp);
  close (fd);
  FILE *fp = fopen (f, "rb");
  if (fp)
    {
      memset (signature, 0, sizeof (signature));
      if (fread (signature, 1, 8, fp) != 8)
        {
          fprintf(stderr, "%s: `%s' is not a PNG file\n", png_boilerplate.name,
                  f);
          return;
        }
      if (png_sig_cmp (signature, 0, 8) != 0)
        {
          fprintf(stderr, "%s: `%s' is not a PNG file\n", png_boilerplate.name, f);
          return;
        }
    }
  FILE *out = fopen (tmp, "wb");
  if (fp && out)
    remove_comment (fp, out);
  fclose (out);
  fclose (fp);
  if (options->no_backups == 0)
    {
      char *new_filename = xasprintf ("%s.bak", f);
      rename (f, new_filename);
      rename (tmp, f);
    }
  else
    {
      remove (f);
      rename (tmp, f);
    }
}

int 
lu_png_boilerplate (struct lu_state_t *state, struct lu_png_boilerplate_options_t *options)
{
  char *f = NULL;
  while ((f = argz_next (options->input_files, options->input_files_len, f)))
    {
      if (is_a_file (f) == 0)
        {
          if (errno == EISDIR)
            fprintf (stderr, N_("%s: %s: %s\n"),
                     png_boilerplate.name, f, strerror (errno));
          else
            fprintf (stderr, N_("%s: could not open `%s' for reading: %s\n"),
                     png_boilerplate.name, f, strerror (errno));
          continue;
        }
      if (options->remove == 0)
        {
          char *comment = NULL;
          FILE *fp = fopen(f, "rb");
          if (fp)
            {
              comment = get_comment (fp, f);
              fclose (fp);
              if (comment)
                {
                  luprintf (state, "%s\n", comment);
                  free (comment);
                }
            }
        }
      else
        {
          if (access (f, W_OK) != 0)
            {
              fprintf (stderr, 
                       N_("%s: could not open `%s' for writing: %s\n"),
                       png_boilerplate.name, f, strerror (errno));
              continue;
            }
          lu_remove_comment (state, options, f);
        }
    }
  return 0;
}

struct lu_command_t png_boilerplate = 
{
  .name         = N_("png-boilerplate"),
  .doc          = NEW_BOILERPLATE_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_png_boilerplate_parse_argp
};
