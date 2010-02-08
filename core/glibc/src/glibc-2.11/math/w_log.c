/* @(#)w_log.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_log.c,v 1.6 1995/05/10 20:49:33 jtc Exp $";
#endif

/*
 * wrapper log(x)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	double __log(double x)		/* wrapper log */
#else
	double __log(x)			/* wrapper log */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_log(x);
#else
	double z;
	z = __ieee754_log(x);
	if(_LIB_VERSION == _IEEE_ || __isnan(x) || x > 0.0) return z;
	if(x==0.0)
	    return __kernel_standard(x,x,16); /* log(0) */
	else
	    return __kernel_standard(x,x,17); /* log(x<0) */
#endif
}
weak_alias (__log, log)
#ifdef NO_LONG_DOUBLE
strong_alias (__log, __logl)
weak_alias (__log, logl)
#endif
