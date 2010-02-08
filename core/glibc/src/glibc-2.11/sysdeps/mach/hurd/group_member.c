/* `group_member' -- test if process is in a given group.  Hurd version.
   Copyright (C) 1993, 1994, 1995, 1997 Free Software Foundation, Inc.
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

int
__group_member (gid)
     gid_t gid;
{
  int member = 0;
  error_t err;
  void *crit;

  crit = _hurd_critical_section_lock ();
  __mutex_lock (&_hurd_id.lock);

  err = _hurd_check_ids ();
  if (! err)
    {
      size_t i;
      for (i = 0; i < _hurd_id.gen.ngids; ++i)
	if (_hurd_id.gen.gids[i] == gid)
	  {
	    member = 1;
	    break;
	  }
    }

  __mutex_unlock (&_hurd_id.lock);
  _hurd_critical_section_unlock (crit);

  if (err)
    __hurd_fail (err);
  return member;
}

weak_alias (__group_member, group_member)
