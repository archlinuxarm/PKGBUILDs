/* Copyright (C) 1991,92,93,94,95,97,2002,2004 Free Software Foundation, Inc.
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

#ifndef	_MALLOC_INTERNAL
#define	_MALLOC_INTERNAL
#include <malloc.h>
#endif

#ifndef	__GNU_LIBRARY__
#define	__sbrk	sbrk
#endif

#ifdef __GNU_LIBRARY__
/* It is best not to declare this and cast its result on foreign operating
   systems with potentially hostile include files.  */

#include <stddef.h>
#include <stdlib.h>
extern __malloc_ptr_t __sbrk (ptrdiff_t increment) __THROW;
libc_hidden_proto (__sbrk)
#endif

#ifndef NULL
#define NULL 0
#endif

/* Allocate INCREMENT more bytes of data space,
   and return the start of data space, or NULL on errors.
   If INCREMENT is negative, shrink data space.  */
__malloc_ptr_t
__default_morecore (increment)
     __malloc_ptrdiff_t increment;
{
  __malloc_ptr_t result = (__malloc_ptr_t) __sbrk (increment);
  if (result == (__malloc_ptr_t) -1)
    return NULL;
  return result;
}
libc_hidden_def (__default_morecore)
