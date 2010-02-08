/* s_rintl.c -- long double version of s_rint.c.
 * Conversion to IEEE quad long double by Jakub Jelinek, jj@ultra.linux.cz.
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
 * rintl(x)
 * Return x rounded to integral value according to the prevailing
 * rounding mode.
 * Method:
 *	Using floating addition.
 * Exception:
 *	Inexact flag raised if x not equal to rintl(x).
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const long double
#else
static long double
#endif
TWO112[2]={
  5.19229685853482762853049632922009600E+33L, /* 0x406F000000000000, 0 */
 -5.19229685853482762853049632922009600E+33L  /* 0xC06F000000000000, 0 */
};

#ifdef __STDC__
	long double __rintl(long double x)
#else
	long double __rintl(x)
	long double x;
#endif
{
	int64_t i0,j0,sx;
	u_int64_t i,i1;
	long double w,t;
	GET_LDOUBLE_WORDS64(i0,i1,x);
	sx = (((u_int64_t)i0)>>63);
	j0 = ((i0>>48)&0x7fff)-0x3fff;
	if(j0<48) {
	    if(j0<0) {
		if(((i0&0x7fffffffffffffffLL)|i1)==0) return x;
		i1 |= (i0&0x0000ffffffffffffLL);
		i0 &= 0xffffe00000000000ULL;
		i0 |= ((i1|-i1)>>16)&0x0000800000000000LL;
		SET_LDOUBLE_MSW64(x,i0);
	        w = TWO112[sx]+x;
	        t = w-TWO112[sx];
		GET_LDOUBLE_MSW64(i0,t);
		SET_LDOUBLE_MSW64(t,(i0&0x7fffffffffffffffLL)|(sx<<63));
	        return t;
	    } else {
		i = (0x0000ffffffffffffLL)>>j0;
		if(((i0&i)|i1)==0) return x; /* x is integral */
		i>>=1;
		if(((i0&i)|i1)!=0) {
		    if(j0==47) i1 = 0x4000000000000000ULL; else
		    i0 = (i0&(~i))|((0x0000200000000000LL)>>j0);
		}
	    }
	} else if (j0>111) {
	    if(j0==0x4000) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	} else {
	    i = -1ULL>>(j0-48);
	    if((i1&i)==0) return x;	/* x is integral */
	    i>>=1;
	    if((i1&i)!=0) i1 = (i1&(~i))|((0x4000000000000000LL)>>(j0-48));
	}
	SET_LDOUBLE_WORDS64(x,i0,i1);
	w = TWO112[sx]+x;
	return w-TWO112[sx];
}
weak_alias (__rintl, rintl)
