/* s_asinhf.c -- float version of s_asinh.c.
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
static char rcsid[] = "$NetBSD: s_asinhf.c,v 1.5 1995/05/12 04:57:39 jtc Exp $";
#endif

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const float
#else
static float
#endif
one =  1.0000000000e+00, /* 0x3F800000 */
ln2 =  6.9314718246e-01, /* 0x3f317218 */
huge=  1.0000000000e+30;

#ifdef __STDC__
	float __asinhf(float x)
#else
	float __asinhf(x)
	float x;
#endif
{
	float t,w;
	int32_t hx,ix;
	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	if(ix>=0x7f800000) return x+x;	/* x is inf or NaN */
	if(ix< 0x38000000) {	/* |x|<2**-14 */
	    if(huge+x>one) return x;	/* return x inexact except 0 */
	}
	if(ix>0x47000000) {	/* |x| > 2**14 */
	    w = __ieee754_logf(fabsf(x))+ln2;
	} else if (ix>0x40000000) {	/* 2**14 > |x| > 2.0 */
	    t = fabsf(x);
	    w = __ieee754_logf((float)2.0*t+one/(__ieee754_sqrtf(x*x+one)+t));
	} else {		/* 2.0 > |x| > 2**-14 */
	    t = x*x;
	    w =__log1pf(fabsf(x)+t/(one+__ieee754_sqrtf(one+t)));
	}
	if(hx>0) return w; else return -w;
}
weak_alias (__asinhf, asinhf)
