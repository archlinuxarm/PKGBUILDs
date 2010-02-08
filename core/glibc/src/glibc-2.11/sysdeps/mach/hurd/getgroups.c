/* Copyright (C) 1993, 1994, 1995, 1997 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <hurd.h>
#include <hurd/id.h>
#include <string.h>

int
__getgroups (n, gidset)
     int n;
     gid_t *gidset;
{
  error_t err;
  int ngids;
  void *crit;

  crit = _hurd_critical_section_lock ();
  __mutex_lock (&_hurd_id.lock);

  if (err = _hurd_check_ids ())
    {
      __mutex_unlock (&_hurd_id.lock);
      _hurd_critical_section_unlock (crit);
      return __hurd_fail (err);
    }

  ngids = _hurd_id.gen.ngids;

  if (n != 0)
    {
      /* Copy the gids onto stack storage and then release the idlock.  */
      gid_t gids[ngids];
      memcpy (gids, _hurd_id.gen.gids, sizeof (gids));
      __mutex_unlock (&_hurd_id.lock);
      _hurd_critical_section_unlock (crit);

      /* Now that the lock is released, we can safely copy the
	 group set into the user's array, which might fault.  */
      if (ngids > n)
	ngids = n;
      memcpy (gidset, gids, ngids * sizeof (gid_t));
    }
  else
    {
      __mutex_unlock (&_hurd_id.lock);
      _hurd_critical_section_unlock (crit);
    }

  return ngids;
}

weak_alias (__getgroups, getgroups)
