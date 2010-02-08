/* Return the complex absolute value of float complex value.
   Copyright (C) 2004 Free Software Foundation, Inc.
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

#define __cabsf __cabsf_not_defined
#define cabsf cabsf_not_defined

#include <complex.h>
#include <math.h>
#include "cfloat-compat.h"

#undef __cabsf
#undef cabsf

float
__c1_cabsf (c1_cfloat_decl (z))
{
  return __hypotf (c1_cfloat_real (z), c1_cfloat_imag (z));
}

float
__c2_cabsf (c2_cfloat_decl (z))
{
  return __hypotf (c2_cfloat_real (z), c2_cfloat_imag (z));
}

cfloat_versions (cabsf);
