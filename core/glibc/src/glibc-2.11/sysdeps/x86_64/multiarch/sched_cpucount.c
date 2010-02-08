/* Count bits in CPU set.  x86-64 multi-arch version.
   This file is part of the GNU C Library.
   Copyright (C) 2008, 2009 Free Software Foundation, Inc.
   Contributed by Ulrich Drepper <drepper@redhat.com>.

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

#include <sched.h>
#include "init-arch.h"

#define __sched_cpucount static generic_cpucount
#include <posix/sched_cpucount.c>
#undef __sched_cpucount

#define POPCNT(l) \
  ({ __cpu_mask r; \
     asm ("popcnt %1, %0" : "=r" (r) : "0" (l));\
     r; })
#define __sched_cpucount static popcount_cpucount
#include <posix/sched_cpucount.c>
#undef __sched_cpucount

libc_ifunc (__sched_cpucount,
	    HAS_POPCOUNT ? popcount_cpucount : generic_cpucount);
