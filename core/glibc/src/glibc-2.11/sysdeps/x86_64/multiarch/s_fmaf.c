/* FMA version of fmaf.
   Copyright (C) 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA. */

#include <config.h>
#include <math.h>
#include <init-arch.h>

#ifdef HAVE_AVX_SUPPORT

extern float __fmaf_sse2 (float x, float y, float z) attribute_hidden;


static float
__fmaf_fma (float x, float y, float z)
{
  asm ("vfmadd213ss %3, %2, %0" : "=x" (x) : "0" (x), "x" (y), "xm" (z));
  return x;
}

libm_ifunc (__fmaf, HAS_FMA ? __fmaf_fma : __fmaf_sse2);
weak_alias (__fmaf, fmaf)

# define __fmaf __fmaf_sse2
#endif

#include <math/s_fmaf.c>
