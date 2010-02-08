/* Copyright (C) 1991,92,93,94,95,97,2000,02 Free Software Foundation, Inc.
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
#include <sys/types.h>

/* Seek to OFFSET on FD, starting from WHENCE.  */
off_t
__libc_lseek (int fd, off_t offset, int whence)
{
  return __libc_lseek64 (fd, (off64_t) offset, whence);
}

weak_alias (__libc_lseek, __lseek)
libc_hidden_def (__lseek)
weak_alias (__libc_lseek, lseek)
