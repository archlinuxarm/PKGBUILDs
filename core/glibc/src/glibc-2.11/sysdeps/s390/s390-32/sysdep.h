/* Assembler macros for s390.
   Copyright (C) 2000, 2001, 2003 Free Software Foundation, Inc.
   Contributed by Martin Schwidefsky (schwidefsky@de.ibm.com).
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

#include <sysdeps/generic/sysdep.h>

#ifdef	__ASSEMBLER__

/* Syntactic details of assembler.  */

#ifdef HAVE_ELF

/* ELF uses byte-counts for .align, most others use log2 of count of bytes.  */
#define ALIGNARG(log2) 1<<log2
/* For ELF we need the `.type' directive to make shared libs work right.  */
#define ASM_TYPE_DIRECTIVE(name,typearg) .type name,typearg;
#define ASM_SIZE_DIRECTIVE(name) .size name,.-name;

/* In ELF C symbols are asm symbols.  */
#undef	NO_UNDERSCORES
#define NO_UNDERSCORES

#else

#define ALIGNARG(log2) log2
#define ASM_TYPE_DIRECTIVE(name,type)	/* Nothing is specified.  */
#define ASM_SIZE_DIRECTIVE(name)	/* Nothing is specified.  */

#endif


/* Define an entry point visible from C. */
#define	ENTRY(name)							      \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);				      \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)			      \
  .align ALIGNARG(2);							      \
  C_LABEL(name)								      \
  cfi_startproc;							      \
  CALL_MCOUNT

#undef	END
#define END(name)							      \
  cfi_endproc;								      \
  ASM_SIZE_DIRECTIVE(name)						      \

/* If compiled for profiling, call `mcount' at the start of each function.  */
#ifdef	PROF
#ifdef PIC
#define CALL_MCOUNT \
  lr 0,14 ; bras 14,.+12 ; .long _GLOBAL_OFFSET_TABLE_ - . ; .long 0f-. ; \
  lr 1,14 ; al 1,4(14) ; al 14,0(14) ; l 14,_mcount@GOT(14) ; \
  basr 14,14 ; lr 14,0 ; .data ; .align 4 ; 0: .long 0 ; .text ;
#else
#define CALL_MCOUNT \
  lr 0,14 ; bras 14,.+12 ; .long _mcount ; .long 0f ; \
  l 1,4(14) ; l 14,0(14) ; basr 14,14 ; lr 14,0 ; \
  .data ; .align 4 ; 0: .long 0 ; .text ;
#endif
#else
#define CALL_MCOUNT		/* Do nothing.  */
#endif

#ifdef	NO_UNDERSCORES
/* Since C identifiers are not normally prefixed with an underscore
   on this system, the asm identifier `syscall_error' intrudes on the
   C name space.  Make sure we use an innocuous name.  */
#define	syscall_error	__syscall_error
#define mcount		_mcount
#endif

#define	PSEUDO(name, syscall_name, args) \
lose: SYSCALL_PIC_SETUP				\
    basr %r1,0;					\
0:  al %r1,1f-0b(%r1);				\
    br %r1;					\
1:  .long JUMPTARGET(syscall_error) - 0b;	\
    .globl syscall_error;			\
    ENTRY (name)				\
    DO_CALL (syscall_name, args);		\
    jm lose

#undef	PSEUDO_END
#define	PSEUDO_END(name)						      \
  END (name)

#undef JUMPTARGET
#ifdef PIC
#define JUMPTARGET(name)	name##@PLT
#define SYSCALL_PIC_SETUP             \
    bras  %r12,1f                     \
0:  .long _GLOBAL_OFFSET_TABLE_-0b    \
1:  al    %r12,0(%r12)
#else
#define JUMPTARGET(name)   	name
#define SYSCALL_PIC_SETUP	/* Nothing.  */
#endif

/* Local label name for asm code. */
#ifndef L
#define L(name)		.L##name
#endif

#endif	/* __ASSEMBLER__ */
