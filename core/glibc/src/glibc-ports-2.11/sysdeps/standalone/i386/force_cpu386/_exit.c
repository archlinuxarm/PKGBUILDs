/* Copyright (C) 1991, 1997, 1999, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Joel Sherrill (jsherril@redstone-emh2.army.mil),
     On-Line Applications Research Corporation.

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

/* This returns control to FORCEbug. */

void Bsp_cleanup (void);

/* The function `_exit' should take a status argument and simply
   terminate program execution, using the low-order 8 bits of the
   given integer as status.  */

__NORETURN void
_exit (status)
     int status;
{
  /* status is ignored */
  Bsp_cleanup();
}
weak_alias (_exit, _Exit)

#ifdef	 HAVE_GNU_LD

#include <gnu-stabs.h>

stub_warning(_exit);

#endif	/* GNU stabs.  */
