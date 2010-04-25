/* Generic implementation of the SPREAD intrinsic
   Copyright 2002, 2005, 2006, 2007, 2009 Free Software Foundation, Inc.
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

static void
spread_internal (gfc_array_char *ret, const gfc_array_char *source,
		 const index_type *along, const index_type *pncopies,
		 index_type size)
{
  /* r.* indicates the return array.  */
  index_type rstride[GFC_MAX_DIMENSIONS];
  index_type rstride0;
  index_type rdelta = 0;
  index_type rrank;
  index_type rs;
  char *rptr;
  char *dest;
  /* s.* indicates the source array.  */
  index_type sstride[GFC_MAX_DIMENSIONS];
  index_type sstride0;
  index_type srank;
  const char *sptr;

  index_type count[GFC_MAX_DIMENSIONS];
  index_type extent[GFC_MAX_DIMENSIONS];
  index_type n;
  index_type dim;
  index_type ncopies;

  srank = GFC_DESCRIPTOR_RANK(source);

  rrank = srank + 1;
  if (rrank > GFC_MAX_DIMENSIONS)
    runtime_error ("return rank too large in spread()");

  if (*along > rrank)
      runtime_error ("dim outside of rank in spread()");

  ncopies = *pncopies;

  if (ret->data == NULL)
    {
      /* The front end has signalled that we need to populate the
	 return array descriptor.  */
      ret->dtype = (source->dtype & ~GFC_DTYPE_RANK_MASK) | rrank;
      dim = 0;
      rs = 1;
      for (n = 0; n < rrank; n++)
	{
	  ret->dim[n].stride = rs;
	  ret->dim[n].lbound = 0;
	  if (n == *along - 1)
	    {
	      ret->dim[n].ubound = ncopies - 1;
	      rdelta = rs * size;
	      rs *= ncopies;
	    }
	  else
	    {
	      count[dim] = 0;
	      extent[dim] = source->dim[dim].ubound + 1
		- source->dim[dim].lbound;
	      sstride[dim] = source->dim[dim].stride * size;
	      rstride[dim] = rs * size;

	      ret->dim[n].ubound = extent[dim]-1;
	      rs *= extent[dim];
	      dim++;
	    }
	}
      ret->offset = 0;
      if (rs > 0)
        ret->data = internal_malloc_size (rs * size);
      else
	{
	  ret->data = internal_malloc_size (1);
	  return;
	}
    }
  else
    {
      int zero_sized;

      zero_sized = 0;

      dim = 0;
      if (GFC_DESCRIPTOR_RANK(ret) != rrank)
	runtime_error ("rank mismatch in spread()");

      if (compile_options.bounds_check)
	{
	  for (n = 0; n < rrank; n++)
	    {
	      index_type ret_extent;

	      ret_extent = ret->dim[n].ubound + 1 - ret->dim[n].lbound;
	      if (n == *along - 1)
		{
		  rdelta = ret->dim[n].stride * size;

		  if (ret_extent != ncopies)
		    runtime_error("Incorrect extent in return value of SPREAD"
				  " intrinsic in dimension %ld: is %ld,"
				  " should be %ld", (long int) n+1,
				  (long int) ret_extent, (long int) ncopies);
		}
	      else
		{
		  count[dim] = 0;
		  extent[dim] = source->dim[dim].ubound + 1
		    - source->dim[dim].lbound;
		  if (ret_extent != extent[dim])
		    runtime_error("Incorrect extent in return value of SPREAD"
				  " intrinsic in dimension %ld: is %ld,"
				  " should be %ld", (long int) n+1,
				  (long int) ret_extent,
				  (long int) extent[dim]);
		    
		  if (extent[dim] <= 0)
		    zero_sized = 1;
		  sstride[dim] = source->dim[dim].stride * size;
		  rstride[dim] = ret->dim[n].stride * size;
		  dim++;
		}
	    }
	}
      else
	{
	  for (n = 0; n < rrank; n++)
	    {
	      if (n == *along - 1)
		{
		  rdelta = ret->dim[n].stride * size;
		}
	      else
		{
		  count[dim] = 0;
		  extent[dim] = source->dim[dim].ubound + 1
		    - source->dim[dim].lbound;
		  if (extent[dim] <= 0)
		    zero_sized = 1;
		  sstride[dim] = source->dim[dim].stride * size;
		  rstride[dim] = ret->dim[n].stride * size;
		  dim++;
		}
	    }
	}

      if (zero_sized)
	return;

      if (sstride[0] == 0)
	sstride[0] = size;
    }
  sstride0 = sstride[0];
  rstride0 = rstride[0];
  rptr = ret->data;
  sptr = source->data;

  while (sptr)
    {
      /* Spread this element.  */
      dest = rptr;
      for (n = 0; n < ncopies; n++)
        {
          memcpy (dest, sptr, size);
          dest += rdelta;
        }
      /* Advance to the next element.  */
      sptr += sstride0;
      rptr += rstride0;
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
          rptr -= rstride[n] * extent[n];
          n++;
          if (n >= srank)
            {
              /* Break out of the loop.  */
              sptr = NULL;
              break;
            }
          else
            {
              count[n]++;
              sptr += sstride[n];
              rptr += rstride[n];
            }
        }
    }
}

