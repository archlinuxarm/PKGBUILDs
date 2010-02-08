/* @(#)w_cosh.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_cosh.c,v 1.6 1995/05/10 20:48:47 jtc Exp $";
#endif

/*
 * wrapper cosh(x)
 */

#include <math.h>
#include "math_private.h"

#ifdef __STDC__
	double __cosh(double x)		/* wrapper cosh */
#else
	double __cosh(x)			/* wrapper cosh */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_cosh(x);
#else
	double z;
	z = __ieee754_cosh(x);
	if(_LIB_VERSION == _IEEE_ || __isnan(x)) return z;
	if(!__finite(z) && __finite(x)) {
	        return __kernel_standard(x,x,5); /* cosh overflow */
	} else
	    return z;
#endif
}
weak_alias (__cosh, cosh)
#ifdef NO_LONG_DOUBLE
strong_alias (__cosh, __coshl)
weak_alias (__cosh, coshl)
#endif
