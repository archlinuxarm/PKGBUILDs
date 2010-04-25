/* Generic implementation of the PACK intrinsic
   Copyright (C) 2002, 2004, 2005, 2006, 2007, 2009 Free Software Foundation, Inc.
   Contributed by Paul Brook <paul@nowt.org>

This file is part of the GNU Fortran 95 runtime library (libgfortran).

Libgfortran is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

Ligbfortran is distributed in the hope that it will be useful,
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

/* PACK is specified as follows:

   13.14.80 PACK (ARRAY, MASK, [VECTOR])

   Description: Pack an array into an array of rank one under the
   control of a mask.

   Class: Transformational function.

   Arguments:
      ARRAY   may be of any type. It shall not be scalar.
      MASK    shall be of type LOGICAL. It shall be conformable with ARRAY.
      VECTOR  (optional) shall be of the same type and type parameters
              as ARRAY. VECTOR shall have at least as many elements as
              there are true elements in MASK. If MASK is a scalar
              with the value true, VECTOR shall have at least as many
              elements as there are in ARRAY.

   Result Characteristics: The result is an array of rank one with the
   same type and type parameters as ARRAY. If VECTOR is present, the
   result size is that of VECTOR; otherwise, the result size is the
   number /t/ of true elements in MASK unless MASK is scalar with the
   value true, in which case the result size is the size of ARRAY.

   Result Value: Element /i/ of the result is the element of ARRAY
   that corresponds to the /i/th true element of MASK, taking elements
   in array element order, for /i/ = 1, 2, ..., /t/. If VECTOR is
   present and has size /n/ > /t/, element /i/ of the result has the
   value VECTOR(/i/), for /i/ = /t/ + 1, ..., /n/.

   Examples: The nonzero elements of an array M with the value
   | 0 0 0 |
   | 9 0 0 | may be "gathered" by the function PACK. The result of
   | 0 0 7 |
   PACK (M, MASK = M.NE.0) is [9,7] and the result of PACK (M, M.NE.0,
   VECTOR = (/ 2,4,6,8,10,12 /)) is [9,7,6,8,10,12].

There are two variants of the PACK intrinsic: one, where MASK is
array valued, and the other one where MASK is scalar.  */

