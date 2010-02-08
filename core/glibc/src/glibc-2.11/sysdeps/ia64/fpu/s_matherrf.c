/* Derived from:  */
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


#include "math.h"
#include "math_private.h"
#include "libm_support.h"

#ifdef __STDC__
	int
	weak_function
	 __matherrf(struct exceptionf *x)
#else
	int
	weak_function
	__matherrf(x)
	struct exceptionf *x;
#endif
{
	int n=0;
	if(x->arg1!=x->arg1) return 0;
	return n;
}
weak_alias (__matherrf, matherrf)
