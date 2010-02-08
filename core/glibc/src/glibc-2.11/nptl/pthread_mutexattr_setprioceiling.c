/* Change priority ceiling setting in pthread_mutexattr_t.
   Copyright (C) 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2006.

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
#include <pthreadP.h>


int
pthread_mutexattr_setprioceiling (attr, prioceiling)
     pthread_mutexattr_t *attr;
     int prioceiling;
{
  if (__sched_fifo_min_prio == -1)
    __init_sched_fifo_prio ();

  if (__builtin_expect (prioceiling < __sched_fifo_min_prio, 0)
      || __builtin_expect (prioceiling > __sched_fifo_max_prio, 0)
      || __builtin_expect ((prioceiling
			    & (PTHREAD_MUTEXATTR_PRIO_CEILING_MASK
			       >> PTHREAD_MUTEXATTR_PRIO_CEILING_SHIFT))
			   != prioceiling, 0))
    return EINVAL;

  struct pthread_mutexattr *iattr = (struct pthread_mutexattr *) attr;

  iattr->mutexkind = ((iattr->mutexkind & ~PTHREAD_MUTEXATTR_PRIO_CEILING_MASK)
		      | (prioceiling << PTHREAD_MUTEXATTR_PRIO_CEILING_SHIFT));

  return 0;
}
