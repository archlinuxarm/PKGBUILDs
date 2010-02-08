/* FPU control word definitions.  ARM VFP version.
   Copyright (C) 2004, 2005, 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.  */

#ifndef _FPU_CONTROL_H
#define _FPU_CONTROL_H

#if !defined(_LIBC) && defined(__SOFTFP__)

#define _FPU_RESERVED 0xffffffff
#define _FPU_DEFAULT  0x00000000
typedef unsigned int fpu_control_t;
#define _FPU_GETCW(cw) 0
#define _FPU_SETCW(cw) do { } while (0)
extern fpu_control_t __fpu_control;

#else

/* masking of interrupts */
#define _FPU_MASK_IM	0x00000100	/* invalid operation */
#define _FPU_MASK_ZM	0x00000200	/* divide by zero */
#define _FPU_MASK_OM	0x00000400	/* overflow */
#define _FPU_MASK_UM	0x00000800	/* underflow */
#define _FPU_MASK_PM	0x00001000	/* inexact */

/* Some bits in the FPSCR are not yet defined.  They must be preserved when
   modifying the contents.  */
#define _FPU_RESERVED	0x0e08e0e0
#define _FPU_DEFAULT    0x00000000
/* Default + exceptions enabled. */
#define _FPU_IEEE	(_FPU_DEFAULT | 0x00001f00)

/* Type of the control word.  */
typedef unsigned int fpu_control_t;

/* Macros for accessing the hardware control word.  */
/* This is fmrx %0, fpscr.  */
#define _FPU_GETCW(cw) \
  __asm__ __volatile__ ("mrc p10, 7, %0, cr1, cr0, 0" : "=r" (cw))
/* This is fmxr fpscr, %0.  */
#define _FPU_SETCW(cw) \
  __asm__ __volatile__ ("mcr p10, 7, %0, cr1, cr0, 0" : : "r" (cw))

/* Default control word set at startup.  */
extern fpu_control_t __fpu_control;

#endif /* __SOFTFP__ */

#endif /* _FPU_CONTROL_H */
