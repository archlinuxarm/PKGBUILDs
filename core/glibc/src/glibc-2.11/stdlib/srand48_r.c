/* Copyright (C) 1995, 1996, 1997, 1998, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, August 1995.

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

#include <stdlib.h>
#include <limits.h>

int
__srand48_r (seedval, buffer)
     long int seedval;
     struct drand48_data *buffer;
{
  /* The standards say we only have 32 bits.  */
  if (sizeof (long int) > 4)
    seedval &= 0xffffffffl;

  buffer->__x[2] = seedval >> 16;
  buffer->__x[1] = seedval & 0xffffl;
  buffer->__x[0] = 0x330e;

  buffer->__a = 0x5deece66dull;
  buffer->__c = 0xb;
  buffer->__init = 1;

  return 0;
}
weak_alias (__srand48_r, srand48_r)
