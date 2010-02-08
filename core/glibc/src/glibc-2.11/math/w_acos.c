/* @(#)w_acos.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_acos.c,v 1.6 1995/05/10 20:48:26 jtc Exp $";
#endif

/*
 * wrap_acos(x)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	double __acos(double x)		/* wrapper acos */
#else
	double __acos(x)			/* wrapper acos */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_acos(x);
#else
	double z;
	z = __ieee754_acos(x);
	if(_LIB_VERSION == _IEEE_ || __isnan(x)) return z;
	if(fabs(x)>1.0) {
	        return __kernel_standard(x,x,1); /* acos(|x|>1) */
	} else
	    return z;
#endif
}
weak_alias (__acos, acos)
#ifdef NO_LONG_DOUBLE
strong_alias (__acos, __acosl)
weak_alias (__acos, acosl)
#endif
