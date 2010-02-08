/* Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2003.

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
#include <stdlib.h>


extern int val;


static void
prepare (void)
{
  ++val;
}

static void
parent (void)
{
  val *= 4;
}

static void
child (void)
{
  val *= 8;
}

static void
__attribute__ ((constructor))
init (void)
{
  extern void *__dso_handle;
  printf ("dsohandle = %p\n", __dso_handle);

  if (pthread_atfork (prepare, parent, child) != 0)
    {
      puts ("init: atfork failed");
      exit (1);
    }
}
