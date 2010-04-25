/* Generic implementation of the EOSHIFT intrinsic
   Copyright 2002, 2005, 2007, 2009 Free Software Foundation, Inc.
   Contributed by Paul Brook <paul@nowt.org>

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

/* TODO: make this work for large shifts when
   sizeof(int) < sizeof (index_type).  */

static void
eoshift0 (gfc_array_char * ret, const gfc_array_char * array,
	  int shift, const char * pbound, int which, index_type size,
	  const char *filler, index_type filler_len)
{
  /* r.* indicates the return array.  */
  index_type rstride[GFC_MAX_DIMENSIONS];
  index_type rstride0;
  index_type roffset;
  char * restrict rptr;
  char *dest;
  /* s.* indicates the source array.  */
  index_type sstride[GFC_MAX_DIMENSIONS];
  index_type sstride0;
  index_type soffset;
  const char *sptr;
  const char *src;

  index_type count[GFC_MAX_DIMENSIONS];
  index_type extent[GFC_MAX_DIMENSIONS];
  index_type dim;
  index_type len;
  index_type n;

  /* The compiler cannot figure out that these are set, initialize
     them to avoid warnings.  */
  len = 0;
  soffset = 0;
  roffset = 0;

  if (ret->data == NULL)
    {
      int i;

      ret->data = internal_malloc_size (size * size0 ((array_t *)array));
      ret->offset = 0;
      ret->dtype = array->dtype;
      for (i = 0; i < GFC_DESCRIPTOR_RANK (array); i++)
        {
          ret->dim[i].lbound = 0;
          ret->dim[i].ubound = array->dim[i].ubound - array->dim[i].lbound;

          if (i == 0)
            ret->dim[i].stride = 1;
          else
            ret->dim[i].stride = (ret->dim[i-1].ubound + 1) * ret->dim[i-1].stride;
        }
    }
  else
    {
      if (size0 ((array_t *) ret) == 0)
	return;
    }

  which = which - 1;

  extent[0] = 1;
  count[0] = 0;
  sstride[0] = -1;
  rstride[0] = -1;
  n = 0;
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

  if ((shift >= 0 ? shift : -shift) > len)
    {
      shift = len;
      len = 0;
    }
  else
    {
      if (shift > 0)
	len = len - shift;
      else
	len = len + shift;
    }

  while (rptr)
    {
      /* Do the shift for this dimension.  */
      if (shift > 0)
        {
          src = &sptr[shift * soffset];
          dest = rptr;
        }
      else
        {
          src = sptr;
          dest = &rptr[-shift * roffset];
        }
      for (n = 0; n < len; n++)
        {
          memcpy (dest, src, size);
          dest += roffset;
          src += soffset;
        }
      if (shift >= 0)
        {
          n = shift;
        }
      else
        {
          dest = rptr;
          n = -shift;
        }

      if (pbound)
	while (n--)
	  {
	    memcpy (dest, pbound, size);
	    dest += roffset;
	  }
      else
	while (n--)
	  {
	    index_type i;

	    if (filler_len == 1)
	      memset (dest, filler[0], size);
	    else
	      for (i = 0; i < size ; i += filler_len)
		memcpy (&dest[i], filler, filler_len);

	    dest += roffset;
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


#define DEFINE_EOSHIFT(N)						      \
  extern void eoshift0_##N (gfc_array_char *, const gfc_array_char *,	      \
			    const GFC_INTEGER_##N *, const char *,	      \
			    const GFC_INTEGER_##N *);			      \
  export_proto(eoshift0_##N);						      \
									      \
  void									      \
  eoshift0_##N (gfc_array_char *ret, const gfc_array_char *array,	      \
		const GFC_INTEGER_##N *pshift, const char *pbound,	      \
		const GFC_INTEGER_##N *pdim)				      \
  {									      \
    eoshift0 (ret, array, *pshift, pbound, pdim ? *pdim : 1,		      \
	      GFC_DESCRIPTOR_SIZE (array), "\0", 1);			      \
  }									      \
									      \
  extern void eoshift0_##N##_char (gfc_array_char *, GFC_INTEGER_4,	      \
				   const gfc_array_char *,		      \
				   const GFC_INTEGER_##N *, const char *,     \
				   const GFC_INTEGER_##N *, GFC_INTEGER_4,    \
				   GFC_INTEGER_4);			      \
  export_proto(eoshift0_##N##_char);					      \
									      \
  void									      \
  eoshift0_##N##_char (gfc_array_char *ret,				      \
		       GFC_INTEGER_4 ret_length __attribute__((unused)),      \
		       const gfc_array_char *array,			      \
		       const GFC_INTEGER_##N *pshift,			      \
		       const char *pbound,				      \
		       const GFC_INTEGER_##N *pdim,			      \
		       GFC_INTEGER_4 array_length,			      \
		       GFC_INTEGER_4 bound_length __attribute__((unused)))    \
  {									      \
    eoshift0 (ret, array, *pshift, pbound, pdim ? *pdim : 1,		      \
	      array_length, " ", 1);					      \
  }									      \
									      \
  extern void eoshift0_##N##_char4 (gfc_array_char *, GFC_INTEGER_4,	      \
				    const gfc_array_char *,		      \
				    const GFC_INTEGER_##N *, const char *,    \
				    const GFC_INTEGER_##N *, GFC_INTEGER_4,   \
				    GFC_INTEGER_4);			      \
  export_proto(eoshift0_##N##_char4);					      \
									      \
  void									      \
  eoshift0_##N##_char4 (gfc_array_char *ret,				      \
			GFC_INTEGER_4 ret_length __attribute__((unused)),     \
			const gfc_array_char *array,			      \
			const GFC_INTEGER_##N *pshift,			      \
			const char *pbound,				      \
			const GFC_INTEGER_##N *pdim,			      \
			GFC_INTEGER_4 array_length,			      \
			GFC_INTEGER_4 bound_length __attribute__((unused)))   \
  {									      \
    static const gfc_char4_t space = (unsigned char) ' ';		      \
    eoshift0 (ret, array, *pshift, pbound, pdim ? *pdim : 1,		      \
	      array_length * sizeof (gfc_char4_t), (const char *) &space,     \
	      sizeof (gfc_char4_t));					      \
  }

DEFINE_EOSHIFT (1);
DEFINE_EOSHIFT (2);
DEFINE_EOSHIFT (4);
DEFINE_EOSHIFT (8);
#ifdef HAVE_GFC_INTEGER_16
DEFINE_EOSHIFT (16);
#endif
