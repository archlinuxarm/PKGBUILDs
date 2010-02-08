/* s_frexpl.c -- long double version of s_frexp.c.
 * Conversion to IEEE quad long double by Jakub Jelinek, jj@ultra.linux.cz.
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
static char rcsid[] = "$NetBSD: $";
#endif

/*
 * for non-zero x
 *	x = frexpl(arg,&exp);
 * return a long double fp quantity x such that 0.5 <= |x| <1.0
 * and the corresponding binary exponent "exp". That is
 *	arg = x*2^exp.
 * If arg is inf, 0.0, or NaN, then frexpl(arg,&exp) returns arg
 * with *exp=0.
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const long double
#else
static long double
#endif
two114 = 2.0769187434139310514121985316880384E+34L; /* 0x4071000000000000, 0 */

#ifdef __STDC__
	long double __frexpl(long double x, int *eptr)
#else
	long double __frexpl(x, eptr)
	long double x; int *eptr;
#endif
{
	u_int64_t hx, lx, ix;
	GET_LDOUBLE_WORDS64(hx,lx,x);
	ix = 0x7fffffffffffffffULL&hx;
	*eptr = 0;
	if(ix>=0x7fff000000000000ULL||((ix|lx)==0)) return x;	/* 0,inf,nan */
	if (ix<0x0001000000000000ULL) {		/* subnormal */
	    x *= two114;
	    GET_LDOUBLE_MSW64(hx,x);
	    ix = hx&0x7fffffffffffffffULL;
	    *eptr = -114;
	}
	*eptr += (ix>>48)-16382;
	hx = (hx&0x8000ffffffffffffULL) | 0x3ffe000000000000ULL;
	SET_LDOUBLE_MSW64(x,hx);
	return x;
}
weak_alias (__frexpl, frexpl)
