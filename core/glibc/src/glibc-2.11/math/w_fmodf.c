/* w_fmodf.c -- float version of w_fmod.c.
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
static char rcsid[] = "$NetBSD: w_fmodf.c,v 1.3 1995/05/10 20:48:57 jtc Exp $";
#endif

/*
 * wrapper fmodf(x,y)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	float __fmodf(float x, float y)	/* wrapper fmodf */
#else
	float __fmodf(x,y)		/* wrapper fmodf */
	float x,y;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_fmodf(x,y);
#else
	float z;
	z = __ieee754_fmodf(x,y);
	if(_LIB_VERSION == _IEEE_ ||__isnanf(y)||__isnanf(x)) return z;
	if(__isinff(x)||y==(float)0.0) {
		/* fmodf(+-Inf,y) or fmodf(x,0) */
	        return (float)__kernel_standard((double)x,(double)y,127);
	} else
	    return z;
#endif
}
weak_alias (__fmodf, fmodf)
