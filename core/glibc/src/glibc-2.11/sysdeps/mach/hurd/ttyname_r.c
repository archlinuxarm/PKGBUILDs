/* Copyright (C) 1994, 95, 96, 98 Free Software Foundation, Inc.
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
#include <string.h>
#include <unistd.h>
#include <hurd.h>
#include <hurd/term.h>
#include <hurd/fd.h>

/* Store at most BUFLEN characters of the pathname of the terminal FD is
   open on in BUF.  Return 0 on success, -1 otherwise.  */
int
__ttyname_r (int fd, char *buf, size_t buflen)
{
  error_t err;
  char nodename[1024];	/* XXX */
  size_t len;

  nodename[0] = '\0';
  if (err = HURD_DPORT_USE (fd, __term_get_nodename (port, nodename)))
    return __hurd_dfail (fd, err), -1;

  len = strlen (nodename) + 1;
  if (len > buflen)
    {
      errno = EINVAL;
      return -1;
    }

  memcpy (buf, nodename, len);
  return 0;
}

weak_alias (__ttyname_r, ttyname_r)
