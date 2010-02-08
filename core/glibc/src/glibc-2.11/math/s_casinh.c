/* Return arc hyperbole sine for double value.
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


__complex__ double
__casinh (__complex__ double x)
{
  __complex__ double res;
  int rcls = fpclassify (__real__ x);
  int icls = fpclassify (__imag__ x);

  if (rcls <= FP_INFINITE || icls <= FP_INFINITE)
    {
      if (icls == FP_INFINITE)
	{
	  __real__ res = __copysign (HUGE_VAL, __real__ x);

	  if (rcls == FP_NAN)
	    __imag__ res = __nan ("");
	  else
	    __imag__ res = __copysign (rcls >= FP_ZERO ? M_PI_2 : M_PI_4,
				       __imag__ x);
	}
      else if (rcls <= FP_INFINITE)
	{
	  __real__ res = __real__ x;
	  if ((rcls == FP_INFINITE && icls >= FP_ZERO)
	      || (rcls == FP_NAN && icls == FP_ZERO))
	    __imag__ res = __copysign (0.0, __imag__ x);
	  else
	    __imag__ res = __nan ("");
	}
      else
	{
	  __real__ res = __nan ("");
	  __imag__ res = __nan ("");
	}
    }
  else if (rcls == FP_ZERO && icls == FP_ZERO)
    {
      res = x;
    }
  else
    {
      __complex__ double y;

      __real__ y = (__real__ x - __imag__ x) * (__real__ x + __imag__ x) + 1.0;
      __imag__ y = 2.0 * __real__ x * __imag__ x;

      y = __csqrt (y);

      __real__ y += __real__ x;
      __imag__ y += __imag__ x;

      res = __clog (y);
    }

  return res;
}
weak_alias (__casinh, casinh)
#ifdef NO_LONG_DOUBLE
strong_alias (__casinh, __casinhl)
weak_alias (__casinh, casinhl)
#endif
