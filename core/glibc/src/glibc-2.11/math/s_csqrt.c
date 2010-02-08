/* Complex square root of double value.
   Copyright (C) 1997, 1998, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Based on an algorithm by Stephen L. Moshier <moshier@world.std.com>.
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


__complex__ double
__csqrt (__complex__ double x)
{
  __complex__ double res;
  int rcls = fpclassify (__real__ x);
  int icls = fpclassify (__imag__ x);

  if (rcls <= FP_INFINITE || icls <= FP_INFINITE)
    {
      if (icls == FP_INFINITE)
	{
	  __real__ res = HUGE_VAL;
	  __imag__ res = __imag__ x;
	}
      else if (rcls == FP_INFINITE)
	{
	  if (__real__ x < 0.0)
	    {
	      __real__ res = icls == FP_NAN ? __nan ("") : 0;
	      __imag__ res = __copysign (HUGE_VAL, __imag__ x);
	    }
	  else
	    {
	      __real__ res = __real__ x;
	      __imag__ res = (icls == FP_NAN
			      ? __nan ("") : __copysign (0.0, __imag__ x));
	    }
	}
      else
	{
	  __real__ res = __nan ("");
	  __imag__ res = __nan ("");
	}
    }
  else
    {
      if (icls == FP_ZERO)
	{
	  if (__real__ x < 0.0)
	    {
	      __real__ res = 0.0;
	      __imag__ res = __copysign (__ieee754_sqrt (-__real__ x),
					 __imag__ x);
	    }
	  else
	    {
	      __real__ res = fabs (__ieee754_sqrt (__real__ x));
	      __imag__ res = __copysign (0.0, __imag__ x);
	    }
	}
      else if (rcls == FP_ZERO)
	{
	  double r = __ieee754_sqrt (0.5 * fabs (__imag__ x));

	  __real__ res = r;
	  __imag__ res = __copysign (r, __imag__ x);
	}
      else
	{
	  double d, r, s;

	  d = __ieee754_hypot (__real__ x, __imag__ x);
	  /* Use the identity   2  Re res  Im res = Im x
	     to avoid cancellation error in  d +/- Re x.  */
	  if (__real__ x > 0)
	    {
	      r = __ieee754_sqrt (0.5 * d + 0.5 * __real__ x);
	      s = (0.5 * __imag__ x) / r;
	    }
	  else
	    {
	      s = __ieee754_sqrt (0.5 * d - 0.5 * __real__ x);
	      r = fabs ((0.5 * __imag__ x) / s);
	    }

	  __real__ res = r;
	  __imag__ res = __copysign (s, __imag__ x);
	}
    }

  return res;
}
weak_alias (__csqrt, csqrt)
#ifdef NO_LONG_DOUBLE
strong_alias (__csqrt, __csqrtl)
weak_alias (__csqrt, csqrtl)
#endif
