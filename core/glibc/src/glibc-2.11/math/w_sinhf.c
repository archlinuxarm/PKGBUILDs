/* w_sinhf.c -- float version of w_sinh.c.
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
static char rcsid[] = "$NetBSD: w_sinhf.c,v 1.3 1995/05/10 20:49:54 jtc Exp $";
#endif

/* 
 * wrapper sinhf(x)
 */

#include <math.h>
#include "math_private.h"

#ifdef __STDC__
	float __sinhf(float x)		/* wrapper sinhf */
#else
	float __sinhf(x)			/* wrapper sinhf */
	float x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_sinhf(x);
#else
	float z; 
	z = __ieee754_sinhf(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!__finitef(z)&&__finitef(x)) {
	    /* sinhf overflow */
	    return (float)__kernel_standard((double)x,(double)x,125);
	} else
	    return z;
#endif
}
weak_alias (__sinhf, sinhf)
