/* Copyright (C) 1997, 1998, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Mark Kettenis <kettenis@phys.uva.nl>, 1997.

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

#include <utmp.h>

#include "utmp-private.h"

#ifndef TRANSFORM_UTMP_FILE_NAME
# define TRANSFORM_UTMP_FILE_NAME(file_name) (file_name)
#endif

void
__updwtmp (const char *wtmp_file, const struct utmp *utmp)
{
  const char *file_name = TRANSFORM_UTMP_FILE_NAME (wtmp_file);

  (*__libc_utmp_file_functions.updwtmp) (file_name, utmp);
}
weak_alias (__updwtmp, updwtmp)
