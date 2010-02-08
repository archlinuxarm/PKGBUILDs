/* Adapted for use as nearbyint by Ulrich Drepper <drepper@cygnus.com>.  */
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

/*
 * rint(x)
 * Return x rounded to integral value according to the prevailing
 * rounding mode.
 * Method:
 *	Using floating addition.
 * Exception:
 *	Inexact flag raised if x not equal to rint(x).
 */

#include <fenv.h>
#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const double
#else
static double
#endif
TWO52[2]={
  4.50359962737049600000e+15, /* 0x43300000, 0x00000000 */
 -4.50359962737049600000e+15, /* 0xC3300000, 0x00000000 */
};

#ifdef __STDC__
	double __nearbyint(double x)
#else
	double __nearbyint(x)
	double x;
#endif
{
	fenv_t env;
	int64_t i0,sx;
	int32_t j0;
	EXTRACT_WORDS64(i0,x);
	sx = (i0>>63)&1;
	j0 = ((i0>>52)&0x7ff)-0x3ff;
	if(j0<52) {
	    if(j0<0) {
	      if((i0&UINT64_C(0x7fffffffffffffff))==0) return x;
		uint64_t i = i0 & UINT64_C(0xfffffffffffff);
		i0 &= UINT64_C(0xfffe000000000000);
		i0 |= (((i|-i) >> 12) & UINT64_C(0x8000000000000));
		INSERT_WORDS64(x,i0);
		feholdexcept (&env);
		double w = TWO52[sx]+x;
		double t =  w-TWO52[sx];
		fesetenv (&env);
		EXTRACT_WORDS64(i0,t);
		INSERT_WORDS64(t,(i0&UINT64_C(0x7fffffffffffffff))|(sx<<63));
		return t;
	    } else {
		uint64_t i = UINT64_C(0x000fffffffffffff)>>j0;
		if((i0&i)==0) return x; /* x is integral */
		i>>=1;
		if((i0&i)!=0)
		    i0 = (i0&(~i))|(UINT64_C(0x4000000000000)>>j0);
	    }
	} else {
	    if(j0==0x400) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	}
	INSERT_WORDS64(x,i0);
	feholdexcept (&env);
	double w = TWO52[sx]+x;
	double t = w-TWO52[sx];
	fesetenv (&env);
	return t;
}
weak_alias (__nearbyint, nearbyint)
#ifdef NO_LONG_DOUBLE
strong_alias (__nearbyint, __nearbyintl)
weak_alias (__nearbyint, nearbyintl)
#endif
