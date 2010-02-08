/* e_scalbf.c -- float version of e_scalb.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: e_scalbf.c,v 1.3 1995/05/10 20:46:12 jtc Exp $";
#endif

#include <fenv.h>
#include <math.h>
#include "math_private.h"

#ifdef _SCALB_INT
#ifdef __STDC__
	float __ieee754_scalbf(float x, int fn)
#else
	float __ieee754_scalbf(x,fn)
	float x; int fn;
#endif
#else
#ifdef __STDC__
	float __ieee754_scalbf(float x, float fn)
#else
	float __ieee754_scalbf(x,fn)
	float x, fn;
#endif
#endif
{
#ifdef _SCALB_INT
	return __scalbnf(x,fn);
#else
	if (__isnanf(x)||__isnanf(fn)) return x*fn;
	if (!__finitef(fn)) {
	    if(fn>(float)0.0) return x*fn;
	    else if (x == 0)
	      return x;
	    else if (!__finitef (x))
	      {
# ifdef FE_INVALID
		feraiseexcept (FE_INVALID);
# endif
		return __nanf ("");
	      }
	    else       return x/(-fn);
	}
	if (__rintf(fn)!=fn)
	  {
# ifdef FE_INVALID
	    feraiseexcept (FE_INVALID);
# endif
	    return __nanf ("");
	  }
	if ( fn > (float)65000.0) return __scalbnf(x, 65000);
	if (-fn > (float)65000.0) return __scalbnf(x,-65000);
	return __scalbnf(x,(int)fn);
#endif
}
