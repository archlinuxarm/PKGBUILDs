/* w_asinf.c -- float version of w_asin.c.
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
static char rcsid[] = "$NetBSD: w_asinf.c,v 1.3 1995/05/10 20:48:37 jtc Exp $";
#endif

/* 
 * wrapper asinf(x)
 */


#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	float __asinf(float x)		/* wrapper asinf */
#else
	float __asinf(x)			/* wrapper asinf */
	float x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_asinf(x);
#else
	float z;
	z = __ieee754_asinf(x);
	if(_LIB_VERSION == _IEEE_ || __isnanf(x)) return z;
	if(fabsf(x)>(float)1.0) {
	    /* asinf(|x|>1) */
	    return (float)__kernel_standard((double)x,(double)x,102);
	} else
	    return z;
#endif
}
weak_alias (__asinf, asinf)
