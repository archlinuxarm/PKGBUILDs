/* Copyright (C) 2000, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Richard Henderson.

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
#include <math_ldbl_opt.h>

double
__copysign (double x, double y)
{
  __asm ("cpys %1, %2, %0" : "=f" (x) : "f" (y), "f" (x));
  return x;
}

weak_alias (__copysign, copysign)
#ifdef NO_LONG_DOUBLE
strong_alias (__copysign, __copysignl)
weak_alias (__copysign, copysignl)
#endif
#ifdef IS_IN_libm
# if LONG_DOUBLE_COMPAT(libm, GLIBC_2_0)
compat_symbol (libm, __copysign, copysignl, GLIBC_2_0);
# endif
#elif LONG_DOUBLE_COMPAT(libc, GLIBC_2_0)
compat_symbol (libc, __copysign, copysignl, GLIBC_2_0);
#endif
