/* Copyright (C) 1997,1998,2000,2002,2003,2004, 2005 Free Software Foundation, Inc.
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
#define __need_NULL
#include <stddef.h>
#include <string.h>

#include <sysdep-cancel.h>
#include <sys/syscall.h>
#include <bp-checks.h>

#ifdef __NR_rt_sigtimedwait

static int
do_sigwaitinfo (const sigset_t *set, siginfo_t *info)
{
#ifdef SIGCANCEL
  sigset_t tmpset;
  if (set != NULL
      && (__builtin_expect (__sigismember (set, SIGCANCEL), 0)
# ifdef SIGSETXID
	  || __builtin_expect (__sigismember (set, SIGSETXID), 0)
# endif
	  ))
    {
      /* Create a temporary mask without the bit for SIGCANCEL set.  */
      // We are not copying more than we have to.
      memcpy (&tmpset, set, _NSIG / 8);
      __sigdelset (&tmpset, SIGCANCEL);
# ifdef SIGSETXID
      __sigdelset (&tmpset, SIGSETXID);
# endif
      set = &tmpset;
    }
#endif

  /* XXX The size argument hopefully will have to be changed to the
     real size of the user-level sigset_t.  */
  int result = INLINE_SYSCALL (rt_sigtimedwait, 4, CHECK_SIGSET (set),
			       CHECK_1 (info), NULL, _NSIG / 8);

  /* The kernel generates a SI_TKILL code in si_code in case tkill is
     used.  tkill is transparently used in raise().  Since having
     SI_TKILL as a code is useful in general we fold the results
     here.  */
  if (result != -1 && info != NULL && info->si_code == SI_TKILL)
    info->si_code = SI_USER;

  return result;
}


/* Return any pending signal or wait for one for the given time.  */
int
__sigwaitinfo (set, info)
     const sigset_t *set;
     siginfo_t *info;
{
  if (SINGLE_THREAD_P)
    return do_sigwaitinfo (set, info);

  int oldtype = LIBC_CANCEL_ASYNC ();

  /* XXX The size argument hopefully will have to be changed to the
     real size of the user-level sigset_t.  */
  int result = do_sigwaitinfo (set, info);

  LIBC_CANCEL_RESET (oldtype);

  return result;
}
libc_hidden_def (__sigwaitinfo)
weak_alias (__sigwaitinfo, sigwaitinfo)
#else
# include <signal/sigwaitinfo.c>
#endif
strong_alias (__sigwaitinfo, __libc_sigwaitinfo)
