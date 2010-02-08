/* w_remainderf.c -- float version of w_remainder.c.
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
static char rcsid[] = "$NetBSD: w_remainderf.c,v 1.3 1995/05/10 20:49:46 jtc Exp $";
#endif

/* 
 * wrapper remainderf(x,p)
 */

#include <math.h>
#include "math_private.h"

#ifdef __STDC__
	float __remainderf(float x, float y)	/* wrapper remainder */
#else
	float __remainderf(x,y)			/* wrapper remainder */
	float x,y;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_remainderf(x,y);
#else
	float z;
	z = __ieee754_remainderf(x,y);
	if(_LIB_VERSION == _IEEE_ || __isnanf(y)) return z;
	if(y==(float)0.0) 
	    /* remainder(x,0) */
	    return (float)__kernel_standard((double)x,(double)y,128);
	else
	    return z;
#endif
}
weak_alias (__remainderf, remainderf)
