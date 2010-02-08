/* @(#)w_sinh.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_sinh.c,v 1.6 1995/05/10 20:49:51 jtc Exp $";
#endif

/*
 * wrapper sinh(x)
 */

#include <math.h>
#include "math_private.h"

#ifdef __STDC__
	double __sinh(double x)		/* wrapper sinh */
#else
	double __sinh(x)			/* wrapper sinh */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_sinh(x);
#else
	double z;
	z = __ieee754_sinh(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!__finite(z)&&__finite(x)) {
	    return __kernel_standard(x,x,25); /* sinh overflow */
	} else
	    return z;
#endif
}
weak_alias (__sinh, sinh)
#ifdef NO_LONG_DOUBLE
strong_alias (__sinh, __sinhl)
weak_alias (__sinh, sinhl)
#endif
