/* Copyright (C) 1992,1993,1995-2000,2002-2006,2007
   	Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper, <drepper@gnu.org>, August 1995.

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

#ifndef _LINUX_I386_SYSDEP_H
#define _LINUX_I386_SYSDEP_H 1

/* There is some commonality.  */
#include <sysdeps/unix/i386/sysdep.h>
#include <bp-sym.h>
#include <bp-asm.h>
/* Defines RTLD_PRIVATE_ERRNO and USE_DL_SYSINFO.  */
#include <dl-sysdep.h>
#include <tls.h>


/* For Linux we can use the system call table in the header file
	/usr/include/asm/unistd.h
   of the kernel.  But these symbols do not follow the SYS_* syntax
   so we have to redefine the `SYS_ify' macro here.  */
#undef SYS_ify
#define SYS_ify(syscall_name)	__NR_##syscall_name

#if defined USE_DL_SYSINFO \
    && (!defined NOT_IN_libc || defined IS_IN_libpthread)
# define I386_USE_SYSENTER	1
#else
# undef I386_USE_SYSENTER
#endif

#ifdef __ASSEMBLER__

/* Linux uses a negative return value to indicate syscall errors,
   unlike most Unices, which use the condition codes' carry flag.

   Since version 2.1 the return value of a system call might be
   negative even if the call succeeded.  E.g., the `lseek' system call
   might return a large offset.  Therefore we must not anymore test
   for < 0, but test for a real error by making sure the value in %eax
   is a real error number.  Linus said he will make sure the no syscall
   returns a value in -1 .. -4095 as a valid result so we can savely
   test with -4095.  */

/* We don't want the label for the error handle to be global when we define
   it here.  */
#ifdef PIC
# define SYSCALL_ERROR_LABEL 0f
#else
# define SYSCALL_ERROR_LABEL syscall_error
#endif

#undef	PSEUDO
#define	PSEUDO(name, syscall_name, args)				      \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args);					      \
    cmpl $-4095, %eax;							      \
    jae SYSCALL_ERROR_LABEL;						      \
  L(pseudo_end):

#undef	PSEUDO_END
#define	PSEUDO_END(name)						      \
  SYSCALL_ERROR_HANDLER							      \
  END (name)

#undef	PSEUDO_NOERRNO
#define	PSEUDO_NOERRNO(name, syscall_name, args)			      \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args)

#undef	PSEUDO_END_NOERRNO
#define	PSEUDO_END_NOERRNO(name)					      \
  END (name)

#define ret_NOERRNO ret

/* The function has to return the error code.  */
#undef	PSEUDO_ERRVAL
#define	PSEUDO_ERRVAL(name, syscall_name, args) \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args);					      \
    negl %eax

#undef	PSEUDO_END_ERRVAL
#define	PSEUDO_END_ERRVAL(name) \
  END (name)

#define ret_ERRVAL ret

#ifndef PIC
# define SYSCALL_ERROR_HANDLER	/* Nothing here; code in sysdep.S is used.  */
#else

# if RTLD_PRIVATE_ERRNO
#  define SYSCALL_ERROR_HANDLER						      \
0:SETUP_PIC_REG(cx);							      \
  addl $_GLOBAL_OFFSET_TABLE_, %ecx;					      \
  xorl %edx, %edx;							      \
  subl %eax, %edx;							      \
  movl %edx, rtld_errno@GOTOFF(%ecx);					      \
  orl $-1, %eax;							      \
  jmp L(pseudo_end);

# elif defined _LIBC_REENTRANT

#  if USE___THREAD
#   ifndef NOT_IN_libc
#    define SYSCALL_ERROR_ERRNO __libc_errno
#   else
#    define SYSCALL_ERROR_ERRNO errno
#   endif
#   define SYSCALL_ERROR_HANDLER					      \
0:SETUP_PIC_REG (cx);							      \
  addl $_GLOBAL_OFFSET_TABLE_, %ecx;					      \
  movl SYSCALL_ERROR_ERRNO@GOTNTPOFF(%ecx), %ecx;			      \
  xorl %edx, %edx;							      \
  subl %eax, %edx;							      \
  SYSCALL_ERROR_HANDLER_TLS_STORE (%edx, %ecx);				      \
  orl $-1, %eax;							      \
  jmp L(pseudo_end);
