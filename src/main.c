/*  Copyright (C) 2011, 2013 Ben Asselstine

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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include "opts.h"
#include "gettext-more.h"
#include <errno.h>
#include "licensing.h"
const char *progname = PACKAGE;

static int 
init_locales (const char *name)
{
  int err = 0;

#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");

  if (!bindtextdomain (PACKAGE_NAME, LOCALEDIR) && errno == ENOMEM)
    err = errno;
  else
    {
      if (!textdomain (PACKAGE_NAME) && errno == ENOMEM)
	err = errno;
    }

  if (err) 
    {
      fprintf (stderr,
	       "%s: `%s' failed; %s\n",
	       name, "init_locales", strerror (errno));
    } 
  else 
    {
      errno = 0;
    }
#endif /* ENABLE_NLS */

  progname = name;

  return err;
}

int
main (int argc, char **argv)
{
  int err = 0;
  struct arguments_t arguments;

  if ((err = init_locales (PACKAGE_NAME)))
    return err;

  memset (&arguments, 0, sizeof (struct arguments_t));

  parse_opts (argc, argv, &arguments);

  err = licensing (&arguments.lu);

  return err;
}
