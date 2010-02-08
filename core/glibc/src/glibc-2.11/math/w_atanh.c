/* @(#)w_atanh.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_atanh.c,v 1.6 1995/05/10 20:48:43 jtc Exp $";
#endif

/*
 * wrapper atanh(x)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	double __atanh(double x)		/* wrapper atanh */
#else
	double __atanh(x)			/* wrapper atanh */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_atanh(x);
#else
	double z,y;
	z = __ieee754_atanh(x);
	if(_LIB_VERSION == _IEEE_ || __isnan(x)) return z;
	y = fabs(x);
	if(y>=1.0) {
	    if(y>1.0)
	        return __kernel_standard(x,x,30); /* atanh(|x|>1) */
	    else
	        return __kernel_standard(x,x,31); /* atanh(|x|==1) */
	} else
	    return z;
#endif
}
weak_alias (__atanh, atanh)
#ifdef NO_LONG_DOUBLE
strong_alias (__atanh, __atanhl)
weak_alias (__atanh, atanhl)
#endif
