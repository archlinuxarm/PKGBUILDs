/* Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <fcntl.h>
#include <unistd.h>


/* Duplicate FD to FD2, closing the old FD2 and making FD2 be
   open the same file as FD is which setting flags according to
   FLAGS.  Return FD2 or -1.  */
int
dup3 (fd, fd2, flags)
     int fd;
     int fd2;
     int flags;
{
  if (fd < 0 || fd2 < 0)
    {
      __set_errno (EBADF);
      return -1;
    }

  if (fd == fd2)
    /* No way to check that they are valid.  */
    return fd2;

  __set_errno (ENOSYS);
  return -1;
}
stub_warning (dup3)

#include <stub-tag.h>
