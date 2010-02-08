/* Copyright (C) 1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 1999.

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

#define SIGCONTEXT struct sigcontext *
#define SIGCONTEXT_EXTRA_ARGS
#define GET_PC(__ctx)	((void *) ((__ctx)->si_regs.pc))
#define FIRST_FRAME_POINTER \
  ({ void *ret;							\
     asm volatile ("ta 3; add %%fp, 56, %0" : "=r" (ret)); ret; })
#define ADVANCE_STACK_FRAME(__next) \
	((void *) (((unsigned *)(__next))+14))

#define GET_STACK(__ctx)	((void *) (__ctx)->si_regs.u_regs[14])
#define GET_FRAME(__ctx)	ADVANCE_STACK_FRAME (GET_STACK(__ctx))
#define CALL_SIGHANDLER(handler, signo, ctx) \
  (handler)((signo), SIGCONTEXT_EXTRA_ARGS (ctx))
