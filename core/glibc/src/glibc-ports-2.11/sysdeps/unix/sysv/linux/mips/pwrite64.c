/* Copyright (C) 1997, 1998, 2000, 2002, 2003, 2004
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ralf Baechle <ralf@gnu.org>, 1998.

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
#ifndef NO_SGIDEFS_H
#include <sgidefs.h>
#endif
#include <unistd.h>
#include <endian.h>

#include <sysdep-cancel.h>
#include <sys/syscall.h>
#include <bp-checks.h>

#include <kernel-features.h>

#ifdef __NR_pwrite64            /* Newer kernels renamed but it's the same.  */
# ifdef __NR_pwrite
#  error "__NR_pwrite and __NR_pwrite64 both defined???"
# endif
# define __NR_pwrite __NR_pwrite64
#endif

#if defined __NR_pwrite || __ASSUME_PWRITE_SYSCALL > 0

# if __ASSUME_PWRITE_SYSCALL == 0
static ssize_t __emulate_pwrite64 (int fd, const void *buf, size_t count,
				   off64_t offset) internal_function;
# endif

ssize_t
__libc_pwrite64 (fd, buf, count, offset)
     int fd;
     const void *buf;
     size_t count;
     off64_t offset;
{
  ssize_t result;

  if (SINGLE_THREAD_P)
    {
     /* First try the syscall.  */
#if _MIPS_SIM == _ABIN32 || _MIPS_SIM == _ABI64
      result = INLINE_SYSCALL (pwrite, 4, fd, CHECK_N (buf, count), count,
			       offset);
#else
     result = INLINE_SYSCALL (pwrite, 6, fd, CHECK_N (buf, count), count, 0,
			      __LONG_LONG_PAIR ((off_t) (offset >> 32),
			     (off_t) (offset & 0xffffffff)));
#endif
# if __ASSUME_PWRITE_SYSCALL == 0
     if (result == -1 && errno == ENOSYS)
     /* No system call available.  Use the emulation.  */
     result = __emulate_pwrite64 (fd, buf, count, offset);
# endif

     return result;
    }

  int oldtype = LIBC_CANCEL_ASYNC ();

  /* First try the syscall.  */
#if _MIPS_SIM == _ABIN32 || _MIPS_SIM == _ABI64
  result = INLINE_SYSCALL (pwrite, 4, fd, CHECK_N (buf, count), count, offset);
#else
  result = INLINE_SYSCALL (pwrite, 6, fd, CHECK_N (buf, count), count, 0,
			   __LONG_LONG_PAIR ((off_t) (offset >> 32),
					     (off_t) (offset & 0xffffffff)));
#endif
# if __ASSUME_PWRITE_SYSCALL == 0
  if (result == -1 && errno == ENOSYS)
    /* No system call available.  Use the emulation.  */
    result = __emulate_pwrite64 (fd, buf, count, offset);
# endif

  LIBC_CANCEL_RESET (oldtype);

  return result;
}

weak_alias (__libc_pwrite64, __pwrite64)
libc_hidden_weak (__pwrite64)
weak_alias (__libc_pwrite64, pwrite64)

# define __libc_pwrite64(fd, buf, count, offset) \
     static internal_function __emulate_pwrite64 (fd, buf, count, offset)
#endif

#if __ASSUME_PWRITE_SYSCALL == 0
# include <sysdeps/posix/pwrite64.c>
#endif
