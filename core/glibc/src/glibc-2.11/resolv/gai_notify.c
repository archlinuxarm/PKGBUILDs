/* Copyright (C) 2001, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2001.

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

#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <gai_misc.h>


struct notify_func
  {
    void (*func) (sigval_t);
    sigval_t value;
  };

static void *
notify_func_wrapper (void *arg)
{
  gai_start_notify_thread ();
  struct notify_func *const n = arg;
  void (*func) (sigval_t) = n->func;
  sigval_t value = n->value;
  free (n);
  (*func) (value);
  return NULL;
}


int
internal_function
__gai_notify_only (struct sigevent *sigev, pid_t caller_pid)
{
  int result = 0;

  /* Send the signal to notify about finished processing of the request.  */
  if (sigev->sigev_notify == SIGEV_THREAD)
    {
      /* We have to start a thread.  */
      pthread_t tid;
      pthread_attr_t attr, *pattr;

      pattr = (pthread_attr_t *) sigev->sigev_notify_attributes;
      if (pattr == NULL)
	{
	  pthread_attr_init (&attr);
	  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	  pattr = &attr;
	}

      /* SIGEV may be freed as soon as we return, so we cannot let the
	 notification thread use that pointer.  Even though a sigval_t is
	 only one word and the same size as a void *, we cannot just pass
	 the value through pthread_create as the argument and have the new
	 thread run the user's function directly, because on some machines
	 the calling convention for a union like sigval_t is different from
	 that for a pointer type like void *.  */
      struct notify_func *nf = malloc (sizeof *nf);
      if (nf == NULL)
	result = -1;
      else
	{
	  nf->func = sigev->sigev_notify_function;
	  nf->value = sigev->sigev_value;
	  if (pthread_create (&tid, pattr, notify_func_wrapper, nf) < 0)
	    {
	      free (nf);
	      result = -1;
	    }
	}
    }
  else if (sigev->sigev_notify == SIGEV_SIGNAL)
    /* We have to send a signal.  */
    if (__gai_sigqueue (sigev->sigev_signo, sigev->sigev_value, caller_pid)
	< 0)
      result = -1;

  return result;
}


void
internal_function
__gai_notify (struct requestlist *req)
{
  struct waitlist *waitlist;

  /* Now also notify possibly waiting threads.  */
  waitlist = req->waiting;
  while (waitlist != NULL)
    {
      struct waitlist *next = waitlist->next;

      if (waitlist->sigevp == NULL)
	{
#ifdef DONT_NEED_GAI_MISC_COND
	  GAI_MISC_NOTIFY (waitlist);
#else
	  /* Decrement the counter.  */
	  --*waitlist->counterp;

	  pthread_cond_signal (waitlist->cond);
#endif
	}
      else
	/* This is part of a asynchronous `getaddrinfo_a' operation.  If
	   this request is the last one, send the signal.  */
	if (--*waitlist->counterp == 0)
	  {
	    __gai_notify_only (waitlist->sigevp, waitlist->caller_pid);
	    /* This is tricky.  See getaddrinfo_a.c for the reason why
	       this works.  */
	    free ((void *) waitlist->counterp);
	  }

      waitlist = next;
    }
}
