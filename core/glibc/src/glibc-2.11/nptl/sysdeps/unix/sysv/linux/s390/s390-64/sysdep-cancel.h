/* Copyright (C) 2003, 2004, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2003.

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

#include <sysdep.h>
#include <tls.h>
#ifndef __ASSEMBLER__
# include <nptl/pthreadP.h>
#endif

#if !defined NOT_IN_libc || defined IS_IN_libpthread || defined IS_IN_librt

# undef PSEUDO
# define PSEUDO(name, syscall_name, args)				      \
	.text;								      \
L(pseudo_cancel):							      \
	cfi_startproc;							      \
	STM_##args							      \
	stmg	%r13,%r15,104(%r15);					      \
	cfi_offset (%r15,-40);						      \
	cfi_offset (%r14,-48);						      \
	cfi_offset (%r13,-56);						      \
	lgr	%r14,%r15;						      \
	aghi	%r15,-160;						      \
	cfi_adjust_cfa_offset (160);					      \
	stg	%r14,0(%r15);						      \
	brasl	%r14,CENABLE;						      \
	lgr	%r0,%r2;						      \
	LM_##args							      \
	.if SYS_ify (syscall_name) < 256;				      \
	svc SYS_ify (syscall_name);					      \
	.else;								      \
	lghi %r1,SYS_ify (syscall_name);				      \
	svc 0;								      \
	.endif;								      \
	LR7_##args							      \
	lgr	%r13,%r2;						      \
	lgr	%r2,%r0;						      \
	brasl	%r14,CDISABLE;						      \
	lgr	%r2,%r13;						      \
	lmg	%r13,%r15,104+160(%r15);				      \
	cfi_endproc;							      \
	j	L(pseudo_check);					      \
ENTRY(name)								      \
	SINGLE_THREAD_P							      \
	jne	L(pseudo_cancel);					      \
.type	__##syscall_name##_nocancel,@function;				      \
.globl	__##syscall_name##_nocancel;					      \
__##syscall_name##_nocancel:						      \
	DO_CALL(syscall_name, args);					      \
L(pseudo_check):							      \
	lghi	%r4,-4095;						      \
	clgr	%r2,%r4;						      \
	jgnl	SYSCALL_ERROR_LABEL;					      \
.size	__##syscall_name##_nocancel,.-__##syscall_name##_nocancel;	      \
L(pseudo_end):

# ifdef IS_IN_libpthread
#  define CENABLE	__pthread_enable_asynccancel
#  define CDISABLE	__pthread_disable_asynccancel
#  define __local_multiple_threads	__pthread_multiple_threads
# elif !defined NOT_IN_libc
#  define CENABLE	__libc_enable_asynccancel
#  define CDISABLE	__libc_disable_asynccancel
#  define __local_multiple_threads	__libc_multiple_threads
# elif defined IS_IN_librt
#  define CENABLE	__librt_enable_asynccancel
#  define CDISABLE	__librt_disable_asynccancel
# else
#  error Unsupported library
# endif

#define STM_0		/* Nothing */
#define STM_1		stg %r2,16(%r15);
#define STM_2		stmg %r2,%r3,16(%r15);
#define STM_3		stmg %r2,%r4,16(%r15);
#define STM_4		stmg %r2,%r5,16(%r15);
#define STM_5		stmg %r2,%r5,16(%r15);
#define STM_6		stmg %r2,%r7,16(%r15);

#define LM_0		/* Nothing */
#define LM_1		lg %r2,16+160(%r15);
#define LM_2		lmg %r2,%r3,16+160(%r15);
#define LM_3		lmg %r2,%r4,16+160(%r15);
#define LM_4		lmg %r2,%r5,16+160(%r15);
#define LM_5		lmg %r2,%r5,16+160(%r15);
#define LM_6		lmg %r2,%r5,16+160(%r15); \
			cfi_offset (%r7, -104); \
			lg %r7,160+160(%r15);

#define LR7_0		/* Nothing */
#define LR7_1		/* Nothing */
#define LR7_2		/* Nothing */
#define LR7_3		/* Nothing */
#define LR7_4		/* Nothing */
#define LR7_5		/* Nothing */
#define LR7_6		lg %r7,56+160(%r15); \
			cfi_restore (%r7);

# if defined IS_IN_libpthread || !defined NOT_IN_libc
#  ifndef __ASSEMBLER__
extern int __local_multiple_threads attribute_hidden;
#   define SINGLE_THREAD_P \
  __builtin_expect (__local_multiple_threads == 0, 1)
#  else
#   define SINGLE_THREAD_P \
	larl	%r1,__local_multiple_threads;				      \
	icm	%r0,15,0(%r1);
#  endif

# else

#  ifndef __ASSEMBLER__
#   define SINGLE_THREAD_P \
  __builtin_expect (THREAD_GETMEM (THREAD_SELF,				      \
				   header.multiple_threads) == 0, 1)
#  else
#   define SINGLE_THREAD_P \
	ear	%r1,%a0;						      \
	sllg	%r1,%r1,32;						      \
	ear	%r1,%a1;						      \
	icm	%r1,15,MULTIPLE_THREADS_OFFSET(%r1);
#  endif

# endif

#elif !defined __ASSEMBLER__

# define SINGLE_THREAD_P (1)
# define NO_CANCELLATION 1

#endif

#ifndef __ASSEMBLER__
# define RTLD_SINGLE_THREAD_P \
  __builtin_expect (THREAD_GETMEM (THREAD_SELF, \
				   header.multiple_threads) == 0, 1)
#endif
