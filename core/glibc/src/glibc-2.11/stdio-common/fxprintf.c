/* Copyright (C) 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.org>.

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

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <libioP.h>


int
__fxprintf (FILE *fp, const char *fmt, ...)
{
  if (fp == NULL)
    fp = stderr;

  va_list ap;
  va_start (ap, fmt);

  int res;
  if (_IO_fwide (fp, 0) > 0)
    {
      size_t len = strlen (fmt) + 1;
      wchar_t wfmt[len];
      for (size_t i = 0; i < len; ++i)
	{
	  assert (isascii (fmt[i]));
	  wfmt[i] = fmt[i];
	}
      res = __vfwprintf (fp, wfmt, ap);
    }
  else
    res = INTUSE(_IO_vfprintf) (fp, fmt, ap);

  va_end (ap);

  return res;
}