/* This version of spread_internal treats the special case of a scalar
   source.  This is much simpler than the more general case above.  */

static void
spread_internal_scalar (gfc_array_char *ret, const char *source,
			const index_type *along, const index_type *pncopies,
			index_type size)
{
  int n;
  int ncopies = *pncopies;
  char * dest;

  if (GFC_DESCRIPTOR_RANK (ret) != 1)
    runtime_error ("incorrect destination rank in spread()");

  if (*along > 1)
    runtime_error ("dim outside of rank in spread()");

  if (ret->data == NULL)
    {
      ret->data = internal_malloc_size (ncopies * size);
      ret->offset = 0;
      ret->dim[0].stride = 1;
      ret->dim[0].lbound = 0;
      ret->dim[0].ubound = ncopies - 1;
    }
  else
    {
      if (ncopies - 1 > (ret->dim[0].ubound - ret->dim[0].lbound)
			   / ret->dim[0].stride)
	runtime_error ("dim too large in spread()");
    }

  for (n = 0; n < ncopies; n++)
    {
      dest = (char*)(ret->data + n*size*ret->dim[0].stride);
      memcpy (dest , source, size);
    }
}

extern void spread (gfc_array_char *, const gfc_array_char *,
		    const index_type *, const index_type *);
export_proto(spread);

void
spread (gfc_array_char *ret, const gfc_array_char *source,
	const index_type *along, const index_type *pncopies)
{
  index_type type_size;

  type_size = GFC_DTYPE_TYPE_SIZE(ret);
  switch(type_size)
    {
    case GFC_DTYPE_DERIVED_1:
    case GFC_DTYPE_LOGICAL_1:
    case GFC_DTYPE_INTEGER_1:
      spread_i1 ((gfc_array_i1 *) ret, (gfc_array_i1 *) source,
		 *along, *pncopies);
      return;

    case GFC_DTYPE_LOGICAL_2:
    case GFC_DTYPE_INTEGER_2:
      spread_i2 ((gfc_array_i2 *) ret, (gfc_array_i2 *) source,
		 *along, *pncopies);
      return;

    case GFC_DTYPE_LOGICAL_4:
    case GFC_DTYPE_INTEGER_4:
      spread_i4 ((gfc_array_i4 *) ret, (gfc_array_i4 *) source,
		 *along, *pncopies);
      return;

    case GFC_DTYPE_LOGICAL_8:
    case GFC_DTYPE_INTEGER_8:
      spread_i8 ((gfc_array_i8 *) ret, (gfc_array_i8 *) source,
		 *along, *pncopies);
      return;

#ifdef HAVE_GFC_INTEGER_16
    case GFC_DTYPE_LOGICAL_16:
    case GFC_DTYPE_INTEGER_16:
      spread_i16 ((gfc_array_i16 *) ret, (gfc_array_i16 *) source,
		 *along, *pncopies);
      return;
#endif

    case GFC_DTYPE_REAL_4:
      spread_r4 ((gfc_array_r4 *) ret, (gfc_array_r4 *) source,
		 *along, *pncopies);
      return;

    case GFC_DTYPE_REAL_8:
      spread_r8 ((gfc_array_r8 *) ret, (gfc_array_r8 *) source,
		 *along, *pncopies);
      return;

#ifdef GFC_HAVE_REAL_10
    case GFC_DTYPE_REAL_10:
      spread_r10 ((gfc_array_r10 *) ret, (gfc_array_r10 *) source,
		 *along, *pncopies);
      return;
#endif

#ifdef GFC_HAVE_REAL_16
    case GFC_DTYPE_REAL_16:
      spread_r16 ((gfc_array_r16 *) ret, (gfc_array_r16 *) source,
		 *along, *pncopies);
      return;
#endif

    case GFC_DTYPE_COMPLEX_4:
      spread_c4 ((gfc_array_c4 *) ret, (gfc_array_c4 *) source,
		 *along, *pncopies);
      return;

    case GFC_DTYPE_COMPLEX_8:
      spread_c8 ((gfc_array_c8 *) ret, (gfc_array_c8 *) source,
		 *along, *pncopies);
      return;

#ifdef GFC_HAVE_COMPLEX_10
    case GFC_DTYPE_COMPLEX_10:
      spread_c10 ((gfc_array_c10 *) ret, (gfc_array_c10 *) source,
		 *along, *pncopies);
      return;
#endif

#ifdef GFC_HAVE_COMPLEX_16
    case GFC_DTYPE_COMPLEX_16:
      spread_c16 ((gfc_array_c16 *) ret, (gfc_array_c16 *) source,
		 *along, *pncopies);
      return;
#endif

    case GFC_DTYPE_DERIVED_2:
      if (GFC_UNALIGNED_2(ret->data) || GFC_UNALIGNED_2(source->data))
	break;
      else
	{
	  spread_i2 ((gfc_array_i2 *) ret, (gfc_array_i2 *) source,
		     *along, *pncopies);
	  return;
	}

    case GFC_DTYPE_DERIVED_4:
      if (GFC_UNALIGNED_4(ret->data) || GFC_UNALIGNED_4(source->data))
	break;
      else
	{
	  spread_i4 ((gfc_array_i4 *) ret, (gfc_array_i4 *) source,
		     *along, *pncopies);
	  return;
	}

    case GFC_DTYPE_DERIVED_8:
      if (GFC_UNALIGNED_8(ret->data) || GFC_UNALIGNED_8(source->data))
	break;
      else
	{
	  spread_i8 ((gfc_array_i8 *) ret, (gfc_array_i8 *) source,
		     *along, *pncopies);
	  return;
	}

#ifdef HAVE_GFC_INTEGER_16
    case GFC_DTYPE_DERIVED_16:
      if (GFC_UNALIGNED_16(ret->data) || GFC_UNALIGNED_16(source->data))
	break;
      else
	{
	  spread_i16 ((gfc_array_i16 *) ret, (gfc_array_i16 *) source,
		      *along, *pncopies);
	  return;
	}
#endif
    }

  spread_internal (ret, source, along, pncopies, GFC_DESCRIPTOR_SIZE (source));
}


