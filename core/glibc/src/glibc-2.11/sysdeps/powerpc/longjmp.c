/* Copyright (C) 1991,92,94,95,97,98,2000,2002,2004
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

/* Versioned copy of sysdeps/generic/longjmp.c modified for AltiVec support. */

#include  <shlib-compat.h>
#include <stddef.h>
#include <setjmp.h>
#include <signal.h>

extern void __vmx__longjmp (__jmp_buf __env, int __val)
     __attribute__ ((noreturn));
extern void __vmx__libc_longjmp (sigjmp_buf env, int val)
     __attribute__ ((noreturn));
libc_hidden_proto (__vmx__libc_longjmp)

/* Set the signal mask to the one specified in ENV, and jump
   to the position specified in ENV, causing the setjmp
   call there to return VAL, or 1 if VAL is 0.  */
void
__vmx__libc_siglongjmp (sigjmp_buf env, int val)
{
  /* Perform any cleanups needed by the frames being unwound.  */
  _longjmp_unwind (env, val);

  if (env[0].__mask_was_saved)
    /* Restore the saved signal mask.  */
    (void) __sigprocmask (SIG_SETMASK, &env[0].__saved_mask,
			  (sigset_t *) NULL);

  /* Call the machine-dependent function to restore machine state.  */
  __vmx__longjmp (env[0].__jmpbuf, val ?: 1);
}

strong_alias (__vmx__libc_siglongjmp, __vmx__libc_longjmp)
libc_hidden_def (__vmx__libc_longjmp)
weak_alias (__vmx__libc_siglongjmp, __vmx_longjmp)
weak_alias (__vmx__libc_siglongjmp, __vmxlongjmp)
weak_alias (__vmx__libc_siglongjmp, __vmxsiglongjmp)


default_symbol_version (__vmx__libc_longjmp, __libc_longjmp, GLIBC_PRIVATE);
default_symbol_version (__vmx__libc_siglongjmp, __libc_siglongjmp, GLIBC_PRIVATE);
default_symbol_version (__vmx_longjmp, _longjmp, GLIBC_2.3.4);
default_symbol_version (__vmxlongjmp, longjmp, GLIBC_2.3.4);
default_symbol_version (__vmxsiglongjmp, siglongjmp, GLIBC_2.3.4);
