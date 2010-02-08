/* w_logf.c -- float version of w_log.c.
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
static char rcsid[] = "$NetBSD: w_logf.c,v 1.3 1995/05/10 20:49:40 jtc Exp $";
#endif

/*
 * wrapper logf(x)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	float __logf(float x)		/* wrapper logf */
#else
	float __logf(x)			/* wrapper logf */
	float x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_logf(x);
#else
	float z;
	z = __ieee754_logf(x);
	if(_LIB_VERSION == _IEEE_ || __isnanf(x) || x > (float)0.0) return z;
	if(x==(float)0.0)
	    /* logf(0) */
	    return (float)__kernel_standard((double)x,(double)x,116);
	else 
	    /* logf(x<0) */
	    return (float)__kernel_standard((double)x,(double)x,117);
#endif
}
weak_alias (__logf, logf)
