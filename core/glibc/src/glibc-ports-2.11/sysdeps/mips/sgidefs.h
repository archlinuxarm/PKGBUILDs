/* Copyright (C) 1996, 1997, 1998, 2003, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ralf Baechle <ralf@gnu.org>.

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

#ifndef _SGIDEFS_H
#define _SGIDEFS_H	1

/*
 * A crude hack to stop <asm/sgidefs.h>
 */
#undef __ASM_SGIDEFS_H
#define __ASM_SGIDEFS_H

/*
 * And remove any damage it might have already done
 */
#undef _MIPS_ISA_MIPS1
#undef _MIPS_ISA_MIPS2
#undef _MIPS_ISA_MIPS3
#undef _MIPS_ISA_MIPS4
#undef _MIPS_ISA_MIPS5
#undef _MIPS_ISA_MIPS32
#undef _MIPS_ISA_MIPS64

#undef _MIPS_SIM_ABI32
#undef _MIPS_SIM_NABI32
#undef _MIPS_SIM_ABI64

/*
 * Definitions for the ISA level
 */
#define _MIPS_ISA_MIPS1 1
#define _MIPS_ISA_MIPS2 2
#define _MIPS_ISA_MIPS3 3
#define _MIPS_ISA_MIPS4 4
#define _MIPS_ISA_MIPS5 5
#define _MIPS_ISA_MIPS32 6
#define _MIPS_ISA_MIPS64 7

/*
 * Subprogram calling convention
 */
#ifndef _ABIO32
# define _ABIO32		1
#endif
#define _MIPS_SIM_ABI32		_ABIO32

#ifndef _ABIN32
# define _ABIN32		2
#endif
#define _MIPS_SIM_NABI32	_ABIN32

#ifndef _ABI64
# define _ABI64			3
#endif
#define _MIPS_SIM_ABI64		_ABI64

#endif /* sgidefs.h */
