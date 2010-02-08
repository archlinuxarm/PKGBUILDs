/* Assembler macros for m68k.
   Copyright (C) 1998, 2003 Free Software Foundation, Inc.
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

#ifdef __ASSEMBLER__

/* Syntactic details of assembler.  */

# ifdef HAVE_ELF

/* ELF uses byte-counts for .align, most others use log2 of count of bytes.  */
#  define ALIGNARG(log2) 1<<log2
/* For ELF we need the `.type' directive to make shared libs work right.  */
#  define ASM_TYPE_DIRECTIVE(name,typearg) .type name,typearg
#  define ASM_SIZE_DIRECTIVE(name) .size name,.-name

/* In ELF C symbols are asm symbols.  */
#  undef NO_UNDERSCORES
#  define NO_UNDERSCORES

# else

#  define ALIGNARG(log2) log2
#  define ASM_TYPE_DIRECTIVE(name,type)	/* Nothing is specified.  */
#  define ASM_SIZE_DIRECTIVE(name)	/* Nothing is specified.  */

# endif


/* Define an entry point visible from C.

   There is currently a bug in gdb which prevents us from specifying
   incomplete stabs information.  Fake some entries here which specify
   the current source file.  */
# define ENTRY(name)							      \
  .globl C_SYMBOL_NAME(name);						      \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function);			      \
  .align ALIGNARG(2);							      \
  C_LABEL(name)								      \
  CALL_MCOUNT

# undef END
# define END(name) ASM_SIZE_DIRECTIVE(name)


/* If compiled for profiling, call `_mcount' at the start of each function.  */
# ifdef	PROF
/* The mcount code relies on a normal frame pointer being on the stack
   to locate our caller, so push one just for its benefit.  */
#  define CALL_MCOUNT \
  move.l %fp, -(%sp); move.l %sp, %fp;					      \
  jbsr JUMPTARGET (mcount);						      \
  move.l (%sp)+, %fp;
# else
#  define CALL_MCOUNT		/* Do nothing.  */
# endif

# ifdef	NO_UNDERSCORES
/* Since C identifiers are not normally prefixed with an underscore
   on this system, the asm identifier `syscall_error' intrudes on the
   C name space.  Make sure we use an innocuous name.  */
#  define syscall_error	__syscall_error
#  define mcount	_mcount
# endif

# define PSEUDO(name, syscall_name, args)				      \
  .globl syscall_error;							      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args);					      \
    jcc JUMPTARGET(syscall_error)

# undef PSEUDO_END
# define PSEUDO_END(name)						      \
  END (name)

# undef JUMPTARGET
# ifdef PIC
#  define JUMPTARGET(name)	name##@PLTPC
# else
#  define JUMPTARGET(name)	name
# endif

/* Perform operation OP with PC-relative SRC as the first operand and
   DST as the second.  TMP is available as a temporary if needed.  */
#ifdef __mcoldfire__
#define PCREL_OP(OP, SRC, DST, TMP) \
  move.l &SRC - ., TMP; OP (-8, %pc, TMP), DST
#else
#define PCREL_OP(OP, SRC, DST, TMP) \
  OP SRC(%pc), DST
#endif

#else

/* As above, but PC is the spelling of the PC register.  We need this
   so that the macro can be used in both normal and extended asms.  */
#ifdef __mcoldfire__
#define PCREL_OP(OP, SRC, DST, TMP, PC) \
  "move.l #" SRC " - ., " TMP "\n\t" OP " (-8, " PC ", " TMP "), " DST
#else
#define PCREL_OP(OP, SRC, DST, TMP, PC) \
  OP " " SRC "(" PC "), " DST
#endif

#endif	/* __ASSEMBLER__ */
