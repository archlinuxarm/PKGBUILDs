/* w_jnf.c -- float version of w_jn.c.
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
static char rcsid[] = "$NetBSD: w_jnf.c,v 1.3 1995/05/10 20:49:21 jtc Exp $";
#endif

#include <math.h>
#include "math_private.h"

#ifdef __STDC__
	float jnf(int n, float x)	/* wrapper jnf */
#else
	float jnf(n,x)			/* wrapper jnf */
	float x; int n;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_jnf(n,x);
#else
	float z;
	z = __ieee754_jnf(n,x);
	if(_LIB_VERSION == _IEEE_ || __isnanf(x) ) return z;
	if(fabsf(x)>(float)X_TLOSS) {
	    /* jn(|x|>X_TLOSS,n) */
	    return (float)__kernel_standard((double)n,(double)x,138);
	} else
	    return z;
#endif
}

#ifdef __STDC__
	float ynf(int n, float x)	/* wrapper ynf */
#else
	float ynf(n,x)			/* wrapper ynf */
	float x; int n;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_ynf(n,x);
#else
	float z;
	z = __ieee754_ynf(n,x);
	if(_LIB_VERSION == _IEEE_ || __isnanf(x) ) return z;
        if(x <= (float)0.0){
                if(x==(float)0.0)
                    /* d= -one/(x-x); */
                    return (float)__kernel_standard((double)n,(double)x,112);
                else
                    /* d = zero/(x-x); */
                    return (float)__kernel_standard((double)n,(double)x,113);
        }
	if(x>(float)X_TLOSS) {
	    /* yn(x>X_TLOSS,n) */
	    return (float)__kernel_standard((double)n,(double)x,139);
	} else
	    return z;
#endif
}
