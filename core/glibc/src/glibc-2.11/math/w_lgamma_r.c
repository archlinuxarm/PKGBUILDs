/* @(#)wr_lgamma.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_lgamma_r.c,v 1.6 1995/05/10 20:49:27 jtc Exp $";
#endif

/*
 * wrapper double lgamma_r(double x, int *signgamp)
 */

#include <math.h>
#include "math_private.h"


#ifdef __STDC__
	double __lgamma_r(double x, int *signgamp) /* wrapper lgamma_r */
#else
	double __lgamma_r(x,signgamp)              /* wrapper lgamma_r */
        double x; int *signgamp;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_lgamma_r(x,signgamp);
#else
        double y;
        y = __ieee754_lgamma_r(x,signgamp);
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
weak_alias (__lgamma_r, lgamma_r)
#ifdef NO_LONG_DOUBLE
strong_alias (__lgamma_r, __lgammal_r)
weak_alias (__lgamma_r, lgammal_r)
#endif
