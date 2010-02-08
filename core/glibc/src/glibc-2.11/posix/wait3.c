/* Copyright (C) 1991, 1993, 1995, 1996, 1997 Free Software Foundation, Inc.
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
#include <sys/wait.h>
#include <sys/types.h>

/* Wait for a child to exit.  When one does, put its status in *STAT_LOC and
   return its process ID.  For errors return (pid_t) -1.  If USAGE is not nil,
   store information about the child's resource usage (as a `struct rusage')
   there.  If the WUNTRACED bit is set in OPTIONS, return status for stopped
   children; otherwise don't.  */
pid_t
__wait3 (__WAIT_STATUS_DEFN stat_loc, int options, struct rusage *usage)
{
  if ((options & ~(WNOHANG|WUNTRACED)) != 0)
    {
      __set_errno (EINVAL);
      return (pid_t) -1;
    }

  __set_errno (ENOSYS);
  return (pid_t) -1;
}
stub_warning (wait3)

weak_alias (__wait3, wait3)
#include <stub-tag.h>
