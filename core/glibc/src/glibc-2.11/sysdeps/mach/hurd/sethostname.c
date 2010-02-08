/* Copyright (C) 1991, 92, 93, 94, 96, 97 Free Software Foundation, Inc.
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

#include <unistd.h>
#include "hurdhost.h"

/* Set the name of the current host to NAME, which is LEN bytes long.
   This call is restricted to the super-user.  */
/* XXX should be __sethostname ? */
int
sethostname (name, len)
     const char *name;
     size_t len;
{
  /* The host name is just the contents of the file /etc/hostname.  */
  ssize_t n = _hurd_set_host_config ("/etc/hostname", name, len);
  return n < 0 ? -1 : 0;
}
