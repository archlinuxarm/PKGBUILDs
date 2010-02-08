/* Copyright (C) 2000, 2002, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Alexandre Oliva <aoliva@redhat.com>
   based on the corresponding file in the mips port.

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

#ifndef _FENV_LIBC_H
#define _FENV_LIBC_H    1

/* Mask for enabling exceptions and for the CAUSE bits.  */
#define ENABLE_MASK	0x003E0U
#define CAUSE_MASK	0x07C00U
#define ROUND_MASK	0x30000U

/* Shift for FE_* flags to get up to the ENABLE bits and the CAUSE bits.  */
#define	ENABLE_SHIFT	5
#define	CAUSE_SHIFT	10

#endif /* _FENV_LIBC_H */
