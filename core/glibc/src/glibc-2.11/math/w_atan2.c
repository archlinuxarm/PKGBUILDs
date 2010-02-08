/* @(#)w_atan2.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_atan2.c,v 1.6 1995/05/10 20:48:39 jtc Exp $";
#endif

/*
 * wrapper atan2(y,x)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	double __atan2(double y, double x)	/* wrapper atan2 */
#else
	double __atan2(y,x)			/* wrapper atan2 */
	double y,x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_atan2(y,x);
#else
	double z;
	z = __ieee754_atan2(y,x);
	if(_LIB_VERSION != _SVID_||__isnan(x)||__isnan(y)) return z;
	if(x==0.0&&y==0.0)
	  return __kernel_standard(y,x,3); /* atan2(+-0,+-0) */
	return z;
#endif
}
weak_alias (__atan2, atan2)
#ifdef NO_LONG_DOUBLE
strong_alias (__atan2, __atan2l)
weak_alias (__atan2, atan2l)
#endif
