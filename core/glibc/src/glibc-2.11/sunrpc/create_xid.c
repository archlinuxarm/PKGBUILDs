/* Copyright (c) 1998, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Thorsten Kukuk <kukuk@vt.uni-paderborn.de>, 1998.

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

#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <bits/libc-lock.h>
#include <rpc/rpc.h>

/* The RPC code is not threadsafe, but new code should be threadsafe. */

__libc_lock_define_initialized (static, createxid_lock)

static int is_initialized;
static struct drand48_data __rpc_lrand48_data;

unsigned long
_create_xid (void)
{
  long int res;

  __libc_lock_lock (createxid_lock);

  if (!is_initialized)
    {
      struct timeval now;

      __gettimeofday (&now, (struct timezone *) 0);
      __srand48_r (now.tv_sec ^ now.tv_usec, &__rpc_lrand48_data);
      is_initialized = 1;
    }

  lrand48_r (&__rpc_lrand48_data, &res);

  __libc_lock_unlock (createxid_lock);

  return res;
}