#   ifndef NO_TLS_DIRECT_SEG_REFS
#    define SYSCALL_ERROR_HANDLER_TLS_STORE(src, destoff)		      \
  movl src, %gs:(destoff)
#   else
#    define SYSCALL_ERROR_HANDLER_TLS_STORE(src, destoff)		      \
  addl %gs:0, destoff;							      \
  movl src, (destoff)
#   endif
#  else
#   define SYSCALL_ERROR_HANDLER					      \
0:pushl %ebx;								      \
  cfi_adjust_cfa_offset (4);						      \
  cfi_rel_offset (ebx, 0);						      \
  SETUP_PIC_REG (bx);							      \
  addl $_GLOBAL_OFFSET_TABLE_, %ebx;					      \
  xorl %edx, %edx;							      \
  subl %eax, %edx;							      \
  pushl %edx;								      \
  cfi_adjust_cfa_offset (4);						      \
  PUSH_ERRNO_LOCATION_RETURN;						      \
  call BP_SYM (__errno_location)@PLT;					      \
  POP_ERRNO_LOCATION_RETURN;						      \
  popl %ecx;								      \
  cfi_adjust_cfa_offset (-4);						      \
  popl %ebx;								      \
  cfi_adjust_cfa_offset (-4);						      \
  cfi_restore (ebx);							      \
  movl %ecx, (%eax);							      \
  orl $-1, %eax;							      \
  jmp L(pseudo_end);
/* A quick note: it is assumed that the call to `__errno_location' does
   not modify the stack!  */
#  endif
# else
/* Store (- %eax) into errno through the GOT.  */
#  define SYSCALL_ERROR_HANDLER						      \
0:SETUP_PIC_REG(cx);							      \
  addl $_GLOBAL_OFFSET_TABLE_, %ecx;					      \
  xorl %edx, %edx;							      \
  subl %eax, %edx;							      \
  movl errno@GOT(%ecx), %ecx;						      \
  movl %edx, (%ecx);							      \
  orl $-1, %eax;							      \
  jmp L(pseudo_end);
# endif	/* _LIBC_REENTRANT */
#endif	/* PIC */


/* The original calling convention for system calls on Linux/i386 is
   to use int $0x80.  */
#ifdef I386_USE_SYSENTER
# ifdef SHARED
#  define ENTER_KERNEL call *%gs:SYSINFO_OFFSET
# else
#  define ENTER_KERNEL call *_dl_sysinfo
# endif
#else
# define ENTER_KERNEL int $0x80
#endif

/* Linux takes system call arguments in registers:

	syscall number	%eax	     call-clobbered
	arg 1		%ebx	     call-saved
	arg 2		%ecx	     call-clobbered
	arg 3		%edx	     call-clobbered
	arg 4		%esi	     call-saved
	arg 5		%edi	     call-saved
	arg 6		%ebp	     call-saved

   The stack layout upon entering the function is:

	24(%esp)	Arg# 6
	20(%esp)	Arg# 5
	16(%esp)	Arg# 4
	12(%esp)	Arg# 3
	 8(%esp)	Arg# 2
	 4(%esp)	Arg# 1
	  (%esp)	Return address

   (Of course a function with say 3 arguments does not have entries for
   arguments 4, 5, and 6.)

   The following code tries hard to be optimal.  A general assumption
   (which is true according to the data books I have) is that

	2 * xchg	is more expensive than	pushl + movl + popl

   Beside this a neat trick is used.  The calling conventions for Linux
   tell that among the registers used for parameters %ecx and %edx need
   not be saved.  Beside this we may clobber this registers even when
   they are not used for parameter passing.

   As a result one can see below that we save the content of the %ebx
   register in the %edx register when we have less than 3 arguments
   (2 * movl is less expensive than pushl + popl).

   Second unlike for the other registers we don't save the content of
   %ecx and %edx when we have more than 1 and 2 registers resp.

   The code below might look a bit long but we have to take care for
   the pipelined processors (i586).  Here the `pushl' and `popl'
   instructions are marked as NP (not pairable) but the exception is
   two consecutive of these instruction.  This gives no penalty on
   other processors though.  */

