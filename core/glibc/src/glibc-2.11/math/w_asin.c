/* @(#)w_asin.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_asin.c,v 1.6 1995/05/10 20:48:35 jtc Exp $";
#endif

/*
 * wrapper asin(x)
 */


#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	double __asin(double x)		/* wrapper asin */
#else
	double __asin(x)			/* wrapper asin */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_asin(x);
#else
	double z;
	z = __ieee754_asin(x);
	if(_LIB_VERSION == _IEEE_ || __isnan(x)) return z;
	if(fabs(x)>1.0) {
	        return __kernel_standard(x,x,2); /* asin(|x|>1) */
	} else
	    return z;
#endif
}
weak_alias (__asin, asin)
#ifdef NO_LONG_DOUBLE
strong_alias (__asin, __asinl)
weak_alias (__asin, asinl)
#endif
