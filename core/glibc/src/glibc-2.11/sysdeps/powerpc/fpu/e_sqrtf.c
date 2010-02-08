/* Single-precision floating point square root.
   Copyright (C) 1997, 2003, 2004, 2008 Free Software Foundation, Inc.
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
#include <math_private.h>
#include <fenv_libc.h>
#include <inttypes.h>

#include <sysdep.h>
#include <ldsodefs.h>

static const float almost_half = 0.50000006;	/* 0.5 + 2^-24 */
static const ieee_float_shape_type a_nan = {.word = 0x7fc00000 };
static const ieee_float_shape_type a_inf = {.word = 0x7f800000 };
static const float two48 = 281474976710656.0;
static const float twom24 = 5.9604644775390625e-8;
extern const float __t_sqrt[1024];

/* The method is based on a description in
   Computation of elementary functions on the IBM RISC System/6000 processor,
   P. W. Markstein, IBM J. Res. Develop, 34(1) 1990.
   Basically, it consists of two interleaved Newton-Rhapson approximations,
   one to find the actual square root, and one to find its reciprocal
   without the expense of a division operation.   The tricky bit here
   is the use of the POWER/PowerPC multiply-add operation to get the
   required accuracy with high speed.

   The argument reduction works by a combination of table lookup to
   obtain the initial guesses, and some careful modification of the
   generated guesses (which mostly runs on the integer unit, while the
   Newton-Rhapson is running on the FPU).  */

#ifdef __STDC__
float
__slow_ieee754_sqrtf (float x)
#else
float
__slow_ieee754_sqrtf (x)
     float x;
#endif
{
  const float inf = a_inf.value;

  if (x > 0)
    {
      if (x != inf)
	{
	  /* Variables named starting with 's' exist in the
	     argument-reduced space, so that 2 > sx >= 0.5,
	     1.41... > sg >= 0.70.., 0.70.. >= sy > 0.35... .
	     Variables named ending with 'i' are integer versions of
	     floating-point values.  */
	  float sx;		/* The value of which we're trying to find the square
				   root.  */
	  float sg, g;		/* Guess of the square root of x.  */
	  float sd, d;		/* Difference between the square of the guess and x.  */
	  float sy;		/* Estimate of 1/2g (overestimated by 1ulp).  */
	  float sy2;		/* 2*sy */
	  float e;		/* Difference between y*g and 1/2 (note that e==se).  */
	  float shx;		/* == sx * fsg */
	  float fsg;		/* sg*fsg == g.  */
	  fenv_t fe;		/* Saved floating-point environment (stores rounding
				   mode and whether the inexact exception is
				   enabled).  */
	  uint32_t xi, sxi, fsgi;
	  const float *t_sqrt;

	  GET_FLOAT_WORD (xi, x);
	  fe = fegetenv_register ();
	  relax_fenv_state ();
	  sxi = (xi & 0x3fffffff) | 0x3f000000;
	  SET_FLOAT_WORD (sx, sxi);
	  t_sqrt = __t_sqrt + (xi >> (23 - 8 - 1) & 0x3fe);
	  sg = t_sqrt[0];
	  sy = t_sqrt[1];

	  /* Here we have three Newton-Rhapson iterations each of a
	     division and a square root and the remainder of the
	     argument reduction, all interleaved.   */
	  sd = -(sg * sg - sx);
	  fsgi = (xi + 0x40000000) >> 1 & 0x7f800000;
	  sy2 = sy + sy;
	  sg = sy * sd + sg;	/* 16-bit approximation to sqrt(sx). */
	  e = -(sy * sg - almost_half);
	  SET_FLOAT_WORD (fsg, fsgi);
	  sd = -(sg * sg - sx);
	  sy = sy + e * sy2;
	  if ((xi & 0x7f800000) == 0)
	    goto denorm;
	  shx = sx * fsg;
	  sg = sg + sy * sd;	/* 32-bit approximation to sqrt(sx),
				   but perhaps rounded incorrectly.  */
	  sy2 = sy + sy;
	  g = sg * fsg;
	  e = -(sy * sg - almost_half);
	  d = -(g * sg - shx);
	  sy = sy + e * sy2;
	  fesetenv_register (fe);
	  return g + sy * d;
	denorm:
	  /* For denormalised numbers, we normalise, calculate the
	     square root, and return an adjusted result.  */
	  fesetenv_register (fe);
	  return __slow_ieee754_sqrtf (x * two48) * twom24;
	}
    }
  else if (x < 0)
    {
      /* For some reason, some PowerPC32 processors don't implement
         FE_INVALID_SQRT.  */
#ifdef FE_INVALID_SQRT
      feraiseexcept (FE_INVALID_SQRT);

      fenv_union_t u = { .fenv = fegetenv_register () };
      if ((u.l[1] & FE_INVALID) == 0)
#endif
	feraiseexcept (FE_INVALID);
      x = a_nan.value;
    }
  return f_washf (x);
}


#ifdef __STDC__
float
__ieee754_sqrtf (float x)
#else
float
__ieee754_sqrtf (x)
     float x;
#endif
{
  double z;

  /* If the CPU is 64-bit we can use the optional FP instructions.  */
  if (__CPU_HAS_FSQRT)
    {
      /* Volatile is required to prevent the compiler from moving the
         fsqrt instruction above the branch.  */
      __asm __volatile ("	fsqrts	%0,%1\n"
				:"=f" (z):"f" (x));
    }
  else
    z = __slow_ieee754_sqrtf (x);

  return z;
}
