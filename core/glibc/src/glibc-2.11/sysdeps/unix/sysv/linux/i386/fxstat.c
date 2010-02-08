/* fxstat using old-style Unix fstat system call.
   Copyright (C) 1991,1995-1998,2000,2002,2003,2006
	Free Software Foundation, Inc.
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

/* Ho hum, if xstat == xstat64 we must get rid of the prototype or gcc
   will complain since they don't strictly match.  */
#define __fxstat64 __fxstat64_disable

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <kernel_stat.h>

#include <sysdep.h>
#include <sys/syscall.h>
#include <bp-checks.h>

#include <kernel-features.h>

#include <xstatconv.h>

#ifdef __NR_stat64
# if  __ASSUME_STAT64_SYSCALL == 0
/* The variable is shared between all wrappers around *stat64 calls.  */
extern int __have_no_stat64;
# endif
#endif

/* Get information about the file FD in BUF.  */
int
__fxstat (int vers, int fd, struct stat *buf)
{
#if __ASSUME_STAT64_SYSCALL == 0
  struct kernel_stat kbuf;
#endif
  int result;

  if (vers == _STAT_VER_KERNEL)
    return INLINE_SYSCALL (fstat, 2, fd, CHECK_1 ((struct kernel_stat *) buf));

#if __ASSUME_STAT64_SYSCALL > 0
  {
    struct stat64 buf64;

    result = INLINE_SYSCALL (fstat64, 2, fd, __ptrvalue (&buf64));
    if (result == 0)
      result = __xstat32_conv (vers, &buf64, buf);
    return result;
  }
#else

# if defined __NR_stat64
  /* To support 32 bit UIDs, we have to use stat64.  The normal stat call only returns
     16 bit UIDs.  */
  if (! __have_no_stat64)
    {
      struct stat64 buf64;

      result = INLINE_SYSCALL (fstat64, 2, fd, __ptrvalue (&buf64));

      if (result == 0)
	result = __xstat32_conv (vers, &buf64, buf);

      if (result != -1 || errno != ENOSYS)
	return result;

      __have_no_stat64 = 1;
    }
# endif

  result = INLINE_SYSCALL (fstat, 2, fd, __ptrvalue (&kbuf));
  if (result == 0)
    result = __xstat_conv (vers, &kbuf, buf);

  return result;
#endif  /* __ASSUME_STAT64_SYSCALL  */
}

hidden_def (__fxstat)
weak_alias (__fxstat, _fxstat);
#ifdef XSTAT_IS_XSTAT64
#undef __fxstat64
strong_alias (__fxstat, __fxstat64);
hidden_ver (__fxstat, __fxstat64)
#endif
