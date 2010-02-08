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
#include <sys/types.h>
#include <hurd/id.h>
#include <string.h>

/* Set the effective user ID of the calling process to GID.  */
int
setegid (gid)
     gid_t gid;
{
  auth_t newauth;
  error_t err;

  HURD_CRITICAL_BEGIN;
  __mutex_lock (&_hurd_id.lock);
  err = _hurd_check_ids ();

  if (!err)
    {
      /* Make a new auth handle which has EGID as the first element in the
         list of effective gids.  */

      if (_hurd_id.gen.ngids > 0)
	{
	  _hurd_id.gen.gids[0] = gid;
	  _hurd_id.valid = 0;
	}

      err = __USEPORT (AUTH, __auth_makeauth
		       (port, NULL, MACH_MSG_TYPE_COPY_SEND, 0,
			_hurd_id.gen.uids, _hurd_id.gen.nuids,
			_hurd_id.aux.uids, _hurd_id.aux.nuids,
			_hurd_id.gen.ngids ? _hurd_id.gen.gids : &gid,
			_hurd_id.gen.ngids ?: 1,
			_hurd_id.aux.gids, _hurd_id.aux.ngids,
			&newauth));
    }
  __mutex_unlock (&_hurd_id.lock);
  HURD_CRITICAL_END;

  if (err)
    return __hurd_fail (err);

  /* Install the new handle and reauthenticate everything.  */
  err = __setauth (newauth);
  __mach_port_deallocate (__mach_task_self (), newauth);
  return err;
}
libc_hidden_def (setegid)
