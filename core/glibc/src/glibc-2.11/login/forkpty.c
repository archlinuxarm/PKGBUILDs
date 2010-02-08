/* Copyright (C) 1998, 2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Zack Weinberg <zack@rabi.phys.columbia.edu>, 1998.

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

#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <utmp.h>
#include <pty.h>

int
forkpty (amaster, name, termp, winp)
     int *amaster;
     char *name;
     const struct termios *termp;
     const struct winsize *winp;
{
  int master, slave, pid;

  if (openpty (&master, &slave, name, termp, winp) == -1)
    return -1;

  switch (pid = fork ())
    {
    case -1:
      close (master);
      close (slave);
      return -1;
    case 0:
      /* Child.  */
      close (master);
      if (login_tty (slave))
	_exit (1);

      return 0;
    default:
      /* Parent.  */
      *amaster = master;
      close (slave);

      return pid;
    }
}
