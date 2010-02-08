#include <math.h>
#include <stdio.h>
#include <errno.h>

long double
__ieee754_coshl (long double x)
{
  fputs ("__ieee754_coshl not implemented\n", stderr);
  __set_errno (ENOSYS);
  return 0.0;
}

stub_warning (__ieee754_coshl)
#include <stub-tag.h>
