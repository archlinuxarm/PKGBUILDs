/* @(#)w_hypot.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_hypot.c,v 1.6 1995/05/10 20:49:07 jtc Exp $";
#endif

/*
 * wrapper hypot(x,y)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	double __hypot(double x, double y)/* wrapper hypot */
#else
	double __hypot(x,y)		/* wrapper hypot */
	double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_hypot(x,y);
#else
	double z;
	z = __ieee754_hypot(x,y);
	if(_LIB_VERSION == _IEEE_) return z;
	if((!__finite(z))&&__finite(x)&&__finite(y))
	    return __kernel_standard(x,y,4); /* hypot overflow */
	else
	    return z;
#endif
}
weak_alias (__hypot, hypot)
#ifdef NO_LONG_DOUBLE
strong_alias (__hypot, __hypotl)
weak_alias (__hypot, hypotl)
#endif
