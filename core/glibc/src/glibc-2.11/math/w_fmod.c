/* @(#)w_fmod.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_fmod.c,v 1.6 1995/05/10 20:48:55 jtc Exp $";
#endif

/*
 * wrapper fmod(x,y)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	double __fmod(double x, double y)	/* wrapper fmod */
#else
	double __fmod(x,y)		/* wrapper fmod */
	double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_fmod(x,y);
#else
	double z;
	z = __ieee754_fmod(x,y);
	if(_LIB_VERSION == _IEEE_ ||__isnan(y)||__isnan(x)) return z;
	if(__isinf(x)||y==0.0) {
		/* fmod(+-Inf,y) or fmod(x,0) */
	        return __kernel_standard(x,y,27);
	} else
	    return z;
#endif
}
weak_alias (__fmod, fmod)
#ifdef NO_LONG_DOUBLE
strong_alias (__fmod, __fmodl)
weak_alias (__fmod, fmodl)
#endif
