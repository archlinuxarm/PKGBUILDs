#include <math_ldbl_opt.h>
#include <math/s_fma.c>
#if LONG_DOUBLE_COMPAT(libm, GLIBC_2_1)
compat_symbol (libm, __fma, fmal, GLIBC_2_1);
#endif