static void
pack_internal (gfc_array_char *ret, const gfc_array_char *array,
	       const gfc_array_l1 *mask, const gfc_array_char *vector,
	       index_type size)
{
  /* r.* indicates the return array.  */
  index_type rstride0;
  char * restrict rptr;
  /* s.* indicates the source array.  */
  index_type sstride[GFC_MAX_DIMENSIONS];
  index_type sstride0;
  const char *sptr;
  /* m.* indicates the mask array.  */
  index_type mstride[GFC_MAX_DIMENSIONS];
  index_type mstride0;
  const GFC_LOGICAL_1 *mptr;

  index_type count[GFC_MAX_DIMENSIONS];
  index_type extent[GFC_MAX_DIMENSIONS];
  int zero_sized;
  index_type n;
  index_type dim;
  index_type nelem;
  index_type total;
  int mask_kind;

  dim = GFC_DESCRIPTOR_RANK (array);

  sptr = array->data;
  mptr = mask->data;

  /* Use the same loop for all logical types, by using GFC_LOGICAL_1
     and using shifting to address size and endian issues.  */

  mask_kind = GFC_DESCRIPTOR_SIZE (mask);

  if (mask_kind == 1 || mask_kind == 2 || mask_kind == 4 || mask_kind == 8
#ifdef HAVE_GFC_LOGICAL_16
      || mask_kind == 16
#endif
      )
    {
      /*  Don't convert a NULL pointer as we use test for NULL below.  */
      if (mptr)
	mptr = GFOR_POINTER_TO_L1 (mptr, mask_kind);
    }
  else
    runtime_error ("Funny sized logical array");

  zero_sized = 0;
  for (n = 0; n < dim; n++)
    {
      count[n] = 0;
      extent[n] = array->dim[n].ubound + 1 - array->dim[n].lbound;
      if (extent[n] <= 0)
       zero_sized = 1;
      sstride[n] = array->dim[n].stride * size;
      mstride[n] = mask->dim[n].stride * mask_kind;
    }
  if (sstride[0] == 0)
    sstride[0] = size;
  if (mstride[0] == 0)
    mstride[0] = mask_kind;

  if (ret->data == NULL || compile_options.bounds_check)
    {
      /* Count the elements, either for allocating memory or
	 for bounds checking.  */

      if (vector != NULL)
	{
	  /* The return array will have as many
	     elements as there are in VECTOR.  */
	  total = vector->dim[0].ubound + 1 - vector->dim[0].lbound;
	}
      else
	{
	  /* We have to count the true elements in MASK.  */

	  /* TODO: We could speed up pack easily in the case of only
	     few .TRUE. entries in MASK, by keeping track of where we
	     would be in the source array during the initial traversal
	     of MASK, and caching the pointers to those elements. Then,
	     supposed the number of elements is small enough, we would
	     only have to traverse the list, and copy those elements
	     into the result array. In the case of datatypes which fit
	     in one of the integer types we could also cache the
	     value instead of a pointer to it.
	     This approach might be bad from the point of view of
	     cache behavior in the case where our cache is not big
	     enough to hold all elements that have to be copied.  */

	  const GFC_LOGICAL_1 *m = mptr;

	  total = 0;
	  if (zero_sized)
	    m = NULL;

	  while (m)
	    {
	      /* Test this element.  */
	      if (*m)
		total++;

	      /* Advance to the next element.  */
	      m += mstride[0];
	      count[0]++;
	      n = 0;
	      while (count[n] == extent[n])
		{
		  /* When we get to the end of a dimension, reset it
		     and increment the next dimension.  */
		  count[n] = 0;
		  /* We could precalculate this product, but this is a
		     less frequently used path so probably not worth
		     it.  */
		  m -= mstride[n] * extent[n];
		  n++;
		  if (n >= dim)
		    {
		      /* Break out of the loop.  */
		      m = NULL;
		      break;
		    }
		  else
		    {
		      count[n]++;
		      m += mstride[n];
		    }
		}
	    }
	}

      if (ret->data == NULL)
	{
	  /* Setup the array descriptor.  */
	  ret->dim[0].lbound = 0;
	  ret->dim[0].ubound = total - 1;
	  ret->dim[0].stride = 1;

	  ret->offset = 0;
	  if (total == 0)
	    {
	      /* In this case, nothing remains to be done.  */
	      ret->data = internal_malloc_size (1);
	      return;
	    }
	  else
	    ret->data = internal_malloc_size (size * total);
	}
      else 
	{
	  /* We come here because of range checking.  */
	  index_type ret_extent;

	  ret_extent = ret->dim[0].ubound + 1 - ret->dim[0].lbound;
	  if (total != ret_extent)
	    runtime_error ("Incorrect extent in return value of PACK intrinsic;"
			   " is %ld, should be %ld", (long int) total,
			   (long int) ret_extent);
	}
    }

  rstride0 = ret->dim[0].stride * size;
  if (rstride0 == 0)
    rstride0 = size;
  sstride0 = sstride[0];
  mstride0 = mstride[0];
  rptr = ret->data;

  while (sptr && mptr)
    {
      /* Test this element.  */
      if (*mptr)
        {
          /* Add it.  */
          memcpy (rptr, sptr, size);
          rptr += rstride0;
        }
      /* Advance to the next element.  */
      sptr += sstride0;
      mptr += mstride0;
      count[0]++;
      n = 0;
      while (count[n] == extent[n])
        {
          /* When we get to the end of a dimension, reset it and increment
             the next dimension.  */
          count[n] = 0;
          /* We could precalculate these products, but this is a less
             frequently used path so probably not worth it.  */
          sptr -= sstride[n] * extent[n];
          mptr -= mstride[n] * extent[n];
          n++;
          if (n >= dim)
            {
              /* Break out of the loop.  */
              sptr = NULL;
              break;
            }
          else
            {
              count[n]++;
              sptr += sstride[n];
              mptr += mstride[n];
            }
        }
    }

  /* Add any remaining elements from VECTOR.  */
  if (vector)
    {
      n = vector->dim[0].ubound + 1 - vector->dim[0].lbound;
      nelem = ((rptr - ret->data) / rstride0);
      if (n > nelem)
        {
          sstride0 = vector->dim[0].stride * size;
          if (sstride0 == 0)
            sstride0 = size;

          sptr = vector->data + sstride0 * nelem;
          n -= nelem;
          while (n--)
            {
              memcpy (rptr, sptr, size);
              rptr += rstride0;
              sptr += sstride0;
            }
        }
    }
}

extern void pack (gfc_array_char *, const gfc_array_char *,
		  const gfc_array_l1 *, const gfc_array_char *);
export_proto(pack);

