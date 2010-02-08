/* @(#)s_ceil.c 5.1 93/09/24 */
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

/*
 * ceil(x)
 * Return x rounded toward -inf to integral value
 * Method:
 *	Bit twiddling.
 * Exception:
 *	Inexact flag raised if x not equal to ceil(x).
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const double huge = 1.0e300;
#else
static double huge = 1.0e300;
#endif

#ifdef __STDC__
	double __ceil(double x)
#else
	double __ceil(x)
	double x;
#endif
{
	int64_t i0,i;
	int32_t j0;
	EXTRACT_WORDS64(i0,x);
	j0 = ((i0>>52)&0x7ff)-0x3ff;
	if(j0<=51) {
	    if(j0<0) { 	/* raise inexact if x != 0 */
		if(huge+x>0.0) {/* return 0*sign(x) if |x|<1 */
		    if(i0<0) {i0=INT64_C(0x8000000000000000);}
		    else if(i0!=0) { i0=INT64_C(0x3ff0000000000000);}
		}
	    } else {
		i = INT64_C(0x000fffffffffffff)>>j0;
		if((i0&i)==0) return x; /* x is integral */
		if(huge+x>0.0) {	/* raise inexact flag */
		    if(i0>0) i0 += UINT64_C(0x0010000000000000)>>j0;
		    i0 &= (~i);
		}
	    }
	} else {
	    if(j0==0x400) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	}
	INSERT_WORDS64(x,i0);
	return x;
}
weak_alias (__ceil, ceil)
#ifdef NO_LONG_DOUBLE
strong_alias (__ceil, __ceill)
weak_alias (__ceil, ceill)
#endif
