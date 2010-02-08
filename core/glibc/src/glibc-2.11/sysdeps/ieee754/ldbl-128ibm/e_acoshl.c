/* @(#)e_acosh.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: e_acosh.c,v 1.9 1995/05/12 04:57:18 jtc Exp $";
#endif

/* __ieee754_acosh(x)
 * Method :
 *	Based on
 *		acosh(x) = log [ x + sqrt(x*x-1) ]
 *	we have
 *		acosh(x) := log(x)+ln2,	if x is large; else
 *		acosh(x) := log(2x-1/(sqrt(x*x-1)+x)) if x>2; else
 *		acosh(x) := log1p(t+sqrt(2.0*t+t*t)); where t=x-1.
 *
 * Special cases:
 *	acosh(x) is NaN with signal if x<1.
 *	acosh(NaN) is NaN without signal.
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const long double
#else
static long double
#endif
one	= 1.0L,
ln2	= 6.93147180559945286227e-01L;  /* 0x3FE62E42, 0xFEFA39EF */

#ifdef __STDC__
	long double __ieee754_acoshl(long double x)
#else
	long double __ieee754_acoshl(x)
	long double x;
#endif
{
	long double t;
	int64_t hx;
	u_int64_t lx;
	GET_LDOUBLE_WORDS64(hx,lx,x);
	if(hx<0x3ff0000000000000LL) {		/* x < 1 */
	    return (x-x)/(x-x);
	} else if(hx >=0x41b0000000000000LL) {	/* x > 2**28 */
	    if(hx >=0x7ff0000000000000LL) {	/* x is inf of NaN */
	        return x+x;
	    } else
		return __ieee754_logl(x)+ln2;	/* acosh(huge)=log(2x) */
	} else if (((hx-0x3ff0000000000000LL)|(lx&0x7fffffffffffffffLL))==0) {
	    return 0.0;			/* acosh(1) = 0 */
	} else if (hx > 0x4000000000000000LL) {	/* 2**28 > x > 2 */
	    t=x*x;
	    return __ieee754_logl(2.0*x-one/(x+__ieee754_sqrtl(t-one)));
	} else {			/* 1<x<2 */
	    t = x-one;
	    return __log1p(t+__sqrtl(2.0*t+t*t));
	}
}
