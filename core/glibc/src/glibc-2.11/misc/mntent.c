/* Utilities for reading/writing fstab, mtab, etc.
   Copyright (C) 1995, 1996, 1997, 2000, 2002 Free Software Foundation, Inc.
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

#include <mntent.h>
#include <stdlib.h>
#include <bits/libc-lock.h>

/* We don't want to allocate the static buffer all the time since it
   is not always used (in fact, rather infrequently).  Accept the
   extra cost of a `malloc'.  */
libc_freeres_ptr (static char *getmntent_buffer);

/* This is the size of the buffer.  This is really big.  */
#define BUFFER_SIZE	4096


static void
allocate (void)
{
  getmntent_buffer = (char *) malloc (BUFFER_SIZE);
}


struct mntent *
getmntent (FILE *stream)
{
  static struct mntent m;
  __libc_once_define (static, once);
  __libc_once (once, allocate);

  if (getmntent_buffer == NULL)
    /* If no core is available we don't have a chance to run the
       program successfully and so returning NULL is an acceptable
       result.  */
    return NULL;

  return __getmntent_r (stream, &m, getmntent_buffer, BUFFER_SIZE);
}
