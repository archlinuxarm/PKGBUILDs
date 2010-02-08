/* Copyright (C) 1993, 94, 97, 98 Free Software Foundation, Inc.
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

#include <hurd.h>
#include <hurd/id.h>

/* Set the uid set for the current user to UIDS (N of them).  */
int
seteuids (int n, const uid_t *uids)
{
  error_t err;
  auth_t newauth;
  int i;
  gid_t new[n];

  /* Fault before taking locks.  */
  for (i = 0; i < n; ++i)
    new[i] = uids[i];

  HURD_CRITICAL_BEGIN;
  __mutex_lock (&_hurd_id.lock);
  err = _hurd_check_ids ();
  if (! err)
    {
      /* Get a new auth port using those IDs.  */
      err = __USEPORT (AUTH,
		       __auth_makeauth (port, NULL, 0, 0,
					new, n,
					_hurd_id.aux.uids, _hurd_id.aux.nuids,
					_hurd_id.gen.gids, _hurd_id.gen.ngids,
					_hurd_id.aux.gids, _hurd_id.aux.ngids,
					&newauth));
    }
  __mutex_unlock (&_hurd_id.lock);
  HURD_CRITICAL_END;

  if (err)
    return __hurd_fail (err);

  /* Install the new auth port and reauthenticate everything.  */
  err = __setauth (newauth);
  __mach_port_deallocate (__mach_task_self (), newauth);
  return err;
}
