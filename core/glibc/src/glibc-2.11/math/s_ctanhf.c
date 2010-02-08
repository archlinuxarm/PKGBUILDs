/* Complex hyperbole tangent for float.
   Copyright (C) 1997, 2005 Free Software Foundation, Inc.
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

#include "math_private.h"


__complex__ float
__ctanhf (__complex__ float x)
{
  __complex__ float res;

  if (!isfinite (__real__ x) || !isfinite (__imag__ x))
    {
      if (__isinff (__real__ x))
	{
	  __real__ res = __copysignf (1.0, __real__ x);
	  __imag__ res = __copysignf (0.0, __imag__ x);
	}
      else if (__imag__ x == 0.0)
	{
	  res = x;
	}
      else
	{
	  __real__ res = __nanf ("");
	  __imag__ res = __nanf ("");

#ifdef FE_INVALID
	  if (__isinff (__imag__ x))
	    feraiseexcept (FE_INVALID);
#endif
	}
    }
  else
    {
      float sin2ix, cos2ix;
      float den;

      __sincosf (2.0 * __imag__ x, &sin2ix, &cos2ix);

      den = (__ieee754_coshf (2.0 * __real__ x) + cos2ix);

      if (den == 0.0f)
	{
	  __complex__ float ez = __cexpf (x);
	  __complex__ float emz = __cexpf (-x);

	  res = (ez - emz) / (ez + emz);
	}
      else
	{
	  __real__ res = __ieee754_sinhf (2.0 * __real__ x) / den;
	  __imag__ res = sin2ix / den;
	}
    }

  return res;
}
#ifndef __ctanhf
weak_alias (__ctanhf, ctanhf)
#endif
