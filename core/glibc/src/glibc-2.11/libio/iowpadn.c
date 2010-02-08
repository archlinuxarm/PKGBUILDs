/* Copyright (C) 1993, 1997, 1999 Free Software Foundation, Inc.
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

#define PADSIZE 16
static wchar_t const blanks[PADSIZE] =
{
  L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' ',
  L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' '
};
static wchar_t const zeroes[PADSIZE] =
{
  L'0', L'0', L'0', L'0', L'0', L'0', L'0', L'0',
  L'0', L'0', L'0', L'0', L'0', L'0', L'0', L'0'
};

_IO_ssize_t
_IO_wpadn (fp, pad, count)
      _IO_FILE *fp;
      wint_t pad;
      _IO_ssize_t count;
{
  wchar_t padbuf[PADSIZE];
  const wchar_t *padptr;
  int i;
  _IO_size_t written = 0;
  _IO_size_t w;

  if (pad == L' ')
    padptr = blanks;
  else if (pad == L'0')
    padptr = zeroes;
  else
    {
      for (i = PADSIZE; --i >= 0; )
	padbuf[i] = pad;
      padptr = padbuf;
    }
  for (i = count; i >= PADSIZE; i -= PADSIZE)
    {
      w = _IO_sputn (fp, (char *) padptr, PADSIZE);
      written += w;
      if (w != PADSIZE)
	return written;
    }

  if (i > 0)
    {
      w = _IO_sputn (fp, (char *) padptr, i);
      written += w;
    }
  return written;
}
