/* e_acoshl.c -- long double version of e_acosh.c.
 * Conversion to long double by Jakub Jelinek, jj@ultra.linux.cz.
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

/* __ieee754_acoshl(x)
 * Method :
 *	Based on
 *		acoshl(x) = logl [ x + sqrtl(x*x-1) ]
 *	we have
 *		acoshl(x) := logl(x)+ln2,	if x is large; else
 *		acoshl(x) := logl(2x-1/(sqrtl(x*x-1)+x)) if x>2; else
 *		acoshl(x) := log1pl(t+sqrtl(2.0*t+t*t)); where t=x-1.
 *
 * Special cases:
 *	acoshl(x) is NaN with signal if x<1.
 *	acoshl(NaN) is NaN without signal.
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const long double
#else
static long double
#endif
one	= 1.0,
ln2	= 0.6931471805599453094172321214581766L;

#ifdef __STDC__
	long double __ieee754_acoshl(long double x)
#else
	long double __ieee754_acoshl(x)
	long double x;
#endif
{
	long double t;
	u_int64_t lx;
	int64_t hx;
	GET_LDOUBLE_WORDS64(hx,lx,x);
	if(hx<0x3fff000000000000LL) {		/* x < 1 */
	    return (x-x)/(x-x);
	} else if(hx >=0x4035000000000000LL) {	/* x > 2**54 */
	    if(hx >=0x7fff000000000000LL) {	/* x is inf of NaN */
	        return x+x;
	    } else
		return __ieee754_logl(x)+ln2;	/* acoshl(huge)=logl(2x) */
	} else if(((hx-0x3fff000000000000LL)|lx)==0) {
	    return 0.0L;			/* acosh(1) = 0 */
	} else if (hx > 0x4000000000000000LL) {	/* 2**28 > x > 2 */
	    t=x*x;
	    return __ieee754_logl(2.0L*x-one/(x+__ieee754_sqrtl(t-one)));
	} else {			/* 1<x<2 */
	    t = x-one;
	    return __log1pl(t+__sqrtl(2.0L*t+t*t));
	}
}
