/* Change a port cell to a directory by looking up a name.
   Copyright (C) 1999,2001,02 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <errno.h>
#include <unistd.h>
#include <hurd.h>
#include <hurd/port.h>
#include <hurd/fd.h>
#include <fcntl.h>
#include <string.h>

int
_hurd_change_directory_port_from_name (struct hurd_port *portcell,
				       const char *name)
{
  size_t len;
  const char *lookup;
  file_t dir;

  /* Append trailing "/." to directory name to force ENOTDIR if it's not a
     directory and EACCES if we don't have search permission.  */
  len = strlen (name);
  if (len >= 2 && name[len - 2] == '/' && name[len - 1] == '.')
    lookup = name;
  else
    {
      char *n = alloca (len + 3);
      memcpy (n, name, len);
      n[len] = '/';
      n[len + 1] = '.';
      n[len + 2] = '\0';
      lookup = n;
    }

  dir = __file_name_lookup (lookup, 0, 0);
  if (dir == MACH_PORT_NULL)
    return -1;

  _hurd_port_set (portcell, dir);
  return 0;
}
