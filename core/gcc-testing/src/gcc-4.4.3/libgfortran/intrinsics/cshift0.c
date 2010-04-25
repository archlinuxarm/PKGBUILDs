/* Generic implementation of the CSHIFT intrinsic
   Copyright 2003, 2005, 2006, 2007, 2009 Free Software Foundation, Inc.
   Contributed by Feng Wang <wf_cs@yahoo.com>

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
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static void
cshift0 (gfc_array_char * ret, const gfc_array_char * array,
	 ssize_t shift, int which, index_type size)
{
  /* r.* indicates the return array.  */
  index_type rstride[GFC_MAX_DIMENSIONS];
  index_type rstride0;
  index_type roffset;
  char *rptr;

  /* s.* indicates the source array.  */
  index_type sstride[GFC_MAX_DIMENSIONS];
  index_type sstride0;
  index_type soffset;
  const char *sptr;

  index_type count[GFC_MAX_DIMENSIONS];
  index_type extent[GFC_MAX_DIMENSIONS];
  index_type dim;
  index_type len;
  index_type n;
  index_type arraysize;

  index_type type_size;

  if (which < 1 || which > GFC_DESCRIPTOR_RANK (array))
    runtime_error ("Argument 'DIM' is out of range in call to 'CSHIFT'");

  arraysize = size0 ((array_t *) array);

  if (ret->data == NULL)
    {
      int i;

      ret->offset = 0;
      ret->dtype = array->dtype;
      for (i = 0; i < GFC_DESCRIPTOR_RANK (array); i++)
        {
          ret->dim[i].lbound = 0;
          ret->dim[i].ubound = array->dim[i].ubound - array->dim[i].lbound;

          if (i == 0)
            ret->dim[i].stride = 1;
          else
            ret->dim[i].stride = (ret->dim[i-1].ubound + 1)
				 * ret->dim[i-1].stride;
        }

      if (arraysize > 0)
	ret->data = internal_malloc_size (size * arraysize);
      else
	{
	  ret->data = internal_malloc_size (1);
	  return;
	}
    }
  
  if (arraysize == 0)
    return;
  type_size = GFC_DTYPE_TYPE_SIZE (array);

  switch(type_size)
    {
    case GFC_DTYPE_LOGICAL_1:
    case GFC_DTYPE_INTEGER_1:
    case GFC_DTYPE_DERIVED_1:
      cshift0_i1 ((gfc_array_i1 *)ret, (gfc_array_i1 *) array, shift, which);
      return;

    case GFC_DTYPE_LOGICAL_2:
    case GFC_DTYPE_INTEGER_2:
      cshift0_i2 ((gfc_array_i2 *)ret, (gfc_array_i2 *) array, shift, which);
      return;

    case GFC_DTYPE_LOGICAL_4:
    case GFC_DTYPE_INTEGER_4:
      cshift0_i4 ((gfc_array_i4 *)ret, (gfc_array_i4 *) array, shift, which);
      return;

    case GFC_DTYPE_LOGICAL_8:
    case GFC_DTYPE_INTEGER_8:
      cshift0_i8 ((gfc_array_i8 *)ret, (gfc_array_i8 *) array, shift, which);
      return;

#ifdef HAVE_GFC_INTEGER_16
    case GFC_DTYPE_LOGICAL_16:
    case GFC_DTYPE_INTEGER_16:
      cshift0_i16 ((gfc_array_i16 *)ret, (gfc_array_i16 *) array, shift,
		   which);
      return;
#endif

    case GFC_DTYPE_REAL_4:
      cshift0_r4 ((gfc_array_r4 *)ret, (gfc_array_r4 *) array, shift, which);
      return;

    case GFC_DTYPE_REAL_8:
      cshift0_r8 ((gfc_array_r8 *)ret, (gfc_array_r8 *) array, shift, which);
      return;

#ifdef HAVE_GFC_REAL_10
    case GFC_DTYPE_REAL_10:
      cshift0_r10 ((gfc_array_r10 *)ret, (gfc_array_r10 *) array, shift,
		   which);
      return;
#endif

#ifdef HAVE_GFC_REAL_16
    case GFC_DTYPE_REAL_16:
      cshift0_r16 ((gfc_array_r16 *)ret, (gfc_array_r16 *) array, shift,
		   which);
      return;
#endif

    case GFC_DTYPE_COMPLEX_4:
      cshift0_c4 ((gfc_array_c4 *)ret, (gfc_array_c4 *) array, shift, which);
      return;

    case GFC_DTYPE_COMPLEX_8:
      cshift0_c8 ((gfc_array_c8 *)ret, (gfc_array_c8 *) array, shift, which);
      return;

#ifdef HAVE_GFC_COMPLEX_10
    case GFC_DTYPE_COMPLEX_10:
      cshift0_c10 ((gfc_array_c10 *)ret, (gfc_array_c10 *) array, shift,
		   which);
      return;
#endif

#ifdef HAVE_GFC_COMPLEX_16
    case GFC_DTYPE_COMPLEX_16:
      cshift0_c16 ((gfc_array_c16 *)ret, (gfc_array_c16 *) array, shift,
		   which);
      return;
#endif

    default:
      break;
    }

  switch (size)
    {
      /* Let's check the actual alignment of the data pointers.  If they
	 are suitably aligned, we can safely call the unpack functions.  */

    case sizeof (GFC_INTEGER_1):
      cshift0_i1 ((gfc_array_i1 *) ret, (gfc_array_i1 *) array, shift,
		  which);
      break;

    case sizeof (GFC_INTEGER_2):
      if (GFC_UNALIGNED_2(ret->data) || GFC_UNALIGNED_2(array->data))
	break;
      else
	{
	  cshift0_i2 ((gfc_array_i2 *) ret, (gfc_array_i2 *) array, shift,
		      which);
	  return;
	}

    case sizeof (GFC_INTEGER_4):
      if (GFC_UNALIGNED_4(ret->data) || GFC_UNALIGNED_4(array->data))
	break;
      else
	{
	  cshift0_i4 ((gfc_array_i4 *)ret, (gfc_array_i4 *) array, shift,
		      which);
	  return;
	}

    case sizeof (GFC_INTEGER_8):
      if (GFC_UNALIGNED_8(ret->data) || GFC_UNALIGNED_8(array->data))
	{
	  /* Let's try to use the complex routines.  First, a sanity
	     check that the sizes match; this should be optimized to
	     a no-op.  */
	  if (sizeof(GFC_INTEGER_8) != sizeof(GFC_COMPLEX_4))
	    break;

	  if (GFC_UNALIGNED_C4(ret->data) || GFC_UNALIGNED_C4(array->data))
	    break;

	  cshift0_c4 ((gfc_array_c4 *) ret, (gfc_array_c4 *) array, shift,
		      which);
	      return;
	}
      else
	{
	  cshift0_i8 ((gfc_array_i8 *)ret, (gfc_array_i8 *) array, shift,
		      which);
	  return;
	}

#ifdef HAVE_GFC_INTEGER_16
    case sizeof (GFC_INTEGER_16):
      if (GFC_UNALIGNED_16(ret->data) || GFC_UNALIGNED_16(array->data))
	{
	  /* Let's try to use the complex routines.  First, a sanity
	     check that the sizes match; this should be optimized to
	     a no-op.  */
	  if (sizeof(GFC_INTEGER_16) != sizeof(GFC_COMPLEX_8))
	    break;

	  if (GFC_UNALIGNED_C8(ret->data) || GFC_UNALIGNED_C8(array->data))
	    break;

	  cshift0_c8 ((gfc_array_c8 *) ret, (gfc_array_c8 *) array, shift,
		      which);
	      return;
	}
      else
	{
	  cshift0_i16 ((gfc_array_i16 *) ret, (gfc_array_i16 *) array,
		       shift, which);
	  return;
	}
#else
    case sizeof (GFC_COMPLEX_8):

      if (GFC_UNALIGNED_C8(ret->data) || GFC_UNALIGNED_C8(array->data))
	break;
      else
	{
	  cshift0_c8 ((gfc_array_c8 *) ret, (gfc_array_c8 *) array, shift,
		      which);
	  return;
	}
#endif

    default:
      break;
    }


  which = which - 1;
  sstride[0] = 0;
  rstride[0] = 0;

  extent[0] = 1;
  count[0] = 0;
  n = 0;
  /* Initialized for avoiding compiler warnings.  */
  roffset = size;
  soffset = size;
  len = 0;

  for (dim = 0; dim < GFC_DESCRIPTOR_RANK (array); dim++)
    {
      if (dim == which)
        {
          roffset = ret->dim[dim].stride * size;
          if (roffset == 0)
            roffset = size;
          soffset = array->dim[dim].stride * size;
          if (soffset == 0)
            soffset = size;
          len = array->dim[dim].ubound + 1 - array->dim[dim].lbound;
        }
      else
        {
          count[n] = 0;
          extent[n] = array->dim[dim].ubound + 1 - array->dim[dim].lbound;
          rstride[n] = ret->dim[dim].stride * size;
          sstride[n] = array->dim[dim].stride * size;
          n++;
        }
    }
  if (sstride[0] == 0)
    sstride[0] = size;
  if (rstride[0] == 0)
    rstride[0] = size;

  dim = GFC_DESCRIPTOR_RANK (array);
  rstride0 = rstride[0];
  sstride0 = sstride[0];
  rptr = ret->data;
  sptr = array->data;

  shift = len == 0 ? 0 : shift % (ssize_t)len;
  if (shift < 0)
    shift += len;

  while (rptr)
    {
      /* Do the shift for this dimension.  */

      /* If elements are contiguous, perform the operation
	 in two block moves.  */
      if (soffset == size && roffset == size)
	{
	  size_t len1 = shift * size;
	  size_t len2 = (len - shift) * size;
	  memcpy (rptr, sptr + len1, len2);
	  memcpy (rptr + len2, sptr, len1);
	}
      else
	{
	  /* Otherwise, we'll have to perform the copy one element at
	     a time.  */
	  char *dest = rptr;
	  const char *src = &sptr[shift * soffset];

	  for (n = 0; n < len - shift; n++)
	    {
	      memcpy (dest, src, size);
	      dest += roffset;
	      src += soffset;
	    }
	  for (src = sptr, n = 0; n < shift; n++)
	    {
	      memcpy (dest, src, size);
	      dest += roffset;
	      src += soffset;
	    }
	}

      /* Advance to the next section.  */
      rptr += rstride0;
      sptr += sstride0;
      count[0]++;
      n = 0;
      while (count[n] == extent[n])
        {
          /* When we get to the end of a dimension, reset it and increment
             the next dimension.  */
          count[n] = 0;
          /* We could precalculate these products, but this is a less
             frequently used path so probably not worth it.  */
          rptr -= rstride[n] * extent[n];
          sptr -= sstride[n] * extent[n];
          n++;
          if (n >= dim - 1)
            {
              /* Break out of the loop.  */
              rptr = NULL;
              break;
            }
          else
            {
              count[n]++;
              rptr += rstride[n];
              sptr += sstride[n];
            }
        }
    }
}

