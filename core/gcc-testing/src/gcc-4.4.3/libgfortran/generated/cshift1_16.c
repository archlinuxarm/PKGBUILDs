/* Implementation of the CSHIFT intrinsic
   Copyright 2003, 2007, 2009 Free Software Foundation, Inc.
   Contributed by Feng Wang <wf_cs@yahoo.com>

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


#if defined (HAVE_GFC_INTEGER_16)

static void
cshift1 (gfc_array_char * const restrict ret, 
	const gfc_array_char * const restrict array,
	const gfc_array_i16 * const restrict h, 
	const GFC_INTEGER_16 * const restrict pwhich, 
	index_type size)
{
  /* r.* indicates the return array.  */
  index_type rstride[GFC_MAX_DIMENSIONS];
  index_type rstride0;
  index_type roffset;
  char *rptr;
  char *dest;
  /* s.* indicates the source array.  */
  index_type sstride[GFC_MAX_DIMENSIONS];
  index_type sstride0;
  index_type soffset;
  const char *sptr;
  const char *src;
  /* h.* indicates the shift array.  */
  index_type hstride[GFC_MAX_DIMENSIONS];
  index_type hstride0;
  const GFC_INTEGER_16 *hptr;

  index_type count[GFC_MAX_DIMENSIONS];
  index_type extent[GFC_MAX_DIMENSIONS];
  index_type dim;
  index_type len;
  index_type n;
  int which;
  GFC_INTEGER_16 sh;
  index_type arraysize;

  if (pwhich)
    which = *pwhich - 1;
  else
    which = 0;

  if (which < 0 || (which + 1) > GFC_DESCRIPTOR_RANK (array))
    runtime_error ("Argument 'DIM' is out of range in call to 'CSHIFT'");

  arraysize = size0 ((array_t *)array);

  if (ret->data == NULL)
    {
      int i;

      ret->data = internal_malloc_size (size * arraysize);
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

  if (arraysize == 0)
    return;

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

          hstride[n] = h->dim[n].stride;
          n++;
        }
    }
  if (sstride[0] == 0)
    sstride[0] = size;
  if (rstride[0] == 0)
    rstride[0] = size;
  if (hstride[0] == 0)
    hstride[0] = 1;

  dim = GFC_DESCRIPTOR_RANK (array);
  rstride0 = rstride[0];
  sstride0 = sstride[0];
  hstride0 = hstride[0];
  rptr = ret->data;
  sptr = array->data;
  hptr = h->data;

  while (rptr)
    {
      /* Do the shift for this dimension.  */
      sh = *hptr;
      sh = (div (sh, len)).rem;
      if (sh < 0)
        sh += len;

      src = &sptr[sh * soffset];
      dest = rptr;

      for (n = 0; n < len; n++)
        {
          memcpy (dest, src, size);
          dest += roffset;
          if (n == len - sh - 1)
            src = sptr;
          else
            src += soffset;
        }

      /* Advance to the next section.  */
      rptr += rstride0;
      sptr += sstride0;
      hptr += hstride0;
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
	  hptr -= hstride[n] * extent[n];
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
	      hptr += hstride[n];
            }
        }
    }
}

void cshift1_16 (gfc_array_char * const restrict, 
	const gfc_array_char * const restrict,
	const gfc_array_i16 * const restrict, 
	const GFC_INTEGER_16 * const restrict);
export_proto(cshift1_16);

void
cshift1_16 (gfc_array_char * const restrict ret,
	const gfc_array_char * const restrict array,
	const gfc_array_i16 * const restrict h, 
	const GFC_INTEGER_16 * const restrict pwhich)
{
  cshift1 (ret, array, h, pwhich, GFC_DESCRIPTOR_SIZE (array));
}


void cshift1_16_char (gfc_array_char * const restrict ret, 
	GFC_INTEGER_4,
	const gfc_array_char * const restrict array,
	const gfc_array_i16 * const restrict h, 
	const GFC_INTEGER_16 * const restrict pwhich,
	GFC_INTEGER_4);
export_proto(cshift1_16_char);

void
cshift1_16_char (gfc_array_char * const restrict ret,
	GFC_INTEGER_4 ret_length __attribute__((unused)),
	const gfc_array_char * const restrict array,
	const gfc_array_i16 * const restrict h, 
	const GFC_INTEGER_16 * const restrict pwhich,
	GFC_INTEGER_4 array_length)
{
  cshift1 (ret, array, h, pwhich, array_length);
}


void cshift1_16_char4 (gfc_array_char * const restrict ret, 
	GFC_INTEGER_4,
	const gfc_array_char * const restrict array,
	const gfc_array_i16 * const restrict h, 
	const GFC_INTEGER_16 * const restrict pwhich,
	GFC_INTEGER_4);
export_proto(cshift1_16_char4);

void
cshift1_16_char4 (gfc_array_char * const restrict ret,
	GFC_INTEGER_4 ret_length __attribute__((unused)),
	const gfc_array_char * const restrict array,
	const gfc_array_i16 * const restrict h, 
	const GFC_INTEGER_16 * const restrict pwhich,
	GFC_INTEGER_4 array_length)
{
  cshift1 (ret, array, h, pwhich, array_length * sizeof (gfc_char4_t));
}

#endif
