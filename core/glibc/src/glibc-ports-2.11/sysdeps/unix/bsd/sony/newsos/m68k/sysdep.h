/* Copyright (C) 1993, 1994, 1995, 1997 Free Software Foundation, Inc.
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

#include <sysdeps/unix/sysdep.h>

#ifdef __ASSEMBLER__

#define	POUND	#

#ifdef	__STDC__
#define	ENTRY(name)							      \
  .globl _##name;							      \
  .even;								      \
  _##name##:
#else
#define	ENTRY(name)							      \
  .globl _/**/name;							      \
  .even;								      \
  _/**/name/**/:
#endif

#define	PSEUDO(name, syscall_name, args)				      \
  .even;								      \
  .globl syscall_error;							      \
  error: jmp syscall_error;						      \
  ENTRY (name)								      \
  DO_CALL (POUND SYS_ify (syscall_name), args)

#define DO_CALL(syscall, args)						      \
  movel syscall, d0;							      \
  linkw a6, POUND(0);							      \
  trap POUND(0);							      \
  unlk a6;								      \
  bcs error

#define	ret	rts
#define	r0	d0
#define	r1	d1
#define	MOVE(x,y)	movel x , y

#endif
