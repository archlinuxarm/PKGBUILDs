/* Copyright (C) 1999, 2006, 2007 Free Software Foundation, Inc.
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

#include <math.h>
#include <stdlib.h>
#include <wchar.h>
#include <xlocale.h>

/* The actual implementation for all floating point sizes is in strtod.c.
   These macros tell it to produce the `long double' version, `strtold'.  */

#define FLOAT		long double
#define FLT		LDBL
#ifdef USE_WIDE_CHAR
extern long double ____new_wcstold_l (const wchar_t *, wchar_t **, __locale_t);
# define STRTOF		__new_wcstold_l
# define __STRTOF	____new_wcstold_l
# define ____STRTOF_INTERNAL ____wcstold_l_internal
#else
extern long double ____new_strtold_l (const char *, char **, __locale_t);
# define STRTOF		__new_strtold_l
# define __STRTOF	____new_strtold_l
# define ____STRTOF_INTERNAL ____strtold_l_internal
#endif
extern __typeof (__STRTOF) STRTOF;
libc_hidden_proto (__STRTOF)
libc_hidden_proto (STRTOF)
#define MPN2FLOAT	__mpn_construct_long_double
#define FLOAT_HUGE_VAL	HUGE_VALL
# define SET_MANTISSA(flt, mant) \
  do { union ibm_extended_long_double u;				      \
       u.d = (flt);							      \
       if ((mant & 0xfffffffffffffULL) == 0)				      \
	 mant = 0x8000000000000ULL;					      \
       u.ieee.mantissa0 = ((mant) >> 32) & 0xfffff;			      \
       u.ieee.mantissa1 = (mant) & 0xffffffff;				      \
       (flt) = u.d;							      \
  } while (0)

#include <strtod_l.c>

#ifdef __LONG_DOUBLE_MATH_OPTIONAL
# include <math_ldbl_opt.h>
# ifdef USE_WIDE_CHAR
weak_alias (____new_wcstold_l, ___new_wcstold_l);
long_double_symbol (libc, ___new_wcstold_l, wcstold_l);
long_double_symbol (libc, ____new_wcstold_l, __wcstold_l);
# else
weak_alias (____new_strtold_l, ___new_strtold_l);
long_double_symbol (libc, ___new_strtold_l, strtold_l);
long_double_symbol (libc, ____new_strtold_l, __strtold_l);
# endif
#endif
