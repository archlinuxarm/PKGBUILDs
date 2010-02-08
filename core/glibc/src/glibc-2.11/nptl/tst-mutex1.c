/* Copyright (C) 2002, 2006 Free Software Foundation, Inc.
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

#include <pthread.h>
#include <stdio.h>
#include <errno.h>


#ifndef ATTR
# define ATTR NULL
#endif


static int
do_test (void)
{
  pthread_mutex_t m;

  int e = pthread_mutex_init (&m, ATTR);
  if (ATTR != NULL && e == ENOTSUP)
    {
      puts ("cannot support selected type of mutexes");
      return 0;
    }
  else if (e != 0)
    {
      puts ("mutex_init failed");
      return 1;
    }

  if (ATTR != NULL && pthread_mutexattr_destroy (ATTR) != 0)
    {
      puts ("mutexattr_destroy failed");
      return 1;
    }

  if (pthread_mutex_lock (&m) != 0)
    {
      puts ("mutex_lock failed");
      return 1;
    }

  if (pthread_mutex_unlock (&m) != 0)
    {
      puts ("mutex_unlock failed");
      return 1;
    }

  if (pthread_mutex_destroy (&m) != 0)
    {
      puts ("mutex_destroy failed");
      return 1;
    }

  return 0;
}

#ifndef TEST_FUNCTION
# define TEST_FUNCTION do_test ()
#endif
#include "../test-skeleton.c"
