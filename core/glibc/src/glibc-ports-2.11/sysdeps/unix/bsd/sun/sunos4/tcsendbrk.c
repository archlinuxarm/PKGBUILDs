/* Copyright (C) 1993, 1997 Free Software Foundation, Inc.
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

#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/termio.h>		/* Sun header file.  */

/* Send zero bits on FD.  */
int
tcsendbreak (fd, duration)
     int fd;
     int duration;
{
  /* According to SunOS 4.1's termios(4), you can't specify a duration.  */
  return __ioctl (fd, TCSBRK, 0);
}
