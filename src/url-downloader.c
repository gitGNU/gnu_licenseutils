/*  Copyright (C) 2014 Ben Asselstine

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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "licensing_priv.h"
#include "read-file.h"
#include "xvasprintf.h"
#include "gettext-more.h"
#include "error.h"
#include "url-downloader.h"
#include "md2.h"

static char *
url_to_checksum (const char *url)
{
  char buf[17];
  memset (buf, 0, sizeof (buf));
  struct md2_ctx md2;
  md2_init_ctx (&md2);
  md2_buffer (url, strlen (url), &md2);
  md2_finish_ctx (&md2, buf);
  return xasprintf ("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
}

static char *
url_to_cache_filename (const char *url)
{
  char *cksum = url_to_checksum (url);
  char *f = xasprintf ("%s/.licenseutils/cache/%s", getenv ("HOME"), cksum);
  free (cksum);
  return f;
}

static void
make_cache_dir ()
{
  char *dir = xasprintf ("%s/.licenseutils/cache", getenv ("HOME"));
  DIR *d = opendir (dir);
  if (!d)
    mkdir (dir, 0775);
  else
    closedir (d);
}

int
download (struct lu_state_t *state, char *url, char **data)
{
  size_t data_len = 0;
  char *filename = url_to_cache_filename (url);
  make_cache_dir ();
  FILE *fileptr = fopen (filename, "r");
  if (fileptr)
    {
      *data = fread_file (fileptr, &data_len);
      fclose (fileptr);
      return 0;
    }
  int err = 0;
  fileptr = fopen (filename, "wb");
  curl_easy_setopt (state->curl, CURLOPT_HTTPGET, 1);
  curl_easy_setopt (state->curl, CURLOPT_URL, url);
  curl_easy_setopt (state->curl, CURLOPT_WRITEDATA, fileptr);
  curl_easy_perform(state->curl);
  fflush (fileptr);
  fsync (fileno (fileptr));
  fclose (fileptr);
  int response = 0;
  curl_easy_getinfo (state->curl, CURLINFO_RESPONSE_CODE, &response);
  if (response != 200)
    {
      error (0, 0, N_("got unexpected response code %d from %s"), response,
             url);
      err = 1;
      return err;
    }
  fileptr = fopen (filename, "r");
  *data = fread_file (fileptr, &data_len);
  fclose (fileptr);
  return err;
}

void
clear_download_cache ()
{
  char *dir = xasprintf ("%s/.licenseutils/cache", getenv ("HOME"));
  DIR *d = opendir (dir);
  struct dirent *entry;
  while ((entry=readdir(d)))
    {
      if (entry->d_name[0] == '.')
        continue;
      char *filename = xasprintf ("%s/%s", dir, entry->d_name);
      remove (filename);
      free (filename);
    }

  closedir (d);
  rmdir (dir);
}
