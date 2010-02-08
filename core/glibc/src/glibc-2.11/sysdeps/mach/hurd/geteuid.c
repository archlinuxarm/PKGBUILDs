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

/* Get the effective user ID of the calling process.  */
uid_t
__geteuid ()
{
  error_t err;
  uid_t euid;

  HURD_CRITICAL_BEGIN;
  __mutex_lock (&_hurd_id.lock);

  if (err = _hurd_check_ids ())
    {
      errno = err;
      euid = -1;
    }
  else if (_hurd_id.gen.nuids >= 1)
    euid = _hurd_id.gen.uids[0];
  else if (_hurd_id.aux.nuids >= 1)
    /* We have no effective uids.  Return the real uid.  */
    euid = _hurd_id.aux.uids[0];
  else
    {
      /* We do not even have a real uid.  */
      errno = EGRATUITOUS;
      euid = -1;
    }

  __mutex_unlock (&_hurd_id.lock);
  HURD_CRITICAL_END;

  return euid;
}

weak_alias (__geteuid, geteuid)
