/* msync -- Synchronize mapped memory to external storage.  Mach version.
   Copyright (C) 2002, 2005 Free Software Foundation, Inc.
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

#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <mach.h>

/* Some Mach variants have vm_msync and some don't.  Those that have it
   define the VM_SYNC_* bits when we include <mach/mach_types.h>.  */

#ifndef VM_SYNC_SYNCHRONOUS
# include <misc/msync.c>
#else

/* Synchronize the region starting at ADDR and extending LEN bytes with the
   file it maps.  Filesystem operations on a file being mapped are
   unpredictable before this is done.  */

int
msync (__ptr_t addr, size_t len, int flags)
{
  vm_sync_t sync_flags = 0;
  kern_return_t err;

  if (flags & MS_SYNC)
    sync_flags |= VM_SYNC_SYNCHRONOUS;
  if (flags & MS_ASYNC)
    sync_flags |= VM_SYNC_ASYNCHRONOUS;
  if (flags & MS_INVALIDATE)
    sync_flags |= VM_SYNC_INVALIDATE;

  if (err = __vm_msync (__mach_task_self (),
			(vm_address_t) addr, (vm_size_t) len, sync_flags))
    {
      errno = err;
      return -1;
    }
  return 0;
}
#endif
