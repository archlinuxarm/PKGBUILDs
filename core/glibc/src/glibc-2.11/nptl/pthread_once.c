/* Copyright (C) 2002, 2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include "pthreadP.h"
#include <lowlevellock.h>



static int once_lock = LLL_LOCK_INITIALIZER;


int
__pthread_once (once_control, init_routine)
     pthread_once_t *once_control;
     void (*init_routine) (void);
{
  /* XXX Depending on whether the LOCK_IN_ONCE_T is defined use a
     global lock variable or one which is part of the pthread_once_t
     object.  */
  if (*once_control == PTHREAD_ONCE_INIT)
    {
      lll_lock (once_lock, LLL_PRIVATE);

      /* XXX This implementation is not complete.  It doesn't take
	 cancelation and fork into account.  */
      if (*once_control == PTHREAD_ONCE_INIT)
	{
	  init_routine ();

	  *once_control = !PTHREAD_ONCE_INIT;
	}

      lll_unlock (once_lock, LLL_PRIVATE);
    }

  return 0;
}
strong_alias (__pthread_once, pthread_once)
