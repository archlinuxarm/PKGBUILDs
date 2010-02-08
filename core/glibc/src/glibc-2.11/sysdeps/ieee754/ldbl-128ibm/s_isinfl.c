/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Change for long double by Jakub Jelinek <jj@ultra.linux.cz>
 * Public domain.
 */

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: $";
#endif

/*
 * isinfl(x) returns 1 if x is inf, -1 if x is -inf, else 0;
 * no branching!
 */

#include "math.h"
#include "math_private.h"
#include <math_ldbl_opt.h>

int
___isinfl (long double x)
{
	int64_t hx,lx;
	GET_LDOUBLE_WORDS64(hx,lx,x);
	lx = (lx & 0x7fffffffffffffffLL);
	lx |= (hx & 0x7fffffffffffffffLL) ^ 0x7ff0000000000000LL;
	lx |= -lx;
	return ~(lx >> 63) & (hx >> 62);
}
hidden_ver (___isinfl, __isinfl)
#ifndef IS_IN_libm
weak_alias (___isinfl, ____isinfl)
long_double_symbol (libc, ___isinfl, isinfl);
long_double_symbol (libc, ____isinfl, __isinfl);
#endif
