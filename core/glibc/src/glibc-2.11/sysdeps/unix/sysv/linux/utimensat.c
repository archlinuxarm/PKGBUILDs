/* Change access and modification times of open file.  Linux version.
   Copyright (C) 2007 Free Software Foundation, Inc.
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
#include <sys/stat.h>
#include <sysdep.h>

#include <kernel-features.h>


/* Change the access time of FILE to TSP[0] and
   the modification time of FILE to TSP[1].

   Starting with 2.6.22 the Linux kernel has the utimensat syscall.  */
int
utimensat (int fd, const char *file, const struct timespec tsp[2],
	   int flags)
{
  if (file == NULL)
    {
      __set_errno (EINVAL);
      return -1;
    }
#ifdef __NR_utimensat
  return INLINE_SYSCALL (utimensat, 4, fd, file, tsp, flags);
#else
  __set_errno (ENOSYS);
  return -1;
#endif
}
#ifndef __NR_utimensat
stub_warning (utimensat)
# include <stub-tag.h>
#endif
