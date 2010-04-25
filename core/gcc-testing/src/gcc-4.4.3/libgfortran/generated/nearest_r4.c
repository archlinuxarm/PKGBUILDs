/* Implementation of the NEAREST intrinsic
   Copyright 2003, 2007, 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

This file is part of the GNU Fortran 95 runtime library (libgfortran).

Libgfortran is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

Libgfortran is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#include "libgfortran.h"


#if defined (HAVE_GFC_REAL_4) && defined (HAVE_COPYSIGNF) && defined (HAVE_NEXTAFTERF)

extern GFC_REAL_4 nearest_r4 (GFC_REAL_4 s, GFC_REAL_4 dir);
export_proto(nearest_r4);

GFC_REAL_4
nearest_r4 (GFC_REAL_4 s, GFC_REAL_4 dir)
{
  dir = copysignf (__builtin_inff (), dir);
  if (FLT_EVAL_METHOD != 0)
    {
      /* ??? Work around glibc bug on x86.  */
      volatile GFC_REAL_4 r = nextafterf (s, dir);
      return r;
    }
  else
    return nextafterf (s, dir);
}

#endif
