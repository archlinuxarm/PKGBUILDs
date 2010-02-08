/* Access to extended attributes on files.  Hurd version.
   Copyright (C) 2005 Free Software Foundation, Inc.
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
#include <sys/xattr.h>
#include <hurd.h>
#include <hurd/xattr.h>
#include <hurd/fd.h>

int
fremovexattr (int fd, const char *name)
{
  error_t err;

  err = HURD_DPORT_USE (fd, _hurd_xattr_remove (port, name));

  return err ? __hurd_dfail (fd, err) : 0;
}
