/* @(#)w_j1.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_j1.c,v 1.6 1995/05/10 20:49:15 jtc Exp $";
#endif

/*
 * wrapper of j1,y1
 */

#include <math.h>
#include "math_private.h"

#ifdef __STDC__
	double j1(double x)		/* wrapper j1 */
#else
	double j1(x)			/* wrapper j1 */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_j1(x);
#else
	double z;
	z = __ieee754_j1(x);
	if(_LIB_VERSION == _IEEE_ || __isnan(x) ) return z;
	if(fabs(x)>X_TLOSS) {
	        return __kernel_standard(x,x,36); /* j1(|x|>X_TLOSS) */
	} else
	    return z;
#endif
}
#ifdef NO_LONG_DOUBLE
strong_alias (j1, j1l)
#endif


#ifdef __STDC__
	double y1(double x)		/* wrapper y1 */
#else
	double y1(x)			/* wrapper y1 */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_y1(x);
#else
	double z;
	z = __ieee754_y1(x);
	if(_LIB_VERSION == _IEEE_ || __isnan(x) ) return z;
        if(x <= 0.0){
                if(x==0.0)
                    /* d= -one/(x-x); */
                    return __kernel_standard(x,x,10);
                else
                    /* d = zero/(x-x); */
                    return __kernel_standard(x,x,11);
        }
	if(x>X_TLOSS) {
	        return __kernel_standard(x,x,37); /* y1(x>X_TLOSS) */
	} else
	    return z;
#endif
}
#ifdef NO_LONG_DOUBLE
strong_alias (y1, y1l)
#endif
