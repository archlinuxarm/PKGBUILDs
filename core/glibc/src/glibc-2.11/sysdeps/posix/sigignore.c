/* Set the disposition of SIG to SIG_IGN.
   Copyright (C) 1998, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

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
#define __need_NULL
#include <stddef.h>
#include <signal.h>
#include <string.h>	/* For the real memset prototype.  */


int
sigignore (sig)
     int sig;
{
  struct sigaction act;

  act.sa_handler = SIG_IGN;
  if (__sigemptyset (&act.sa_mask) < 0)
    return -1;
  act.sa_flags = 0;

  return __sigaction (sig, &act, NULL);
}
