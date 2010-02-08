/* Return power of complex float value.
   Copyright (C) 2004, 2005 Free Software Foundation, Inc.
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

#define __cpowf __cpowf_not_defined
#define cpowf cpowf_not_defined

#include <complex.h>
#include <math.h>

#undef __cpowf
#undef cpowf
#define __cpowf internal_cpowf

static _Complex float internal_cpowf (_Complex float x, _Complex float c);

#include <math/s_cpowf.c>
#include "cfloat-compat.h"

#undef __cpowf

c1_cfloat_rettype
__c1_cpowf (c1_cfloat_decl (x), c1_cfloat_decl (c))
{
  _Complex float r = internal_cpowf (c1_cfloat_value (x), c1_cfloat_value (c));
  return c1_cfloat_return (r);
}

c2_cfloat_rettype
__c2_cpowf (c2_cfloat_decl (x), c2_cfloat_decl (c))
{
  _Complex float r = internal_cpowf (c2_cfloat_value (x), c2_cfloat_value (c));
  return c2_cfloat_return (r);
}

cfloat_versions (cpowf);
