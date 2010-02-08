/* Copyright (C) 1991,92,97,2001,02 Free Software Foundation, Inc.
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

#include <stddef.h>		/* For NULL.  */
#include <time.h>
#include <sys/time.h>


/* Return the current time as a `time_t' and also put it in *T if T is
   not NULL.  Time is represented as seconds from Jan 1 00:00:00 1970.  */
time_t
time (t)
     time_t *t;
{
  struct timeval tv;
  time_t result;

  if (__gettimeofday (&tv, (struct timezone *) NULL))
    result = (time_t) -1;
  else
    result = (time_t) tv.tv_sec;

  if (t != NULL)
    *t = result;
  return result;
}
libc_hidden_def (time)
