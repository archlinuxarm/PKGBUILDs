/* Lookup helper function for Hurd implementation of *at functions.
   Copyright (C) 2006 Free Software Foundation, Inc.
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
#include <hurd/lookup.h>
#include <hurd/fd.h>
#include <string.h>
#include <fcntl.h>

file_t
__file_name_lookup_at (int fd, int at_flags,
		       const char *file_name, int flags, mode_t mode)
{
  error_t err;
  file_t result;

  flags |= (at_flags & AT_SYMLINK_NOFOLLOW) ? O_NOLINK : 0;
  at_flags &= ~AT_SYMLINK_NOFOLLOW;
  if (at_flags != 0)
    return __hurd_fail (EINVAL);

  if (fd == AT_FDCWD || file_name[0] == '/')
    return __file_name_lookup (file_name, flags, mode);

  file_t startdir;
  error_t use_init_port (int which, error_t (*operate) (mach_port_t))
    {
      return (which == INIT_PORT_CWDIR ? (*operate) (startdir) :
	      _hurd_ports_use (which, operate));
    }

  err = HURD_DPORT_USE (fd, (startdir = port,
			     __hurd_file_name_lookup (&use_init_port,
						      &__getdport, NULL,
						      file_name,
						      flags,
						      mode & ~_hurd_umask,
						      &result)));

  return err ? (__hurd_dfail (fd, err), MACH_PORT_NULL) : result;
}

file_t
__file_name_split_at (int fd, const char *file_name, char **name)
{
  error_t err;
  file_t result;

  if (fd == AT_FDCWD || file_name[0] == '/')
    return __file_name_split (file_name, name);

  err = __hurd_file_name_split (&_hurd_ports_use, &__getdport, 0,
				file_name, &result, name);

  file_t startdir;
  error_t use_init_port (int which, error_t (*operate) (mach_port_t))
  {
    return (which == INIT_PORT_CWDIR ? (*operate) (startdir) :
	    _hurd_ports_use (which, operate));
  }

  err = HURD_DPORT_USE (fd, (startdir = port,
			     __hurd_file_name_split (&use_init_port,
						     &__getdport, 0,
						     file_name,
						     &result, name)));

  return err ? (__hurd_dfail (fd, err), MACH_PORT_NULL) : result;
}

file_t
__directory_name_split_at (int fd, const char *directory_name, char **name)
{
  error_t err;
  file_t result;

  if (fd == AT_FDCWD || directory_name[0] == '/')
    return __directory_name_split (directory_name, name);

  file_t startdir;
  error_t use_init_port (int which, error_t (*operate) (mach_port_t))
    {
      return (which == INIT_PORT_CWDIR ? (*operate) (startdir) :
	      _hurd_ports_use (which, operate));
    }

  err = HURD_DPORT_USE (fd, (startdir = port,
			     __hurd_directory_name_split (&use_init_port,
							  &__getdport, 0,
							  directory_name,
							  &result, name)));

  return err ? (__hurd_dfail (fd, err), MACH_PORT_NULL) : result;
}
