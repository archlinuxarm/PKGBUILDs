/* w_scalbf.c -- float version of w_scalb.c.
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
static char rcsid[] = "$NetBSD: w_scalbf.c,v 1.3 1995/05/10 20:49:50 jtc Exp $";
#endif

/*
 * wrapper scalbf(float x, float fn) is provide for
 * passing various standard test suite. One
 * should use scalbn() instead.
 */

#include <math.h>
#include "math_private.h"

#include <errno.h>

#ifdef __STDC__
#ifdef _SCALB_INT
	float __scalbf(float x, int fn)		/* wrapper scalbf */
#else
	float __scalbf(float x, float fn)		/* wrapper scalbf */
#endif
#else
	float __scalbf(x,fn)			/* wrapper scalbf */
#ifdef _SCALB_INT
	float x; int fn;
#else
	float x,fn;
#endif
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_scalbf(x,fn);
#else
	float z;
	z = __ieee754_scalbf(x,fn);
	if(_LIB_VERSION != _SVID_) return z;
	if(!(__finitef(z)||__isnanf(z))&&__finitef(x)) {
	    /* scalbf overflow */
	    return (float)__kernel_standard((double)x,(double)fn,132);
	}
	if(z==(float)0.0&&z!=x) {
	    /* scalbf underflow */
	    return (float)__kernel_standard((double)x,(double)fn,133);
	}
#ifndef _SCALB_INT
	if(!__finitef(fn)) __set_errno (ERANGE);
#endif
	return z;
#endif
}
weak_alias (__scalbf, scalbf)
