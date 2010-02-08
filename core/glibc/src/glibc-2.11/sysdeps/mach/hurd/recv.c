/* Copyright (C) 1994,1997,2001,2005 Free Software Foundation, Inc.
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
#include <sys/socket.h>
#include <hurd.h>
#include <hurd/fd.h>
#include <hurd/socket.h>
#include <string.h>

/* Read N bytes into BUF from socket FD.
   Returns the number read or -1 for errors.  */

ssize_t
__recv (fd, buf, n, flags)
     int fd;
     void *buf;
     size_t n;
     int flags;
{
  error_t err;
  mach_port_t addrport;
  char *bufp = buf;
  mach_msg_type_number_t nread = n;
  mach_port_t *ports;
  mach_msg_type_number_t nports = 0;
  char *cdata = NULL;
  mach_msg_type_number_t clen = 0;

  if (err = HURD_DPORT_USE (fd, __socket_recv (port, &addrport,
					       flags, &bufp, &nread,
					       &ports, &nports,
					       &cdata, &clen,
					       &flags,
					       n)))
    return __hurd_sockfail (fd, flags, err);

  __mach_port_deallocate (__mach_task_self (), addrport);
  __vm_deallocate (__mach_task_self (), (vm_address_t) cdata, clen);

  if (bufp != buf)
    {
      memcpy (buf, bufp, nread);
      __vm_deallocate (__mach_task_self (), (vm_address_t) bufp, nread);
    }

  return nread;
}
weak_alias (__recv, recv)
