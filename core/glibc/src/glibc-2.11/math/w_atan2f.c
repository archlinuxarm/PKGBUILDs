/* w_atan2f.c -- float version of w_atan2.c.
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
static char rcsid[] = "$NetBSD: w_atan2f.c,v 1.3 1995/05/10 20:48:42 jtc Exp $";
#endif

/*
 * wrapper atan2f(y,x)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	float __atan2f(float y, float x)		/* wrapper atan2f */
#else
	float __atan2f(y,x)			/* wrapper atan2 */
	float y,x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_atan2f(y,x);
#else
	float z;
	z = __ieee754_atan2f(y,x);
	if(_LIB_VERSION != _SVID_||__isnanf(x)||__isnanf(y)) return z;
	if(x==0.0&&y==0.0)
	  return __kernel_standard(y,x,103); /* atan2(+-0,+-0) */
	return z;
#endif
}
weak_alias (__atan2f, atan2f)
