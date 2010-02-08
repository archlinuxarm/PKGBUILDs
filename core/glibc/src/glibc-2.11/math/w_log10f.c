/* w_log10f.c -- float version of w_log10.c.
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
static char rcsid[] = "$NetBSD: w_log10f.c,v 1.3 1995/05/10 20:49:37 jtc Exp $";
#endif

/* 
 * wrapper log10f(X)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	float __log10f(float x)		/* wrapper log10f */
#else
	float __log10f(x)			/* wrapper log10f */
	float x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_log10f(x);
#else
	float z;
	z = __ieee754_log10f(x);
	if(_LIB_VERSION == _IEEE_ || __isnanf(x)) return z;
	if(x<=(float)0.0) {
	    if(x==(float)0.0)
	        /* log10(0) */
	        return (float)__kernel_standard((double)x,(double)x,118);
	    else 
	        /* log10(x<0) */
	        return (float)__kernel_standard((double)x,(double)x,119);
	} else
	    return z;
#endif
}
weak_alias (__log10f, log10f)
