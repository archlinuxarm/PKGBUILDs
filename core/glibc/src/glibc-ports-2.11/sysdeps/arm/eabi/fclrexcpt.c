/* Clear given exceptions in current floating-point environment.
   Copyright (C) 1997,98,99,2000,01,05 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include <fenv.h>
#include <fpu_control.h>

#include <unistd.h>
#include <ldsodefs.h>
#include <dl-procinfo.h>
#include <sysdep.h>

int
__feclearexcept (int excepts)
{
  if (GLRO (dl_hwcap) & HWCAP_ARM_VFP)
    {
      unsigned long int temp;

      /* Mask out unsupported bits/exceptions.  */
      excepts &= FE_ALL_EXCEPT;

      /* Get the current floating point status. */
      _FPU_GETCW (temp);

      /* Clear the relevant bits.  */
      temp = (temp & ~FE_ALL_EXCEPT) | (temp & FE_ALL_EXCEPT & ~excepts);

      /* Put the new data in effect.  */
      _FPU_SETCW (temp);

      /* Success.  */
      return 0;
    }

  /* Unsupported, so fail.  */
  return 1;
}

#include <shlib-compat.h>
#if SHLIB_COMPAT (libm, GLIBC_2_1, GLIBC_2_2)
strong_alias (__feclearexcept, __old_feclearexcept)
compat_symbol (libm, __old_feclearexcept, feclearexcept, GLIBC_2_1);
#endif

versioned_symbol (libm, __feclearexcept, feclearexcept, GLIBC_2_2);
