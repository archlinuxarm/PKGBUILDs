/* Resolve function pointers to VDSO functions.
   Copyright (C) 2005, 2007 Free Software Foundation, Inc.
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

#ifndef _LIBC_VDSO_H
#define _LIBC_VDSO_H

#include <time.h>
#include <sys/time.h>

#ifdef SHARED

extern long int (*__vdso_gettimeofday) (struct timeval *, void *)
  attribute_hidden;

extern long int (*__vdso_clock_gettime) (clockid_t, struct timespec *);

#endif

#endif /* _LIBC_VDSO_H */
