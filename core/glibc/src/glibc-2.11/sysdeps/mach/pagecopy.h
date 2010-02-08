/* Macros for copying by pages; used in memcpy, memmove.  Mach version.
   Copyright (C) 1995, 1997 Free Software Foundation, Inc.
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

#include <mach.h>

/* Threshold at which vm_copy is more efficient than well-optimized copying
   by words.  This parameter should be tuned as necessary.  */
#define PAGE_THRESHOLD		(2 * PAGE_SIZE)	/* XXX ? */

#define PAGE_SIZE		__vm_page_size
#define PAGE_COPY_FWD(dstp, srcp, nbytes_left, nbytes)			      \
  ((nbytes_left) = ((nbytes) -						      \
		    (__vm_copy (__mach_task_self (),			      \
				(vm_address_t) srcp, trunc_page (nbytes),     \
				(vm_address_t) dstp) == KERN_SUCCESS	      \
		     ? trunc_page (nbytes)				      \
		     : 0)))

/* Get the generic macro.  */
#include <sysdeps/generic/pagecopy.h>
