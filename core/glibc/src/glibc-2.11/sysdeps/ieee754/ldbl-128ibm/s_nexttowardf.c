/* s_nexttowardf.c -- float version of s_nextafter.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com
 * and Jakub Jelinek, jj@ultra.linux.cz.
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

#include "math.h"
#include <math_private.h>
#include <math_ldbl_opt.h>
#include <float.h>

#ifdef __STDC__
	float __nexttowardf(float x, long double y)
#else
	float __nexttowardf(x,y)
	float x;
	long double y;
#endif
{
	int32_t hx,ix;
	int64_t hy,iy;
	u_int64_t ly, uly;

	GET_FLOAT_WORD(hx,x);
	GET_LDOUBLE_WORDS64(hy,ly,y);
	ix = hx&0x7fffffff;		/* |x| */
	iy = hy&0x7fffffffffffffffLL;	/* |y| */
	uly = ly&0x7fffffffffffffffLL;	/* |y| */

	if((ix>0x7f800000) ||   /* x is nan */
	   ((iy>=0x7ff0000000000000LL)&&((iy-0x7ff0000000000000LL)|uly)!=0))
				/* y is nan */
	   return x+y;
	if((long double) x==y) return y;	/* x=y, return y */
	if(ix==0) {				/* x == 0 */
	    float u;
	    SET_FLOAT_WORD(x,(u_int32_t)((hy>>32)&0x80000000)|1);/* return +-minsub*/
	    u = math_opt_barrier (x);
	    u = u * u;
	    math_force_eval (u);		/* raise underflow flag */
	    return x;
	}
	if(hx>=0) {				/* x > 0 */
	    if(hy<0||(ix>>23)>(iy>>52)-0x380
	       || ((ix>>23)==(iy>>52)-0x380
		   && (ix&0x7fffff)>((hy>>29)&0x7fffff))) {/* x > y, x -= ulp */
		hx -= 1;
	    } else {				/* x < y, x += ulp */
		hx += 1;
	    }
	} else {				/* x < 0 */
	    if(hy>=0||(ix>>23)>(iy>>52)-0x380
	       || ((ix>>23)==(iy>>52)-0x380
		   && (ix&0x7fffff)>((hy>>29)&0x7fffff))) {/* x < y, x -= ulp */
		hx -= 1;
	    } else {				/* x > y, x += ulp */
		hx += 1;
	    }
	}
	hy = hx&0x7f800000;
	if(hy>=0x7f800000) {
	  x = x+x;	/* overflow  */
	  if (FLT_EVAL_METHOD != 0)
	    /* Force conversion to float.  */
	    asm ("" : "+m"(x));
	  return x;
	}
	if(hy<0x00800000) {		/* underflow */
	    float u = x*x;
	    math_force_eval (u);	/* raise underflow flag */
	}
	SET_FLOAT_WORD(x,hx);
	return x;
}
long_double_symbol (libm, __nexttowardf, nexttowardf);
