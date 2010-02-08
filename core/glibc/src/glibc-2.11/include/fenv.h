#ifndef _FENV_H
#include <math/fenv.h>

/* Now define the internal interfaces.  */

extern int __feclearexcept (int __excepts);
extern int __fegetexcept (void);
extern int __fegetexceptflag (fexcept_t *__flagp, int __excepts);
extern int __feraiseexcept (int __excepts);
extern int __fesetexceptflag (__const fexcept_t *__flagp, int __excepts);
extern int __fegetenv (fenv_t *__envp);
extern int __fesetenv (__const fenv_t *__envp);
extern int __feupdateenv (__const fenv_t *__envp);

libm_hidden_proto (feraiseexcept)
libm_hidden_proto (fesetenv)
libm_hidden_proto (fesetround)
libm_hidden_proto (feholdexcept)

#endif
