#include <math.h>
#include <stdio.h>
#include <errno.h>

long double
__log1pl (long double x)
{
  fputs ("__log1pl not implemented\n", stderr);
  __set_errno (ENOSYS);
  return 0.0;
}
weak_alias (__log1pl, log1pl)

stub_warning (log1pl)
#include <stub-tag.h>