#undef	DO_CALL
#define DO_CALL(syscall_name, args)			      		      \
    PUSHARGS_##args							      \
    DOARGS_##args							      \
    movl $SYS_ify (syscall_name), %eax;					      \
    ENTER_KERNEL							      \
    POPARGS_##args

#define PUSHARGS_0	/* No arguments to push.  */
#define	DOARGS_0	/* No arguments to frob.  */
#define	POPARGS_0	/* No arguments to pop.  */
#define	_PUSHARGS_0	/* No arguments to push.  */
#define _DOARGS_0(n)	/* No arguments to frob.  */
#define	_POPARGS_0	/* No arguments to pop.  */

#define PUSHARGS_1	movl %ebx, %edx; L(SAVEBX1): PUSHARGS_0
#define	DOARGS_1	_DOARGS_1 (4)
#define	POPARGS_1	POPARGS_0; movl %edx, %ebx; L(RESTBX1):
#define	_PUSHARGS_1	pushl %ebx; cfi_adjust_cfa_offset (4); \
			cfi_rel_offset (ebx, 0); L(PUSHBX1): _PUSHARGS_0
#define _DOARGS_1(n)	movl n(%esp), %ebx; _DOARGS_0(n-4)
#define	_POPARGS_1	_POPARGS_0; popl %ebx; cfi_adjust_cfa_offset (-4); \
			cfi_restore (ebx); L(POPBX1):

#define PUSHARGS_2	PUSHARGS_1
#define	DOARGS_2	_DOARGS_2 (8)
#define	POPARGS_2	POPARGS_1
#define _PUSHARGS_2	_PUSHARGS_1
#define	_DOARGS_2(n)	movl n(%esp), %ecx; _DOARGS_1 (n-4)
#define	_POPARGS_2	_POPARGS_1

#define PUSHARGS_3	_PUSHARGS_2
#define DOARGS_3	_DOARGS_3 (16)
#define POPARGS_3	_POPARGS_3
#define _PUSHARGS_3	_PUSHARGS_2
#define _DOARGS_3(n)	movl n(%esp), %edx; _DOARGS_2 (n-4)
#define _POPARGS_3	_POPARGS_2

#define PUSHARGS_4	_PUSHARGS_4
#define DOARGS_4	_DOARGS_4 (24)
#define POPARGS_4	_POPARGS_4
#define _PUSHARGS_4	pushl %esi; cfi_adjust_cfa_offset (4); \
			cfi_rel_offset (esi, 0); L(PUSHSI1): _PUSHARGS_3
#define _DOARGS_4(n)	movl n(%esp), %esi; _DOARGS_3 (n-4)
#define _POPARGS_4	_POPARGS_3; popl %esi; cfi_adjust_cfa_offset (-4); \
			cfi_restore (esi); L(POPSI1):

#define PUSHARGS_5	_PUSHARGS_5
#define DOARGS_5	_DOARGS_5 (32)
#define POPARGS_5	_POPARGS_5
#define _PUSHARGS_5	pushl %edi; cfi_adjust_cfa_offset (4); \
			cfi_rel_offset (edi, 0); L(PUSHDI1): _PUSHARGS_4
#define _DOARGS_5(n)	movl n(%esp), %edi; _DOARGS_4 (n-4)
#define _POPARGS_5	_POPARGS_4; popl %edi; cfi_adjust_cfa_offset (-4); \
			cfi_restore (edi); L(POPDI1):

#define PUSHARGS_6	_PUSHARGS_6
#define DOARGS_6	_DOARGS_6 (40)
#define POPARGS_6	_POPARGS_6
#define _PUSHARGS_6	pushl %ebp; cfi_adjust_cfa_offset (4); \
			cfi_rel_offset (ebp, 0); L(PUSHBP1): _PUSHARGS_5
