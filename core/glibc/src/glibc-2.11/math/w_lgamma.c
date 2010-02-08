/* @(#)w_lgamma.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_lgamma.c,v 1.6 1995/05/10 20:49:24 jtc Exp $";
#endif

/* double lgamma(double x)
 * Return the logarithm of the Gamma function of x.
 *
 * Method: call __ieee754_lgamma_r
 */

#include <math.h>
#include "math_private.h"

#ifdef __STDC__
	double __lgamma(double x)
#else
	double __lgamma(x)
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_lgamma_r(x,&signgam);
#else
        double y;
	int local_signgam = 0;
        y = __ieee754_lgamma_r(x,&local_signgam);
	if (_LIB_VERSION != _ISOC_)
	  /* ISO C99 does not define the global variable.  */
	  signgam = local_signgam;
        if(_LIB_VERSION == _IEEE_) return y;
        if(!__finite(y)&&__finite(x)) {
            if(__floor(x)==x&&x<=0.0)
                return __kernel_standard(x,x,15); /* lgamma pole */
            else
                return __kernel_standard(x,x,14); /* lgamma overflow */
        } else
            return y;
#endif
}
weak_alias (__lgamma, lgamma)
strong_alias (__lgamma, __gamma)
weak_alias (__gamma, gamma)
#ifdef NO_LONG_DOUBLE
strong_alias (__lgamma, __lgammal)
weak_alias (__lgamma, lgammal)
strong_alias (__gamma, __gammal)
weak_alias (__gamma, gammal)
#endif
