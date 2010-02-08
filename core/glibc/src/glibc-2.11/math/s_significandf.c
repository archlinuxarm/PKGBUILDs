/* s_significandf.c -- float version of s_significand.c.
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
static char rcsid[] = "$NetBSD: s_significandf.c,v 1.3 1995/05/10 20:48:13 jtc Exp $";
#endif

#include <math.h>
#include "math_private.h"

#ifdef __STDC__
	float __significandf(float x)
#else
	float __significandf(x)
	float x;
#endif
{
	return __ieee754_scalbf(x,(float) -__ilogbf(x));
}
weak_alias (__significandf, significandf)
