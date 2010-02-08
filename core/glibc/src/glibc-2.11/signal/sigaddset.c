/* Copyright (C) 1991, 1996, 2003 Free Software Foundation, Inc.
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

#include "sigsetops.h"

/* Add SIGNO to SET.  */
int
sigaddset (set, signo)
     sigset_t *set;
     int signo;
{
  if (set == NULL || signo <= 0 || signo >= NSIG)
    {
      __set_errno (EINVAL);
      return -1;
    }

  return __sigaddset (set, signo);
}
libc_hidden_def (sigaddset)