#define DEFINE_CSHIFT(N)						      \
  extern void cshift0_##N (gfc_array_char *, const gfc_array_char *,	      \
			   const GFC_INTEGER_##N *, const GFC_INTEGER_##N *); \
  export_proto(cshift0_##N);						      \
									      \
  void									      \
  cshift0_##N (gfc_array_char *ret, const gfc_array_char *array,	      \
	       const GFC_INTEGER_##N *pshift, const GFC_INTEGER_##N *pdim)    \
  {									      \
    cshift0 (ret, array, *pshift, pdim ? *pdim : 1,			      \
	     GFC_DESCRIPTOR_SIZE (array));				      \
  }									      \
									      \
  extern void cshift0_##N##_char (gfc_array_char *, GFC_INTEGER_4,	      \
				  const gfc_array_char *,		      \
				  const GFC_INTEGER_##N *,		      \
				  const GFC_INTEGER_##N *, GFC_INTEGER_4);    \
  export_proto(cshift0_##N##_char);					      \
									      \
  void									      \
  cshift0_##N##_char (gfc_array_char *ret,				      \
		      GFC_INTEGER_4 ret_length __attribute__((unused)),	      \
		      const gfc_array_char *array,			      \
		      const GFC_INTEGER_##N *pshift,			      \
		      const GFC_INTEGER_##N *pdim,			      \
		      GFC_INTEGER_4 array_length)			      \
  {									      \
    cshift0 (ret, array, *pshift, pdim ? *pdim : 1, array_length);	      \
  }									      \
									      \
  extern void cshift0_##N##_char4 (gfc_array_char *, GFC_INTEGER_4,	      \
				   const gfc_array_char *,		      \
				   const GFC_INTEGER_##N *,		      \
				   const GFC_INTEGER_##N *, GFC_INTEGER_4);   \
  export_proto(cshift0_##N##_char4);					      \
									      \
  void									      \
  cshift0_##N##_char4 (gfc_array_char *ret,				      \
		       GFC_INTEGER_4 ret_length __attribute__((unused)),      \
		       const gfc_array_char *array,			      \
		       const GFC_INTEGER_##N *pshift,			      \
		       const GFC_INTEGER_##N *pdim,			      \
		       GFC_INTEGER_4 array_length)			      \
  {									      \
    cshift0 (ret, array, *pshift, pdim ? *pdim : 1,			      \
	     array_length * sizeof (gfc_char4_t));			      \
  }

DEFINE_CSHIFT (1);
DEFINE_CSHIFT (2);
DEFINE_CSHIFT (4);
DEFINE_CSHIFT (8);
#ifdef HAVE_GFC_INTEGER_16
DEFINE_CSHIFT (16);
#endif
