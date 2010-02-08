/* Complex tangent function for long double.  IBM extended format version.
   Copyright (C) 1997,2005,2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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

#include <complex.h>
#include <fenv.h>
#include <math.h>
#include <math_ldbl_opt.h>

#include "math_private.h"


__complex__ long double
__ctanl (__complex__ long double x)
{
  __complex__ long double res;

  if (!isfinite (__real__ x) || !isfinite (__imag__ x))
    {
      if (__isinfl (__imag__ x))
	{
	  __real__ res = __copysignl (0.0, __real__ x);
	  __imag__ res = __copysignl (1.0, __imag__ x);
	}
      else if (__real__ x == 0.0)
	{
	  res = x;
	}
      else
	{
	  __real__ res = __nanl ("");
	  __imag__ res = __nanl ("");

#ifdef FE_INVALID
	  if (__isinfl (__real__ x))
	    feraiseexcept (FE_INVALID);
#endif
	}
    }
  else
    {
      long double sin2rx, cos2rx;
      long double den;

      __sincosl (2.0 * __real__ x, &sin2rx, &cos2rx);

      den = cos2rx + __ieee754_coshl (2.0 * __imag__ x);


      if (den == 0.0)
	{
	  __complex__ long double ez = __cexpl (1.0i * x);
	  __complex__ long double emz = __cexpl (-1.0i * x);

	  res = (ez - emz) / (ez + emz) * -1.0i;
	}
      else
	{
	  __real__ res = sin2rx / den;
	  __imag__ res = __ieee754_sinhl (2.0 * __imag__ x) / den;
	}
      /* __gcc_qmul does not respect -0.0 so we need the following fixup.  */
      if ((__real__ res == 0.0) && (__real__ x == 0.0))
        __real__ res = __real__ x;

      if ((__real__ res == 0.0) && (__imag__ x == 0.0))
        __imag__ res = __imag__ x;
    }

  return res;
}
long_double_symbol (libm, __ctanl, ctanl);
