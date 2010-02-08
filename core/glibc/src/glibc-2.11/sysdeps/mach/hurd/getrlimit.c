/* Copyright (C) 1991,93,94,96,97,2000 Free Software Foundation, Inc.
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

#include <sys/resource.h>
#include <errno.h>
#include <hurd.h>
#include <hurd/resource.h>

/* Put the soft and hard limits for RESOURCE in *RLIMITS.
   Returns 0 if successful, -1 if not (and sets errno).  */
int
__getrlimit (enum __rlimit_resource resource, struct rlimit *rlimits)
{
  struct rlimit lim;

  if (rlimits == NULL || (unsigned int) resource >= RLIMIT_NLIMITS)
    {
      errno = EINVAL;
      return -1;
    }

  HURD_CRITICAL_BEGIN;
  __mutex_lock (&_hurd_rlimit_lock);
  lim = _hurd_rlimits[resource];
  __mutex_unlock (&_hurd_rlimit_lock);
  HURD_CRITICAL_END;

  *rlimits = lim;

  return 0;
}
weak_alias (__getrlimit, getrlimit)
