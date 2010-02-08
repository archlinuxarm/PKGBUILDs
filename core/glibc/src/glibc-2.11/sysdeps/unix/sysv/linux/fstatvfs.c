/* Copyright (C) 1998, 1999, 2000, 2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

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

#include <stddef.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>

extern void __internal_statvfs (const char *name, struct statvfs *buf,
				struct statfs *fsbuf, struct stat64 *st);


int
fstatvfs (int fd, struct statvfs *buf)
{
  struct statfs fsbuf;
  struct stat64 st;

  /* Get as much information as possible from the system.  */
  if (__fstatfs (fd, &fsbuf) < 0)
    return -1;

  /* Convert the result.  */
  __internal_statvfs (NULL, buf, &fsbuf, fstat64 (fd, &st) == -1 ? NULL : &st);

  /* We signal success if the statfs call succeeded.  */
  return 0;
}
libc_hidden_def (fstatvfs)
