/* getifaddrs -- get names and addresses of all network interfaces
   Copyright (C) 2002, 2003 Free Software Foundation, Inc.
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

#include <ifaddrs.h>
#include <errno.h>
#include <stdlib.h>

/* Create a linked list of `struct ifaddrs' structures, one for each
   network interface on the host machine.  If successful, store the
   list in *IFAP and return 0.  On errors, return -1 and set `errno'.  */
int
getifaddrs (struct ifaddrs **ifap)
{
  __set_errno (ENOSYS);
  return -1;
}
libc_hidden_def (getifaddrs)
stub_warning (getifaddrs)

void
freeifaddrs (struct ifaddrs *ifa)
{
  if (ifa == NULL)
    return;			/* a la free, why not? */

  /* Can't be called properly if getifaddrs never succeeded.  */
  abort ();
}
libc_hidden_def (freeifaddrs)
stub_warning (freeifaddrs)
