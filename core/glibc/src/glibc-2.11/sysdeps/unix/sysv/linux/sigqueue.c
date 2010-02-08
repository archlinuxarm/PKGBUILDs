/* Copyright (C) 1997, 1998, 2000, 2003, 2005 Free Software Foundation, Inc.
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
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <sysdep.h>
#include <sys/syscall.h>

#ifdef __NR_rt_sigqueueinfo
/* Return any pending signal or wait for one for the given time.  */
int
__sigqueue (pid, sig, val)
     pid_t pid;
     int sig;
     const union sigval val;
{
  siginfo_t info;

  /* First, clear the siginfo_t structure, so that we don't pass our
     stack content to other tasks.  */
  memset (&info, 0, sizeof (siginfo_t));
  /* We must pass the information about the data in a siginfo_t value.  */
  info.si_signo = sig;
  info.si_code = SI_QUEUE;
  info.si_pid = __getpid ();
  info.si_uid = __getuid ();
  info.si_value = val;

  return INLINE_SYSCALL (rt_sigqueueinfo, 3, pid, sig, __ptrvalue (&info));
}
weak_alias (__sigqueue, sigqueue)
#else
# include <signal/sigqueue.c>
#endif
