/* w_atanhf.c -- float version of w_atanh.c.
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
static char rcsid[] = "$NetBSD: w_atanhf.c,v 1.3 1995/05/10 20:48:45 jtc Exp $";
#endif

/* 
 * wrapper atanhf(x)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	float __atanhf(float x)		/* wrapper atanhf */
#else
	float __atanhf(x)			/* wrapper atanhf */
	float x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_atanhf(x);
#else
	float z,y;
	z = __ieee754_atanhf(x);
	if(_LIB_VERSION == _IEEE_ || __isnanf(x)) return z;
	y = fabsf(x);
	if(y>=(float)1.0) {
	    if(y>(float)1.0)
	        /* atanhf(|x|>1) */
	        return (float)__kernel_standard((double)x,(double)x,130);
	    else 
	        /* atanhf(|x|==1) */
	        return (float)__kernel_standard((double)x,(double)x,131);
	} else
	    return z;
#endif
}
weak_alias (__atanhf, atanhf)
