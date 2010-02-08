/* `struct termios' speed frobnication functions.  Linux version.
   Copyright (C) 1991,1992,1993,1995,1996,1997,1998,2000,2002,2003
	Free Software Foundation, Inc.
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

#include <stddef.h>
#include <errno.h>
#include <termios.h>


/* This is a gross hack around a kernel bug.  If the cfsetispeed functions
   is called with the SPEED argument set to zero this means use the same
   speed as for output.  But we don't have independent input and output
   speeds and therefore cannot record this.

   We use an unused bit in the `c_iflag' field to keep track of this
   use of `cfsetispeed'.  The value here must correspond to the one used
   in `tcsetattr.c'.  */
#define IBAUD0	020000000000


/* Return the output baud rate stored in *TERMIOS_P.  */
speed_t
cfgetospeed (termios_p)
     const struct termios *termios_p;
{
  return termios_p->c_cflag & (CBAUD | CBAUDEX);
}

/* Return the input baud rate stored in *TERMIOS_P.
   Although for Linux there is no difference between input and output
   speed, the numerical 0 is a special case for the input baud rate. It
   should set the input baud rate to the output baud rate. */
speed_t
cfgetispeed (termios_p)
     const struct termios *termios_p;
{
  return ((termios_p->c_iflag & IBAUD0)
	  ? 0 : termios_p->c_cflag & (CBAUD | CBAUDEX));
}

/* Set the output baud rate stored in *TERMIOS_P to SPEED.  */
int
cfsetospeed  (termios_p, speed)
     struct termios *termios_p;
     speed_t speed;
{
  if ((speed & ~CBAUD) != 0
      && (speed < B57600 || speed > __MAX_BAUD))
    {
      __set_errno (EINVAL);
      return -1;
    }

#ifdef _HAVE_STRUCT_TERMIOS_C_OSPEED
  termios_p->c_ospeed = speed;
#endif
  termios_p->c_cflag &= ~(CBAUD | CBAUDEX);
  termios_p->c_cflag |= speed;

  return 0;
}
libc_hidden_def (cfsetospeed)


/* Set the input baud rate stored in *TERMIOS_P to SPEED.
   Although for Linux there is no difference between input and output
   speed, the numerical 0 is a special case for the input baud rate.  It
   should set the input baud rate to the output baud rate.  */
int
cfsetispeed (termios_p, speed)
     struct termios *termios_p;
     speed_t speed;
{
  if ((speed & ~CBAUD) != 0
      && (speed < B57600 || speed > __MAX_BAUD))
    {
      __set_errno (EINVAL);
      return -1;
    }

#ifdef _HAVE_STRUCT_TERMIOS_C_ISPEED
  termios_p->c_ispeed = speed;
#endif
  if (speed == 0)
    termios_p->c_iflag |= IBAUD0;
  else
    {
      termios_p->c_iflag &= ~IBAUD0;
      termios_p->c_cflag &= ~(CBAUD | CBAUDEX);
      termios_p->c_cflag |= speed;
    }

  return 0;
}
libc_hidden_def (cfsetispeed)
