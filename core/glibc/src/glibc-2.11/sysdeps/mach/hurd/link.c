/* Copyright (C) 1991, 92, 93, 94, 95, 96, 97 Free Software Foundation, Inc.
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
#include <stddef.h>
#include <unistd.h>
#include <hurd.h>

/* Make a link to FROM called TO.  */
int
__link (from, to)
     const char *from;
     const char *to;
{
  error_t err;
  file_t oldfile, linknode, todir;
  char *toname;

  oldfile = __file_name_lookup (from, 0, 0);
  if (oldfile == MACH_PORT_NULL)
    return -1;

  /* The file_getlinknode RPC returns the port that should be passed to
     the receiving filesystem (the one containing TODIR) in dir_link.  */

  err = __file_getlinknode (oldfile, &linknode);
  __mach_port_deallocate (__mach_task_self (), oldfile);
  if (err)
    return __hurd_fail (err);

  todir = __file_name_split (to, &toname);
  if (todir != MACH_PORT_NULL)
    {
      err = __dir_link (todir, linknode, toname, 1);
      __mach_port_deallocate (__mach_task_self (), todir);
    }
  __mach_port_deallocate (__mach_task_self (), linknode);
  if (todir == MACH_PORT_NULL)
    return -1;

  if (err)
    return __hurd_fail (err);
  return 0;
}

weak_alias (__link, link)