#define _DOARGS_6(n)	movl n(%esp), %ebp; _DOARGS_5 (n-4)
#define _POPARGS_6	_POPARGS_5; popl %ebp; cfi_adjust_cfa_offset (-4); \
			cfi_restore (ebp); L(POPBP1):

#else	/* !__ASSEMBLER__ */

/* We need some help from the assembler to generate optimal code.  We
   define some macros here which later will be used.  */
asm (".L__X'%ebx = 1\n\t"
     ".L__X'%ecx = 2\n\t"
     ".L__X'%edx = 2\n\t"
     ".L__X'%eax = 3\n\t"
     ".L__X'%esi = 3\n\t"
     ".L__X'%edi = 3\n\t"
     ".L__X'%ebp = 3\n\t"
     ".L__X'%esp = 3\n\t"
     ".macro bpushl name reg\n\t"
     ".if 1 - \\name\n\t"
     ".if 2 - \\name\n\t"
     "error\n\t"
     ".else\n\t"
     "xchgl \\reg, %ebx\n\t"
     ".endif\n\t"
     ".endif\n\t"
     ".endm\n\t"
     ".macro bpopl name reg\n\t"
     ".if 1 - \\name\n\t"
     ".if 2 - \\name\n\t"
     "error\n\t"
     ".else\n\t"
     "xchgl \\reg, %ebx\n\t"
     ".endif\n\t"
     ".endif\n\t"
     ".endm\n\t");

/* Define a macro which expands inline into the wrapper code for a system
   call.  */
#undef INLINE_SYSCALL
#define INLINE_SYSCALL(name, nr, args...) \
  ({									      \
    unsigned int resultvar = INTERNAL_SYSCALL (name, , nr, args);	      \
    if (__builtin_expect (INTERNAL_SYSCALL_ERROR_P (resultvar, ), 0))	      \
      {									      \
	__set_errno (INTERNAL_SYSCALL_ERRNO (resultvar, ));		      \
	resultvar = 0xffffffff;						      \
      }									      \
    (int) resultvar; })

/* Define a macro which expands inline into the wrapper code for a system
   call.  This use is for internal calls that do not need to handle errors
   normally.  It will never touch errno.  This returns just what the kernel
   gave back.

   The _NCS variant allows non-constant syscall numbers but it is not
   possible to use more than four parameters.  */
