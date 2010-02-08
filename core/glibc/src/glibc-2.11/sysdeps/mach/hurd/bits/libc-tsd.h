/* libc-internal interface for thread-specific data.  Hurd version.
   Copyright (C) 1998,2002,2008 Free Software Foundation, Inc.
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

#ifndef _BITS_LIBC_TSD_H
#define _BITS_LIBC_TSD_H 1

#include <hurd/threadvar.h>

#define __libc_tsd_define(CLASS, TYPE, KEY) /* nothing, always have threadvars */

#define __libc_tsd_address(TYPE, KEY) \
  ((TYPE *) __hurd_threadvar_location (_HURD_THREADVAR_##KEY))

#define __libc_tsd_get(TYPE, KEY) \
  (*__libc_tsd_address (TYPE, KEY))
#define __libc_tsd_set(TYPE, KEY, VALUE) \
  (*__libc_tsd_address (TYPE, KEY) = (VALUE))

#endif	/* bits/libc-tsd.h */
