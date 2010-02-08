/* Copyright (C) 1991, 1995, 1996, 1997, 1998, 2002, 2003
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

#include "libioP.h"
#include "stdio.h"

#undef _IO_putc

int
_IO_putc (c, fp)
     int c;
     _IO_FILE *fp;
{
  int result;
  CHECK_FILE (fp, EOF);
  _IO_acquire_lock (fp);
  result = _IO_putc_unlocked (c, fp);
  _IO_release_lock (fp);
  return result;
}
INTDEF(_IO_putc)

#undef putc

#ifdef weak_alias
weak_alias (_IO_putc, putc)

#ifndef _IO_MTSAFE_IO
#undef putc_unlocked
weak_alias (_IO_putc, putc_unlocked)
#endif
#endif
