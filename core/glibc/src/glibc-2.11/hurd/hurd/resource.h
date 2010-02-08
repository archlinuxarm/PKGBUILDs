/* Resource limits for the Hurd.
   Copyright (C) 1994, 1995, 1997 Free Software Foundation, Inc.
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

#ifndef _HURD_RESOURCE_H
#define _HURD_RESOURCE_H

#include <sys/types.h>
#include <sys/resource.h>
#include <errno.h>
#include <hurd/process.h>

/* This array contains the current resource limits for the process.  */
extern struct rlimit _hurd_rlimits[RLIM_NLIMITS];
extern struct mutex _hurd_rlimit_lock; /* Locks _hurd_rlimits.  */


/* Helper function for getpriority and setpriority.  Maps FN over all the
   processes specified by WHICH and WHO.  PI is non-null if a
   proc_getprocinfo was already done; FN may use *PI arbitrarily, it is
   reset on the next call; PI_FLAGS is passed to proc_getprocinfo.  Returns
   FN's result the first time it returns nonzero.  If FN never returns
   nonzero, this returns zero.  */
extern error_t _hurd_priority_which_map (enum __priority_which which, int who,
					 error_t (*fn) (pid_t pid,
							struct procinfo *pi),
					 int pi_flags);

/* Convert between Mach priority values and the priority
   values used by getpriority, setpriority, and nice.  */
#define MACH_PRIORITY_TO_NICE(prio) (2 * ((prio) - 12))
#define NICE_TO_MACH_PRIORITY(nice) (12 + ((nice) / 2))




#endif
