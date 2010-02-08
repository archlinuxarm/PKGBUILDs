/* Return arc hyperbole tangent for float value.
   Copyright (C) 1997 Free Software Foundation, Inc.
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
#include <math.h>

#include "math_private.h"


__complex__ float
__catanhf (__complex__ float x)
{
  __complex__ float res;
  int rcls = fpclassify (__real__ x);
  int icls = fpclassify (__imag__ x);

  if (rcls <= FP_INFINITE || icls <= FP_INFINITE)
    {
      if (icls == FP_INFINITE)
	{
	  __real__ res = __copysignf (0.0, __real__ x);
	  __imag__ res = __copysignf (M_PI_2, __imag__ x);
	}
      else if (rcls == FP_INFINITE || rcls == FP_ZERO)
	{
	  __real__ res = __copysignf (0.0, __real__ x);
	  if (icls >= FP_ZERO)
	    __imag__ res = __copysignf (M_PI_2, __imag__ x);
	  else
	    __imag__ res = __nanf ("");
	}
      else
	{
	  __real__ res = __nanf ("");
	  __imag__ res = __nanf ("");
	}
    }
  else if (rcls == FP_ZERO && icls == FP_ZERO)
    {
      res = x;
    }
  else
    {
      float i2, num, den;

      i2 = __imag__ x * __imag__ x;

      num = 1.0 + __real__ x;
      num = i2 + num * num;

      den = 1.0 - __real__ x;
      den = i2 + den * den;

      __real__ res = 0.25 * (__ieee754_logf (num) - __ieee754_logf (den));

      den = 1 - __real__ x * __real__ x - i2;

      __imag__ res = 0.5 * __ieee754_atan2f (2.0 * __imag__ x, den);
    }

  return res;
}
#ifndef __catanhf
weak_alias (__catanhf, catanhf)
#endif
