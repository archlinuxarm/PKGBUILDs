/* Header file for constants used in floating point <-> decimal conversions.
   Copyright (C) 1995, 1996, 1997, 1998, 1999, 2002, 2003
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

#ifndef _FPIOCONST_H
#define	_FPIOCONST_H

#include <float.h>
#include <math.h>
#include <gmp.h>


/* These values are used by __printf_fp, where they are noncritical (if the
   value is not large enough, it will just be slower); and by
   strtof/strtod/strtold, where it is critical (it's used for overflow
   detection).

   XXX These should be defined in <float.h>.  For the time being, we have the
   IEEE754 values here.  */

#if !defined __NO_LONG_DOUBLE_MATH && __LDBL_MAX_EXP__ > 1024
# define LDBL_MAX_10_EXP_LOG	12 /* = floor(log_2(LDBL_MAX_10_EXP)) */
#else
# define LDBL_MAX_10_EXP_LOG	8 /* = floor(log_2(LDBL_MAX_10_EXP)) */
#endif
#define DBL_MAX_10_EXP_LOG	8 /* = floor(log_2(DBL_MAX_10_EXP)) */
#define FLT_MAX_10_EXP_LOG	5 /* = floor(log_2(FLT_MAX_10_EXP)) */


/* The array with the number representation. */
extern const mp_limb_t __tens[] attribute_hidden;

/* Table of powers of ten.  This is used by __printf_fp and by
   strtof/strtod/strtold.  */
struct mp_power
  {
    size_t arrayoff;		/* Offset in `__tens'.  */
    mp_size_t arraysize;	/* Size of the array.  */
    int p_expo;			/* Exponent of the number 10^(2^i).  */
    int m_expo;			/* Exponent of the number 10^-(2^i-1).  */
  };
extern const struct mp_power _fpioconst_pow10[LDBL_MAX_10_EXP_LOG + 1]
     attribute_hidden;

/* The constants in the array `_fpioconst_pow10' have an offset.  */
#if BITS_PER_MP_LIMB == 32
# define _FPIO_CONST_OFFSET	2
#else
# define _FPIO_CONST_OFFSET	1
#endif


#endif	/* fpioconst.h */