void
pack (gfc_array_char *ret, const gfc_array_char *array,
      const gfc_array_l1 *mask, const gfc_array_char *vector)
{
  index_type type_size;
  index_type size;

  type_size = GFC_DTYPE_TYPE_SIZE(array);

  switch(type_size)
    {
    case GFC_DTYPE_LOGICAL_1:
    case GFC_DTYPE_INTEGER_1:
    case GFC_DTYPE_DERIVED_1:
      pack_i1 ((gfc_array_i1 *) ret, (gfc_array_i1 *) array,
	       (gfc_array_l1 *) mask, (gfc_array_i1 *) vector);
      return;

    case GFC_DTYPE_LOGICAL_2:
    case GFC_DTYPE_INTEGER_2:
      pack_i2 ((gfc_array_i2 *) ret, (gfc_array_i2 *) array,
	       (gfc_array_l1 *) mask, (gfc_array_i2 *) vector);
      return;

    case GFC_DTYPE_LOGICAL_4:
    case GFC_DTYPE_INTEGER_4:

      pack_i4 ((gfc_array_i4 *) ret, (gfc_array_i4 *) array,
	       (gfc_array_l1 *) mask, (gfc_array_i4 *) vector);
      return;

    case GFC_DTYPE_LOGICAL_8:
    case GFC_DTYPE_INTEGER_8:

      pack_i8 ((gfc_array_i8 *) ret, (gfc_array_i8 *) array,
	       (gfc_array_l1 *) mask, (gfc_array_i8 *) vector);
      return;

#ifdef HAVE_GFC_INTEGER_16
    case GFC_DTYPE_LOGICAL_16:
    case GFC_DTYPE_INTEGER_16:

      pack_i16 ((gfc_array_i16 *) ret, (gfc_array_i16 *) array,
		(gfc_array_l1 *) mask, (gfc_array_i16 *) vector);
      return;
#endif
    case GFC_DTYPE_REAL_4:
      pack_r4 ((gfc_array_r4 *) ret, (gfc_array_r4 *) array,
	       (gfc_array_l1 *) mask, (gfc_array_r4 *) vector);
      return;

    case GFC_DTYPE_REAL_8:
      pack_r8 ((gfc_array_r8 *) ret, (gfc_array_r8 *) array,
	       (gfc_array_l1 *) mask, (gfc_array_r8 *) vector);
      return;

#ifdef HAVE_GFC_REAL_10
    case GFC_DTYPE_REAL_10:
      pack_r10 ((gfc_array_r10 *) ret, (gfc_array_r10 *) array,
		(gfc_array_l1 *) mask, (gfc_array_r10 *) vector);
      return;
#endif

#ifdef HAVE_GFC_REAL_16
    case GFC_DTYPE_REAL_16:
      pack_r16 ((gfc_array_r16 *) ret, (gfc_array_r16 *) array,
		(gfc_array_l1 *) mask, (gfc_array_r16 *) vector);
      return;
#endif
    case GFC_DTYPE_COMPLEX_4:
      pack_c4 ((gfc_array_c4 *) ret, (gfc_array_c4 *) array,
	       (gfc_array_l1 *) mask, (gfc_array_c4 *) vector);
      return;

    case GFC_DTYPE_COMPLEX_8:
      pack_c8 ((gfc_array_c8 *) ret, (gfc_array_c8 *) array,
	       (gfc_array_l1 *) mask, (gfc_array_c8 *) vector);
      return;

#ifdef HAVE_GFC_COMPLEX_10
    case GFC_DTYPE_COMPLEX_10:
      pack_c10 ((gfc_array_c10 *) ret, (gfc_array_c10 *) array,
		(gfc_array_l1 *) mask, (gfc_array_c10 *) vector);
      return;
#endif

#ifdef HAVE_GFC_COMPLEX_16
    case GFC_DTYPE_COMPLEX_16:
      pack_c16 ((gfc_array_c16 *) ret, (gfc_array_c16 *) array,
		(gfc_array_l1 *) mask, (gfc_array_c16 *) vector);
      return;
#endif

      /* For derived types, let's check the actual alignment of the
	 data pointers.  If they are aligned, we can safely call
	 the unpack functions.  */

    case GFC_DTYPE_DERIVED_2:
      if (GFC_UNALIGNED_2(ret->data) || GFC_UNALIGNED_2(array->data)
	  || (vector && GFC_UNALIGNED_2(vector->data)))
	break;
      else
	{
	  pack_i2 ((gfc_array_i2 *) ret, (gfc_array_i2 *) array,
		   (gfc_array_l1 *) mask, (gfc_array_i2 *) vector);
	  return;
	}

    case GFC_DTYPE_DERIVED_4:
      if (GFC_UNALIGNED_4(ret->data) || GFC_UNALIGNED_4(array->data)
	  || (vector && GFC_UNALIGNED_4(vector->data)))
	break;
      else
	{
	  pack_i4 ((gfc_array_i4 *) ret, (gfc_array_i4 *) array,
		   (gfc_array_l1 *) mask, (gfc_array_i4 *) vector);
	  return;
	}

    case GFC_DTYPE_DERIVED_8:
      if (GFC_UNALIGNED_8(ret->data) || GFC_UNALIGNED_8(array->data)
	  || (vector && GFC_UNALIGNED_8(vector->data)))
	break;
      else
	{
	  pack_i8 ((gfc_array_i8 *) ret, (gfc_array_i8 *) array,
		   (gfc_array_l1 *) mask, (gfc_array_i8 *) vector);
	}

#ifdef HAVE_GFC_INTEGER_16
    case GFC_DTYPE_DERIVED_16:
      if (GFC_UNALIGNED_16(ret->data) || GFC_UNALIGNED_16(array->data)
	  || (vector && GFC_UNALIGNED_16(vector->data)))
	break;
      else
	{
	  pack_i16 ((gfc_array_i16 *) ret, (gfc_array_i16 *) array,
		   (gfc_array_l1 *) mask, (gfc_array_i16 *) vector);
	  return;
	}
#endif

    }

  size = GFC_DESCRIPTOR_SIZE (array);
  pack_internal (ret, array, mask, vector, size);
}


