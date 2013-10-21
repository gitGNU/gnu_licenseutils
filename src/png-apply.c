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
#include "png-apply.h"
#include "gettext-more.h"
#include "read-file.h"
#include "xvasprintf.h"
#include "util.h"

static struct argp_option argp_options[] = 
{
    {"no-backup", 'n', NULL, 0, 
      N_("don't retain original png file in a .bak file")},
    {"quiet", 'q', NULL, 0, N_("don't show diagnostic messages")},
    {0}
};

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct lu_png_apply_options_t *opt = NULL;
  if (state)
    opt = (struct lu_png_apply_options_t*) state->input;
  switch (key)
    {
    case 'q':
      opt->quiet = 1;
      break;
    case 'n':
      opt->backup = 0;
      break;
    case ARGP_KEY_ARG:
      argz_add (&opt->input_files, &opt->input_files_len, arg);
      break;
    case ARGP_KEY_INIT:
      opt->input_files = NULL;
      opt->input_files_len = 0;
      opt->backup = 1;
      opt->quiet = 0;
      break;
    case ARGP_KEY_FINI:
      if (opt->input_files == NULL)
        {
          argp_failure (state, 0, 0, N_("no files specified"));
          argp_state_help (state, stderr, ARGP_HELP_STD_ERR);
        }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


#undef PNG_APPLY_DOC
#define PNG_APPLY_DOC N_("Put the current working boilerplate into a PNG file.")
static struct argp argp = { argp_options, parse_opt, "FILE...", PNG_APPLY_DOC};

int 
lu_png_apply_parse_argp (struct lu_state_t *state, int argc, char **argv)
{
  int err = 0;
  struct lu_png_apply_options_t opts;
  opts.state = state;

  err = argp_parse (&argp, argc, argv, state->argp_flags,  0, &opts);
  if (!err)
    return lu_png_apply (state, &opts);
  else
    return err;
}

static void 
nowarn (png_structp png, const char *msg)
{
  return;
}
static void
save_comment (FILE *fp, FILE *out, char *data, size_t data_len)
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

  int num_text = 0;
  png_textp text_ptr;
  if (png_get_text (inpng, ininfo, &text_ptr, &num_text) > 0)
    {
      text_ptr = realloc (text_ptr, sizeof (png_text) * (num_text + 1));
      memmove (text_ptr + sizeof (png_text), text_ptr, sizeof (png_text) * num_text);
      memset (text_ptr, 0, sizeof (png_text));
      text_ptr->key = strdup ("Comment");
      text_ptr->text = data;
      text_ptr->text_length = data_len;
      text_ptr->itxt_length = data_len;
      text_ptr->compression = 1;
      png_set_text (inpng, ininfo, text_ptr, num_text);
    }
  else
    {
      //png_data_freer (inpng, ininfo, PNG_USER_WILL_FREE_DATA, PNG_FREE_TEXT);
      text_ptr = malloc (sizeof (png_text));
      memset (text_ptr, 0, sizeof (png_text));
      text_ptr->key = strdup ("Comment");
      text_ptr->text = data;
      text_ptr->text_length = data_len;
      text_ptr->itxt_length = data_len;
      text_ptr->compression = 1;
      png_set_text (inpng, ininfo, text_ptr, 1);
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

static int 
lu_save_comment (struct lu_state_t *state, struct lu_png_apply_options_t *options, char *f, char *text, size_t text_len)
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
          fprintf (stderr, "%s: `%s' is not a PNG file\n", png_apply.name,
                  f);
          return 1;
        }
      if (png_sig_cmp (signature, 0, 8) != 0)
        {
          fprintf (stderr, "%s: `%s' is not a PNG file\n", png_apply.name, f);
          return 1;
        }
    }
  FILE *out = fopen (tmp, "wb");
  if (fp && out)
    save_comment (fp, out, text, text_len);
  fclose (out);
  fclose (fp);
  if (options->backup)
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
  return 0;
}

int 
lu_png_apply (struct lu_state_t *state, struct lu_png_apply_options_t *options)
{
  int err = 0;
  if (!can_apply(png_apply.name))
    return -1;

  char boilerplate[sizeof(PACKAGE) + 13];
  snprintf (boilerplate, sizeof boilerplate, "/tmp/%s.XXXXXX", PACKAGE);
  int fd = mkstemp (boilerplate);
  close (fd);
  char *generate_cmd = xasprintf ("%s preview > %s", INTERPRETER_PATH, 
                                  boilerplate);
  system (generate_cmd);

  FILE *fileptr = fopen (boilerplate, "r");
  size_t data_len = 0;
  char *data = fread_file (fileptr, &data_len);
  fclose (fileptr);

  char *f = NULL;
  while ((f = argz_next (options->input_files, options->input_files_len, f)))
    {
      if (is_a_file (f) == 0)
        {
          if (errno == EISDIR)
            fprintf (stderr, N_("%s: %s: %s\n"),
                     png_apply.name, f, strerror (errno));
          else
            fprintf (stderr, N_("%s: could not open `%s' for reading: %s\n"),
                     png_apply.name, f, strerror (errno));
          continue;
        }
      else
        {
          if (access (f, W_OK) != 0)
            {
              fprintf (stderr, N_("%s: could not open `%s' for writing: %s\n"),
                       png_apply.name, f, strerror (errno));
              continue;
            }
        }
      err = lu_save_comment (state, options, f, data, data_len);
      if (!err)
        {
          if (options->quiet == 0)
            fprintf (stderr, "%s: %s -> Boilerplate applied.\n", png_apply.name, f);
        }
      if (err)
        break;
    }
  free (data);
  remove (boilerplate);
  return err;
}

struct lu_command_t png_apply = 
{
  .name         = N_("png-apply"),
  .doc          = PNG_APPLY_DOC,
  .flags        = DO_NOT_SHOW_IN_HELP | SAVE_IN_HISTORY,
  .argp         = &argp,
  .parser       = lu_png_apply_parse_argp
};