extern void spread_char (gfc_array_char *, GFC_INTEGER_4,
			 const gfc_array_char *, const index_type *,
			 const index_type *, GFC_INTEGER_4);
export_proto(spread_char);

void
spread_char (gfc_array_char *ret,
	     GFC_INTEGER_4 ret_length __attribute__((unused)),
	     const gfc_array_char *source, const index_type *along,
	     const index_type *pncopies, GFC_INTEGER_4 source_length)
{
  spread_internal (ret, source, along, pncopies, source_length);
}


extern void spread_char4 (gfc_array_char *, GFC_INTEGER_4,
			  const gfc_array_char *, const index_type *,
			  const index_type *, GFC_INTEGER_4);
export_proto(spread_char4);

void
spread_char4 (gfc_array_char *ret,
	      GFC_INTEGER_4 ret_length __attribute__((unused)),
	      const gfc_array_char *source, const index_type *along,
	      const index_type *pncopies, GFC_INTEGER_4 source_length)
{
  spread_internal (ret, source, along, pncopies,
		   source_length * sizeof (gfc_char4_t));
}


/* The following are the prototypes for the versions of spread with a
   scalar source.  */

extern void spread_scalar (gfc_array_char *, const char *,
			   const index_type *, const index_type *);
export_proto(spread_scalar);

