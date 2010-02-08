/* pselect for Hurd.
   Copyright (C) 1998 Free Software Foundation, Inc.
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
#include <signal.h>
#include <sys/time.h>
#include <sys/select.h>
#include <hurd.h>
#include <hurd/fd.h>

/* Check the first NFDS descriptors each in READFDS (if not NULL) for read
   readiness, in WRITEFDS (if not NULL) for write readiness, and in EXCEPTFDS
   (if not NULL) for exceptional conditions.  If TIMEOUT is not NULL, time out
   after waiting the interval specified therein.  Additionally set the sigmask
   SIGMASK for this call.  Returns the number of ready descriptors, or -1 for
   errors.  */
int
__pselect (nfds, readfds, writefds, exceptfds, timeout, sigmask)
     int nfds;
     fd_set *readfds;
     fd_set *writefds;
     fd_set *exceptfds;
     const struct timespec *timeout;
     const sigset_t *sigmask;
{
  return _hurd_select (nfds, NULL,
		       readfds, writefds, exceptfds, timeout, sigmask);
}
weak_alias (__pselect, pselect)
