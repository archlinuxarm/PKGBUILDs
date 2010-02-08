/* Copyright (C) 1992,93,94,95,96,97,2002 Free Software Foundation, Inc.
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

#include <errno.h>
#include <sys/stat.h>
#include <stddef.h>

#include "xstatconv.c"

int
__lxstat (int vers, const char *file, struct stat *buf)
{
  struct stat64 buf64;
  return __lxstat64 (vers, file, &buf64) ?: xstat64_conv (buf, &buf64);
}
hidden_def (__lxstat)
weak_alias (__lxstat, _lxstat)
