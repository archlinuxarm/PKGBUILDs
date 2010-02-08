/* w_expl.c -- long double version of w_exp.c.
 * Conversion to long double by Ulrich Drepper,
 * Cygnus Support, drepper@cygnus.com.
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
static char rcsid[] = "$NetBSD: $";
#endif

/*
 * wrapper expl(x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const long double
#else
static long double
#endif
o_threshold=  1.135652340629414394949193107797076489134e4,
  /* 0x400C, 0xB17217F7, 0xD1CF79AC */
u_threshold= -1.140019167866942050398521670162263001513e4;
  /* 0x400C, 0xB220C447, 0x69C201E8 */

#ifdef __STDC__
	long double __expl(long double x)	/* wrapper exp */
#else
	long double __expl(x)			/* wrapper exp */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_expl(x);
#else
	long double z;
	z = __ieee754_expl(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(__finitel(x)) {
	    if(x>o_threshold)
	        return __kernel_standard(x,x,206); /* exp overflow */
	    else if(x<u_threshold)
	        return __kernel_standard(x,x,207); /* exp underflow */
	}
	return z;
#endif
}
hidden_def (__expl)
weak_alias (__expl, expl)
