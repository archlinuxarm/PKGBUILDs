/* Copyright (C) 2007 Free Software Foundation, Inc.
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

#include <fcntl.h>
#include <kernel-features.h>
#include <sysdep.h>

#define posix_fallocate static internal_fallocate
#include <sysdeps/posix/posix_fallocate.c>
#undef posix_fallocate

#if !defined __ASSUME_FALLOCATE && defined __NR_fallocate
static int __have_fallocate;
#endif


/* Reserve storage for the data of the file associated with FD.  */
int
posix_fallocate (int fd, __off_t offset, __off_t len)
{
#ifdef __NR_fallocate
# ifndef __ASSUME_FALLOCATE
  if (__builtin_expect (__have_fallocate >= 0, 1))
# endif
    {
      INTERNAL_SYSCALL_DECL (err);
      int res = INTERNAL_SYSCALL (fallocate, err, 4, fd, 0, offset, len);

      if (! INTERNAL_SYSCALL_ERROR_P (res, err))
	return 0;

# ifndef __ASSUME_FALLOCATE
      if (__builtin_expect (INTERNAL_SYSCALL_ERRNO (res, err) == ENOSYS, 0))
	__have_fallocate = -1;
      else
# endif
	if (INTERNAL_SYSCALL_ERRNO (res, err) != EOPNOTSUPP)
	  return INTERNAL_SYSCALL_ERRNO (res, err);
    }
#endif

  return internal_fallocate (fd, offset, len);
}
strong_alias (posix_fallocate, posix_fallocate64)
