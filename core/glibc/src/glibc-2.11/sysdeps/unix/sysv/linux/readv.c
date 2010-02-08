/* readv supports all Linux kernels >= 2.0.
   Copyright (C) 1997,1998,2000,2002,2003,2009 Free Software Foundation, Inc.
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
#include <stddef.h>
#include <sys/param.h>
#include <sys/uio.h>

#include <sysdep-cancel.h>
#include <sys/syscall.h>
#include <bp-checks.h>
#include <kernel-features.h>

#ifndef __ASSUME_COMPLETE_READV_WRITEV
static ssize_t __atomic_readv_replacement (int, __const struct iovec *,
					   int) internal_function;
#endif


/* Not all versions of the kernel support the large number of records.  */
#ifndef UIO_FASTIOV
# define UIO_FASTIOV	8	/* 8 is a safe number.  */
#endif


ssize_t
__libc_readv (fd, vector, count)
     int fd;
     const struct iovec *vector;
     int count;
{
  ssize_t result;

  if (SINGLE_THREAD_P)
    result = INLINE_SYSCALL (readv, 3, fd, CHECK_N (vector, count), count);
  else
    {
      int oldtype = LIBC_CANCEL_ASYNC ();

      result = INLINE_SYSCALL (readv, 3, fd, CHECK_N (vector, count), count);

      LIBC_CANCEL_RESET (oldtype);
    }

#ifdef __ASSUME_COMPLETE_READV_WRITEV
  return result;
#else
  if (result >= 0 || errno != EINVAL || count <= UIO_FASTIOV)
    return result;

  return __atomic_readv_replacement (fd, vector, count);
#endif
}
strong_alias (__libc_readv, __readv)
weak_alias (__libc_readv, readv)

#ifndef __ASSUME_COMPLETE_READV_WRITEV
# define __libc_readv static internal_function __atomic_readv_replacement
# include <sysdeps/posix/readv.c>
#endif
