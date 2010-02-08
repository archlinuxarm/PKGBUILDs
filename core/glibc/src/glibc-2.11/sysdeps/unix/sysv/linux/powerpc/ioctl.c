/* Copyright (C) 1998, 2002, 2003, 2004 Free Software Foundation, Inc.
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

#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sysdep.h>

/* The user-visible size of struct termios has changed.  Catch ioctl calls
   using the new-style struct termios, and translate them to old-style.  */

int
__ioctl (int fd, unsigned long int request, ...)
{
  void *arg;
  va_list ap;
  int result;

  va_start (ap, request);
  arg = va_arg (ap, void *);

  switch (request)
    {
    case TCGETS:
      result = __tcgetattr (fd, (struct termios *) arg);
      break;

    case TCSETS:
      result = tcsetattr (fd, TCSANOW, (struct termios *) arg);
      break;

    case TCSETSW:
      result = tcsetattr (fd, TCSADRAIN, (struct termios *) arg);
      break;

    case TCSETSF:
      result = tcsetattr (fd, TCSAFLUSH, (struct termios *) arg);
      break;

    default:
      result = INLINE_SYSCALL (ioctl, 3, fd, request, arg);
      break;
    }

  va_end (ap);

  return result;
}
weak_alias (__ioctl, ioctl)
