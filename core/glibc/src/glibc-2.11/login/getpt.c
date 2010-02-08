/* Copyright (C) 1998, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Zack Weinberg <zack@rabi.phys.columbia.edu>, 1998.

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
#include <errno.h>

/* Open the master side of a pseudoterminal and return its file
   descriptor, or -1 on error. */
int
__getpt ()
{
  __set_errno (ENOSYS);
  return -1;
}
weak_alias (__getpt, getpt)

/* We cannot define posix_openpt in general for BSD systems.  */
int
__posix_openpt (oflag)
     int oflag;
{
  __set_errno (ENOSYS);
  return -1;
}
weak_alias (__posix_openpt, posix_openpt)

stub_warning (getpt)
stub_warning (posix_openpt)
#include <stub-tag.h>
