/* s_modff.c -- float version of s_modf.c.
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
static char rcsid[] = "$NetBSD: s_modff.c,v 1.4 1995/05/10 20:47:56 jtc Exp $";
#endif

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const float one = 1.0;
#else
static float one = 1.0;
#endif

#ifdef __STDC__
	float __modff(float x, float *iptr)
#else
	float __modff(x, iptr)
	float x,*iptr;
#endif
{
	int32_t i0,j0;
	u_int32_t i;
	GET_FLOAT_WORD(i0,x);
	j0 = ((i0>>23)&0xff)-0x7f;	/* exponent of x */
	if(j0<23) {			/* integer part in x */
	    if(j0<0) {			/* |x|<1 */
	        SET_FLOAT_WORD(*iptr,i0&0x80000000);	/* *iptr = +-0 */
		return x;
	    } else {
		i = (0x007fffff)>>j0;
		if((i0&i)==0) {			/* x is integral */
		    u_int32_t ix;
		    *iptr = x;
		    GET_FLOAT_WORD(ix,x);
		    SET_FLOAT_WORD(x,ix&0x80000000);	/* return +-0 */
		    return x;
		} else {
		    SET_FLOAT_WORD(*iptr,i0&(~i));
		    return x - *iptr;
		}
	    }
	} else {			/* no fraction part */
	    *iptr = x*one;
	    /* We must handle NaNs separately.  */
	    if (j0 == 0x80 && (i0 & 0x7fffff))
	      return x*one;
	    SET_FLOAT_WORD(x,i0&0x80000000);	/* return +-0 */
	    return x;
	}
}
weak_alias (__modff, modff)