extern void pack_char (gfc_array_char *, GFC_INTEGER_4, const gfc_array_char *,
		       const gfc_array_l1 *, const gfc_array_char *,
		       GFC_INTEGER_4, GFC_INTEGER_4);
export_proto(pack_char);

void
pack_char (gfc_array_char *ret,
	   GFC_INTEGER_4 ret_length __attribute__((unused)),
	   const gfc_array_char *array, const gfc_array_l1 *mask,
	   const gfc_array_char *vector, GFC_INTEGER_4 array_length,
	   GFC_INTEGER_4 vector_length __attribute__((unused)))
{
  pack_internal (ret, array, mask, vector, array_length);
}


extern void pack_char4 (gfc_array_char *, GFC_INTEGER_4, const gfc_array_char *,
			const gfc_array_l1 *, const gfc_array_char *,
			GFC_INTEGER_4, GFC_INTEGER_4);
export_proto(pack_char4);

void
pack_char4 (gfc_array_char *ret,
	    GFC_INTEGER_4 ret_length __attribute__((unused)),
	    const gfc_array_char *array, const gfc_array_l1 *mask,
	    const gfc_array_char *vector, GFC_INTEGER_4 array_length,
	    GFC_INTEGER_4 vector_length __attribute__((unused)))
{
  pack_internal (ret, array, mask, vector, array_length * sizeof (gfc_char4_t));
}