void
spread_scalar (gfc_array_char *ret, const char *source,
	       const index_type *along, const index_type *pncopies)
{
  index_type type_size;

  if (!ret->dtype)
    runtime_error ("return array missing descriptor in spread()");

  type_size = GFC_DTYPE_TYPE_SIZE(ret);
  switch(type_size)
    {
    case GFC_DTYPE_DERIVED_1:
    case GFC_DTYPE_LOGICAL_1:
    case GFC_DTYPE_INTEGER_1:
      spread_scalar_i1 ((gfc_array_i1 *) ret, (GFC_INTEGER_1 *) source,
			*along, *pncopies);
      return;

    case GFC_DTYPE_LOGICAL_2:
    case GFC_DTYPE_INTEGER_2:
      spread_scalar_i2 ((gfc_array_i2 *) ret, (GFC_INTEGER_2 *) source,
			*along, *pncopies);
      return;

    case GFC_DTYPE_LOGICAL_4:
    case GFC_DTYPE_INTEGER_4:
      spread_scalar_i4 ((gfc_array_i4 *) ret, (GFC_INTEGER_4 *) source,
			*along, *pncopies);
      return;

    case GFC_DTYPE_LOGICAL_8:
    case GFC_DTYPE_INTEGER_8:
      spread_scalar_i8 ((gfc_array_i8 *) ret, (GFC_INTEGER_8 *) source,
			*along, *pncopies);
      return;

#ifdef HAVE_GFC_INTEGER_16
    case GFC_DTYPE_LOGICAL_16:
    case GFC_DTYPE_INTEGER_16:
      spread_scalar_i16 ((gfc_array_i16 *) ret, (GFC_INTEGER_16 *) source,
			*along, *pncopies);
      return;
#endif

    case GFC_DTYPE_REAL_4:
      spread_scalar_r4 ((gfc_array_r4 *) ret, (GFC_REAL_4 *) source,
			*along, *pncopies);
      return;

    case GFC_DTYPE_REAL_8:
      spread_scalar_r8 ((gfc_array_r8 *) ret, (GFC_REAL_8 *) source,
			*along, *pncopies);
      return;

#ifdef HAVE_GFC_REAL_10
    case GFC_DTYPE_REAL_10:
      spread_scalar_r10 ((gfc_array_r10 *) ret, (GFC_REAL_10 *) source,
			*along, *pncopies);
      return;
#endif

#ifdef HAVE_GFC_REAL_16
    case GFC_DTYPE_REAL_16:
      spread_scalar_r16 ((gfc_array_r16 *) ret, (GFC_REAL_16 *) source,
			*along, *pncopies);
      return;
#endif

    case GFC_DTYPE_COMPLEX_4:
      spread_scalar_c4 ((gfc_array_c4 *) ret, (GFC_COMPLEX_4 *) source,
			*along, *pncopies);
      return;

    case GFC_DTYPE_COMPLEX_8:
      spread_scalar_c8 ((gfc_array_c8 *) ret, (GFC_COMPLEX_8 *) source,
			*along, *pncopies);
      return;

#ifdef HAVE_GFC_COMPLEX_10
    case GFC_DTYPE_COMPLEX_10:
      spread_scalar_c10 ((gfc_array_c10 *) ret, (GFC_COMPLEX_10 *) source,
			*along, *pncopies);
      return;
#endif

#ifdef HAVE_GFC_COMPLEX_16
    case GFC_DTYPE_COMPLEX_16:
      spread_scalar_c16 ((gfc_array_c16 *) ret, (GFC_COMPLEX_16 *) source,
			*along, *pncopies);
      return;
#endif

    case GFC_DTYPE_DERIVED_2:
      if (GFC_UNALIGNED_2(ret->data) || GFC_UNALIGNED_2(source))
	break;
      else
	{
	  spread_scalar_i2 ((gfc_array_i2 *) ret, (GFC_INTEGER_2 *) source,
			    *along, *pncopies);
	  return;
	}

    case GFC_DTYPE_DERIVED_4:
      if (GFC_UNALIGNED_4(ret->data) || GFC_UNALIGNED_4(source))
	break;
      else
	{
	  spread_scalar_i4 ((gfc_array_i4 *) ret, (GFC_INTEGER_4 *) source,
			    *along, *pncopies);
	  return;
	}

    case GFC_DTYPE_DERIVED_8:
      if (GFC_UNALIGNED_8(ret->data) || GFC_UNALIGNED_8(source))
	break;
      else
	{
	  spread_scalar_i8 ((gfc_array_i8 *) ret, (GFC_INTEGER_8 *) source,
			    *along, *pncopies);
	  return;
	}
#ifdef HAVE_GFC_INTEGER_16
    case GFC_DTYPE_DERIVED_16:
      if (GFC_UNALIGNED_16(ret->data) || GFC_UNALIGNED_16(source))
	break;
      else
	{
	  spread_scalar_i16 ((gfc_array_i16 *) ret, (GFC_INTEGER_16 *) source,
			     *along, *pncopies);
	  return;
	}
#endif
    }

  spread_internal_scalar (ret, source, along, pncopies, GFC_DESCRIPTOR_SIZE (ret));
}


extern void spread_char_scalar (gfc_array_char *, GFC_INTEGER_4,
				const char *, const index_type *,
				const index_type *, GFC_INTEGER_4);
export_proto(spread_char_scalar);

void
spread_char_scalar (gfc_array_char *ret,
		    GFC_INTEGER_4 ret_length __attribute__((unused)),
		    const char *source, const index_type *along,
		    const index_type *pncopies, GFC_INTEGER_4 source_length)
{
  if (!ret->dtype)
    runtime_error ("return array missing descriptor in spread()");
  spread_internal_scalar (ret, source, along, pncopies, source_length);
}


extern void spread_char4_scalar (gfc_array_char *, GFC_INTEGER_4,
				 const char *, const index_type *,
				 const index_type *, GFC_INTEGER_4);
export_proto(spread_char4_scalar);

void
spread_char4_scalar (gfc_array_char *ret,
		     GFC_INTEGER_4 ret_length __attribute__((unused)),
		     const char *source, const index_type *along,
		     const index_type *pncopies, GFC_INTEGER_4 source_length)
{
  if (!ret->dtype)
    runtime_error ("return array missing descriptor in spread()");
  spread_internal_scalar (ret, source, along, pncopies,
			  source_length * sizeof (gfc_char4_t));
}

