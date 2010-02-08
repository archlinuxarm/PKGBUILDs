/* Copyright (C) 1991,92,95,96,97,2002,2003 Free Software Foundation, Inc.
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
#include <sysdep-cancel.h>
#include <stdlib.h>
#include <sys/wait.h>

__pid_t
__libc_waitpid (__pid_t pid, int *stat_loc, int options)
{
  if (SINGLE_THREAD_P)
    {
#ifdef __NR_waitpid
      return INLINE_SYSCALL (waitpid, 3, pid, stat_loc, options);
#else
      return INLINE_SYSCALL (wait4, 4, pid, stat_loc, options, NULL);
#endif
    }

  int oldtype = LIBC_CANCEL_ASYNC ();

#ifdef __NR_waitpid
  int result = INLINE_SYSCALL (waitpid, 3, pid, stat_loc, options);
#else
  int result = INLINE_SYSCALL (wait4, 4, pid, stat_loc, options, NULL);
#endif

  LIBC_CANCEL_RESET (oldtype);

  return result;
}
weak_alias (__libc_waitpid, __waitpid)
libc_hidden_weak (__waitpid)
weak_alias (__libc_waitpid, waitpid)