#undef INTERNAL_SYSCALL
#ifdef I386_USE_SYSENTER
# ifdef SHARED
#  define INTERNAL_SYSCALL(name, err, nr, args...) \
  ({									      \
    register unsigned int resultvar;					      \
    EXTRAVAR_##nr							      \
    asm volatile (							      \
    LOADARGS_##nr							      \
    "movl %1, %%eax\n\t"						      \
    "call *%%gs:%P2\n\t"						      \
    RESTOREARGS_##nr							      \
    : "=a" (resultvar)							      \
    : "i" (__NR_##name), "i" (offsetof (tcbhead_t, sysinfo))		      \
      ASMFMT_##nr(args) : "memory", "cc");				      \
    (int) resultvar; })
#  define INTERNAL_SYSCALL_NCS(name, err, nr, args...) \
  ({									      \
    register unsigned int resultvar;					      \
    EXTRAVAR_##nr							      \
    asm volatile (							      \
    LOADARGS_##nr							      \
    "call *%%gs:%P2\n\t"						      \
    RESTOREARGS_##nr							      \
    : "=a" (resultvar)							      \
    : "0" (name), "i" (offsetof (tcbhead_t, sysinfo))			      \
      ASMFMT_##nr(args) : "memory", "cc");				      \
    (int) resultvar; })
# else
#  define INTERNAL_SYSCALL(name, err, nr, args...) \
  ({									      \
    register unsigned int resultvar;					      \
    EXTRAVAR_##nr							      \
    asm volatile (							      \
    LOADARGS_##nr							      \
    "movl %1, %%eax\n\t"						      \
    "call *_dl_sysinfo\n\t"						      \
    RESTOREARGS_##nr							      \
    : "=a" (resultvar)							      \
    : "i" (__NR_##name) ASMFMT_##nr(args) : "memory", "cc");		      \
    (int) resultvar; })
#  define INTERNAL_SYSCALL_NCS(name, err, nr, args...) \
  ({									      \
    register unsigned int resultvar;					      \
    EXTRAVAR_##nr							      \
    asm volatile (							      \
    LOADARGS_##nr							      \
    "call *_dl_sysinfo\n\t"						      \
    RESTOREARGS_##nr							      \
    : "=a" (resultvar)							      \
    : "0" (name) ASMFMT_##nr(args) : "memory", "cc");			      \
    (int) resultvar; })
# endif
#else
# define INTERNAL_SYSCALL(name, err, nr, args...) \
  ({									      \
    register unsigned int resultvar;					      \
    EXTRAVAR_##nr							      \
    asm volatile (							      \
    LOADARGS_##nr							      \
    "movl %1, %%eax\n\t"						      \
    "int $0x80\n\t"							      \
    RESTOREARGS_##nr							      \
    : "=a" (resultvar)							      \
    : "i" (__NR_##name) ASMFMT_##nr(args) : "memory", "cc");		      \
    (int) resultvar; })
# define INTERNAL_SYSCALL_NCS(name, err, nr, args...) \
  ({									      \
    register unsigned int resultvar;					      \
    EXTRAVAR_##nr							      \
    asm volatile (							      \
    LOADARGS_##nr							      \
    "int $0x80\n\t"							      \
    RESTOREARGS_##nr							      \
    : "=a" (resultvar)							      \
    : "0" (name) ASMFMT_##nr(args) : "memory", "cc");			      \
    (int) resultvar; })
#endif

#undef INTERNAL_SYSCALL_DECL
#define INTERNAL_SYSCALL_DECL(err) do { } while (0)

#undef INTERNAL_SYSCALL_ERROR_P
#define INTERNAL_SYSCALL_ERROR_P(val, err) \
  ((unsigned int) (val) >= 0xfffff001u)

#undef INTERNAL_SYSCALL_ERRNO
#define INTERNAL_SYSCALL_ERRNO(val, err)	(-(val))

#define LOADARGS_0
#ifdef __PIC__
# if defined I386_USE_SYSENTER && defined SHARED
#  define LOADARGS_1 \
    "bpushl .L__X'%k3, %k3\n\t"
#  define LOADARGS_5 \
    "movl %%ebx, %4\n\t"						      \
    "movl %3, %%ebx\n\t"
# else
#  define LOADARGS_1 \
    "bpushl .L__X'%k2, %k2\n\t"
#  define LOADARGS_5 \
    "movl %%ebx, %3\n\t"						      \
    "movl %2, %%ebx\n\t"
# endif
# define LOADARGS_2	LOADARGS_1
# define LOADARGS_3 \
    "xchgl %%ebx, %%edi\n\t"
# define LOADARGS_4	LOADARGS_3
#else
# define LOADARGS_1
# define LOADARGS_2
# define LOADARGS_3
# define LOADARGS_4
# define LOADARGS_5
#endif

#define RESTOREARGS_0
#ifdef __PIC__
# if defined I386_USE_SYSENTER && defined SHARED
#  define RESTOREARGS_1 \
    "bpopl .L__X'%k3, %k3\n\t"
#  define RESTOREARGS_5 \
    "movl %4, %%ebx"
# else
#  define RESTOREARGS_1 \
    "bpopl .L__X'%k2, %k2\n\t"
#  define RESTOREARGS_5 \
    "movl %3, %%ebx"
# endif
# define RESTOREARGS_2	RESTOREARGS_1
# define RESTOREARGS_3 \
    "xchgl %%edi, %%ebx\n\t"
# define RESTOREARGS_4	RESTOREARGS_3
#else
# define RESTOREARGS_1
# define RESTOREARGS_2
# define RESTOREARGS_3
# define RESTOREARGS_4
# define RESTOREARGS_5
#endif

#define ASMFMT_0()
#ifdef __PIC__
# define ASMFMT_1(arg1) \
	, "cd" (arg1)
# define ASMFMT_2(arg1, arg2) \
	, "d" (arg1), "c" (arg2)
# define ASMFMT_3(arg1, arg2, arg3) \
	, "D" (arg1), "c" (arg2), "d" (arg3)
# define ASMFMT_4(arg1, arg2, arg3, arg4) \
	, "D" (arg1), "c" (arg2), "d" (arg3), "S" (arg4)
# define ASMFMT_5(arg1, arg2, arg3, arg4, arg5) \
	, "0" (arg1), "m" (_xv), "c" (arg2), "d" (arg3), "S" (arg4), "D" (arg5)
#else
# define ASMFMT_1(arg1) \
	, "b" (arg1)
# define ASMFMT_2(arg1, arg2) \
	, "b" (arg1), "c" (arg2)
# define ASMFMT_3(arg1, arg2, arg3) \
	, "b" (arg1), "c" (arg2), "d" (arg3)
# define ASMFMT_4(arg1, arg2, arg3, arg4) \
	, "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4)
# define ASMFMT_5(arg1, arg2, arg3, arg4, arg5) \
	, "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4), "D" (arg5)
#endif

#define EXTRAVAR_0
#define EXTRAVAR_1
#define EXTRAVAR_2
#define EXTRAVAR_3
#define EXTRAVAR_4
#ifdef __PIC__
# define EXTRAVAR_5 int _xv;
#else
# define EXTRAVAR_5
#endif

/* Consistency check for position-independent code.  */
#ifdef __PIC__
# define check_consistency()						      \
  ({ int __res;								      \
     __asm__ __volatile__						      \
       ("call __i686.get_pc_thunk.cx;"					      \
	"addl $_GLOBAL_OFFSET_TABLE_, %%ecx;"				      \
	"subl %%ebx, %%ecx;"						      \
	"je 1f;"							      \
	"ud2;"								      \
	"1:\n"								      \
	".section .gnu.linkonce.t.__i686.get_pc_thunk.cx,\"ax\",@progbits;"   \
	".globl __i686.get_pc_thunk.cx;"				      \
	".hidden __i686.get_pc_thunk.cx;"				      \
	".type __i686.get_pc_thunk.cx,@function;"			      \
	"__i686.get_pc_thunk.cx:"					      \
	"movl (%%esp), %%ecx;"						      \
	"ret;"								      \
	".previous"							      \
	: "=c" (__res));						      \
     __res; })
#endif

#endif	/* __ASSEMBLER__ */


/* Pointer mangling support.  */
#if defined NOT_IN_libc && defined IS_IN_rtld
/* We cannot use the thread descriptor because in ld.so we use setjmp
   earlier than the descriptor is initialized.  Using a global variable
   is too complicated here since we have no PC-relative addressing mode.  */
#else
# ifdef __ASSEMBLER__
#  define PTR_MANGLE(reg)	xorl %gs:POINTER_GUARD, reg;		      \
				roll $9, reg
#  define PTR_DEMANGLE(reg)	rorl $9, reg;				      \
				xorl %gs:POINTER_GUARD, reg
# else
#  define PTR_MANGLE(var)	asm ("xorl %%gs:%c2, %0\n"		      \
				     "roll $9, %0"			      \
				     : "=r" (var)			      \
				     : "0" (var),			      \
				       "i" (offsetof (tcbhead_t,	      \
						      pointer_guard)))
#  define PTR_DEMANGLE(var)	asm ("rorl $9, %0\n"			      \
				     "xorl %%gs:%c2, %0"		      \
				     : "=r" (var)			      \
				     : "0" (var),			      \
				       "i" (offsetof (tcbhead_t,	      \
						      pointer_guard)))
# endif
#endif

#endif /* linux/i386/sysdep.h */
