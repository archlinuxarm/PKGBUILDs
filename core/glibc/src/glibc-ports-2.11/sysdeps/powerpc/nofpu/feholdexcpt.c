/* Store current floating-point environment and clear exceptions
   (soft-float edition).
   Copyright (C) 2002, 2007, 2008 Free Software Foundation, Inc.
   Contributed by Aldy Hernandez <aldyh@redhat.com>, 2002.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include "soft-fp.h"
#include "soft-supp.h"

int
feholdexcept (fenv_t *envp)
{
  fenv_union_t u;

  /* Get the current state.  */
  __fegetenv (envp);

  u.fenv = *envp;
  /* Clear everything except the rounding mode.  */
  u.l[0] &= 0x3;
  /* Disable exceptions */
  u.l[1] = FE_ALL_EXCEPT;

  /* Put the new state in effect.  */
  fesetenv (&u.fenv);

  return 0;
}
libm_hidden_def (feholdexcept)