static void
pack_s_internal (gfc_array_char *ret, const gfc_array_char *array,
		 const GFC_LOGICAL_4 *mask, const gfc_array_char *vector,
		 index_type size)
{
  /* r.* indicates the return array.  */
  index_type rstride0;
  char *rptr;
  /* s.* indicates the source array.  */
  index_type sstride[GFC_MAX_DIMENSIONS];
  index_type sstride0;
  const char *sptr;

  index_type count[GFC_MAX_DIMENSIONS];
  index_type extent[GFC_MAX_DIMENSIONS];
  index_type n;
  index_type dim;
  index_type ssize;
  index_type nelem;
  index_type total;

  dim = GFC_DESCRIPTOR_RANK (array);
  ssize = 1;
  for (n = 0; n < dim; n++)
    {
      count[n] = 0;
      extent[n] = array->dim[n].ubound + 1 - array->dim[n].lbound;
      if (extent[n] < 0)
	extent[n] = 0;

      sstride[n] = array->dim[n].stride * size;
      ssize *= extent[n];
    }
  if (sstride[0] == 0)
    sstride[0] = size;

  sstride0 = sstride[0];

  if (ssize != 0)
    sptr = array->data;
  else
    sptr = NULL;

  if (ret->data == NULL)
    {
      /* Allocate the memory for the result.  */

      if (vector != NULL)
	{
	  /* The return array will have as many elements as there are
	     in vector.  */
	  total = vector->dim[0].ubound + 1 - vector->dim[0].lbound;
	  if (total <= 0)
	    {
	      total = 0;
	      vector = NULL;
	    }
	}
      else
	{
	  if (*mask)
	    {
	      /* The result array will have as many elements as the input
		 array.  */
	      total = extent[0];
	      for (n = 1; n < dim; n++)
		total *= extent[n];
	    }
	  else
	    /* The result array will be empty.  */
	    total = 0;
	}

      /* Setup the array descriptor.  */
      ret->dim[0].lbound = 0;
      ret->dim[0].ubound = total - 1;
      ret->dim[0].stride = 1;
      ret->offset = 0;

      if (total == 0)
	{
	  ret->data = internal_malloc_size (1);
	  return;
	}
      else
	ret->data = internal_malloc_size (size * total);
    }

  rstride0 = ret->dim[0].stride * size;
  if (rstride0 == 0)
    rstride0 = size;
  rptr = ret->data;

  /* The remaining possibilities are now:
       If MASK is .TRUE., we have to copy the source array into the
     result array. We then have to fill it up with elements from VECTOR.
       If MASK is .FALSE., we have to copy VECTOR into the result
     array. If VECTOR were not present we would have already returned.  */

  if (*mask && ssize != 0)
    {
      while (sptr)
	{
	  /* Add this element.  */
	  memcpy (rptr, sptr, size);
	  rptr += rstride0;

	  /* Advance to the next element.  */
	  sptr += sstride0;
	  count[0]++;
	  n = 0;
	  while (count[n] == extent[n])
	    {
	      /* When we get to the end of a dimension, reset it and
		 increment the next dimension.  */
	      count[n] = 0;
	      /* We could precalculate these products, but this is a
		 less frequently used path so probably not worth it.  */
	      sptr -= sstride[n] * extent[n];
	      n++;
	      if (n >= dim)
		{
		  /* Break out of the loop.  */
		  sptr = NULL;
		  break;
		}
	      else
		{
		  count[n]++;
		  sptr += sstride[n];
		}
	    }
	}
    }

  /* Add any remaining elements from VECTOR.  */
  if (vector)
    {
      n = vector->dim[0].ubound + 1 - vector->dim[0].lbound;
      nelem = ((rptr - ret->data) / rstride0);
      if (n > nelem)
        {
          sstride0 = vector->dim[0].stride * size;
          if (sstride0 == 0)
            sstride0 = size;

          sptr = vector->data + sstride0 * nelem;
          n -= nelem;
          while (n--)
            {
              memcpy (rptr, sptr, size);
              rptr += rstride0;
              sptr += sstride0;
            }
        }
    }
}

extern void pack_s (gfc_array_char *ret, const gfc_array_char *array,
		    const GFC_LOGICAL_4 *, const gfc_array_char *);
export_proto(pack_s);

void
pack_s (gfc_array_char *ret, const gfc_array_char *array,
	const GFC_LOGICAL_4 *mask, const gfc_array_char *vector)
{
  pack_s_internal (ret, array, mask, vector, GFC_DESCRIPTOR_SIZE (array));
}


extern void pack_s_char (gfc_array_char *ret, GFC_INTEGER_4,
			 const gfc_array_char *array, const GFC_LOGICAL_4 *,
			 const gfc_array_char *, GFC_INTEGER_4,
			 GFC_INTEGER_4);
export_proto(pack_s_char);

void
pack_s_char (gfc_array_char *ret,
	     GFC_INTEGER_4 ret_length __attribute__((unused)),
	     const gfc_array_char *array, const GFC_LOGICAL_4 *mask,
	     const gfc_array_char *vector, GFC_INTEGER_4 array_length,
	     GFC_INTEGER_4 vector_length __attribute__((unused)))
{
  pack_s_internal (ret, array, mask, vector, array_length);
}


extern void pack_s_char4 (gfc_array_char *ret, GFC_INTEGER_4,
			  const gfc_array_char *array, const GFC_LOGICAL_4 *,
			  const gfc_array_char *, GFC_INTEGER_4,
			  GFC_INTEGER_4);
export_proto(pack_s_char4);

void
pack_s_char4 (gfc_array_char *ret,
	      GFC_INTEGER_4 ret_length __attribute__((unused)),
	      const gfc_array_char *array, const GFC_LOGICAL_4 *mask,
	      const gfc_array_char *vector, GFC_INTEGER_4 array_length,
	      GFC_INTEGER_4 vector_length __attribute__((unused)))
{
  pack_s_internal (ret, array, mask, vector,
		   array_length * sizeof (gfc_char4_t));
}
