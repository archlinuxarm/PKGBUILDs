#include <math_ldbl_opt.h>
#undef weak_alias
#define weak_alias(n,a)
#define __signbitl(arg) ___signbitl(arg)
#include <sysdeps/ieee754/ldbl-128/s_signbitl.c>
#undef __signbitl
#ifdef IS_IN_libm
long_double_symbol (libm, ___signbitl, __signbitl);
#else
long_double_symbol (libc, ___signbitl, __signbitl);
#endif
