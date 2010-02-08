/* @(#)w_sqrt.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_sqrt.c,v 1.6 1995/05/10 20:49:55 jtc Exp $";
#endif

/*
 * wrapper sqrt(x)
 */

#include <math.h>
#include "math_private.h"

#ifdef __STDC__
	double __sqrt(double x)		/* wrapper sqrt */
#else
	double __sqrt(x)			/* wrapper sqrt */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_sqrt(x);
#else
	double z;
	z = __ieee754_sqrt(x);
	if(_LIB_VERSION == _IEEE_ || __isnan(x)) return z;
	if(x<0.0) {
	    return __kernel_standard(x,x,26); /* sqrt(negative) */
	} else
	    return z;
#endif
}
weak_alias (__sqrt, sqrt)
#ifdef NO_LONG_DOUBLE
strong_alias (__sqrt, __sqrtl)
weak_alias (__sqrt, sqrtl)
#endif
