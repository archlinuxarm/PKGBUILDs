/* Implementation of the ASSOCIATED intrinsic
   Copyright 2003, 2009 Free Software Foundation, Inc.
   Contributed by kejia Zhao (CCRG) <kejia_zh@yahoo.com.cn>

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

extern int associated (const gfc_array_void *, const gfc_array_void *);
export_proto(associated);

int
associated (const gfc_array_void *pointer, const gfc_array_void *target)
{
  int n, rank;

  if (GFC_DESCRIPTOR_DATA (pointer) == NULL)
    return 0;
  if (GFC_DESCRIPTOR_DATA (pointer) != GFC_DESCRIPTOR_DATA (target))
    return 0;
  if (GFC_DESCRIPTOR_DTYPE (pointer) != GFC_DESCRIPTOR_DTYPE (target))
    return 0;

  rank = GFC_DESCRIPTOR_RANK (pointer);
  for (n = 0; n < rank; n++)
    {
      long diff;
      diff = pointer->dim[n].ubound - pointer->dim[n].lbound;

      if (diff != (target->dim[n].ubound - target->dim[n].lbound))
        return 0;
      if (pointer->dim[n].stride != target->dim[n].stride && diff != 0)
        return 0;
      if (pointer->dim[n].ubound < pointer->dim[n].lbound)
	return 0;
    }

  return 1;
}
