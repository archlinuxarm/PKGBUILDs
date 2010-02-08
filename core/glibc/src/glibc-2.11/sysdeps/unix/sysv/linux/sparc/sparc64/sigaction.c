/* POSIX.1 sigaction call for Linux/SPARC64.
   Copyright (C) 1997-2000,2002,2003,2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Miguel de Icaza (miguel@nuclecu.unam.mx) and
		  Jakub Jelinek (jj@ultra.linux.cz).

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

#include <string.h>
#include <syscall.h>
#include <sysdep.h>
#include <sys/signal.h>
#include <errno.h>

#include <kernel_sigaction.h>

/* SPARC 64bit userland requires a kernel that has rt signals anyway. */

static void __rt_sigreturn_stub (void);

int
__libc_sigaction (int sig, __const struct sigaction *act,
		  struct sigaction *oact)
{
  int ret;
  struct kernel_sigaction kact, koact;
  unsigned long stub = ((unsigned long) &__rt_sigreturn_stub) - 8;

  if (act)
    {
      kact.k_sa_handler = act->sa_handler;
      memcpy (&kact.sa_mask, &act->sa_mask, sizeof (sigset_t));
      kact.sa_flags = act->sa_flags;
      kact.sa_restorer = NULL;
    }

  /* XXX The size argument hopefully will have to be changed to the
     real size of the user-level sigset_t.  */
  ret = INLINE_SYSCALL (rt_sigaction, 5, sig,
			act ? __ptrvalue (&kact) : 0,
			oact ? __ptrvalue (&koact) : 0, stub, _NSIG / 8);

  if (oact && ret >= 0)
    {
      oact->sa_handler = koact.k_sa_handler;
      memcpy (&oact->sa_mask, &koact.sa_mask, sizeof (sigset_t));
      oact->sa_flags = koact.sa_flags;
      oact->sa_restorer = koact.sa_restorer;
    }

  return ret;
}
libc_hidden_def (__libc_sigaction)

#ifdef WRAPPER_INCLUDE
# include WRAPPER_INCLUDE
#endif

#ifndef LIBC_SIGACTION
weak_alias (__libc_sigaction, __sigaction);
libc_hidden_weak (__sigaction)
weak_alias (__libc_sigaction, sigaction);
#endif

static void
__rt_sigreturn_stub (void)
{
  __asm__ ("mov %0, %%g1\n\t"
	   "ta	0x6d\n\t"
	   : /* no outputs */
	   : "i" (__NR_rt_sigreturn));
}
