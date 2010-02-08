/* Copyright (C) 1993, 1996, 1997, 1998, 1999, 2001, 2002, 2003, 2006
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
   02111-1307 USA.

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.  */

#include "libioP.h"
#include <wchar.h>

wint_t
ungetwc (c, fp)
     wint_t c;
     _IO_FILE *fp;
{
  wint_t result;
  CHECK_FILE (fp, WEOF);
  _IO_acquire_lock (fp);
  _IO_fwide (fp, 1);
  if (c == WEOF)
    result = WEOF;
  else
    result = INTUSE(_IO_sputbackwc) (fp, c);
  _IO_release_lock (fp);
  return result;
}
