/* Copyright (C) 2002, 2003, 2005, 2006 Free Software Foundation, Inc.
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

#include <errno.h>
#include <stdlib.h>

#include <atomic.h>
#include "pthreadP.h"


static void
cleanup (void *arg)
{
  /* If we already changed the waiter ID, reset it.  The call cannot
     fail for any reason but the thread not having done that yet so
     there is no reason for a loop.  */
  (void) atomic_compare_and_exchange_bool_acq ((struct pthread **) arg, NULL,
					       THREAD_SELF);
}


int
pthread_join (threadid, thread_return)
     pthread_t threadid;
     void **thread_return;
{
  struct pthread *pd = (struct pthread *) threadid;

  /* Make sure the descriptor is valid.  */
  if (INVALID_NOT_TERMINATED_TD_P (pd))
    /* Not a valid thread handle.  */
    return ESRCH;

  /* Is the thread joinable?.  */
  if (IS_DETACHED (pd))
    /* We cannot wait for the thread.  */
    return EINVAL;

  struct pthread *self = THREAD_SELF;
  int result = 0;

  /* During the wait we change to asynchronous cancellation.  If we
     are canceled the thread we are waiting for must be marked as
     un-wait-ed for again.  */
  pthread_cleanup_push (cleanup, &pd->joinid);

  /* Switch to asynchronous cancellation.  */
  int oldtype = CANCEL_ASYNC ();

  if ((pd == self
       || (self->joinid == pd
	   && (pd->cancelhandling
	       & (CANCELING_BITMASK | CANCELED_BITMASK | EXITING_BITMASK
		  | TERMINATED_BITMASK)) == 0))
      && !CANCEL_ENABLED_AND_CANCELED (self->cancelhandling))
    /* This is a deadlock situation.  The threads are waiting for each
       other to finish.  Note that this is a "may" error.  To be 100%
       sure we catch this error we would have to lock the data
       structures but it is not necessary.  In the unlikely case that
       two threads are really caught in this situation they will
       deadlock.  It is the programmer's problem to figure this
       out.  */
    result = EDEADLK;
  /* Wait for the thread to finish.  If it is already locked something
     is wrong.  There can only be one waiter.  */
  else if (__builtin_expect (atomic_compare_and_exchange_bool_acq (&pd->joinid,
								   self,
								   NULL), 0))
    /* There is already somebody waiting for the thread.  */
    result = EINVAL;
  else
    /* Wait for the child.  */
    lll_wait_tid (pd->tid);


  /* Restore cancellation mode.  */
  CANCEL_RESET (oldtype);

  /* Remove the handler.  */
  pthread_cleanup_pop (0);


  if (__builtin_expect (result == 0, 1))
    {
      /* We mark the thread as terminated and as joined.  */
      pd->tid = -1;

      /* Store the return value if the caller is interested.  */
      if (thread_return != NULL)
	*thread_return = pd->result;


      /* Free the TCB.  */
      __free_tcb (pd);
    }

  return result;
}
