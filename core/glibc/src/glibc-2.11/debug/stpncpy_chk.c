/* Copyright (C) 1993,1995,1996,1997,2002,2005 Free Software Foundation, Inc.
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

/* This is almost copied from strncpy.c, written by Torbjorn Granlund.  */

#include <string.h>


/* Copy no more than N characters of SRC to DEST, returning the address of
   the terminating '\0' in DEST, if any, or else DEST + N.  */
char *
__stpncpy_chk (char *dest, const char *src, size_t n, size_t destlen)
{
  char c;
  char *s = dest;

  if (__builtin_expect (destlen < n, 0))
    __chk_fail ();

  if (n >= 4)
    {
      size_t n4 = n >> 2;

      for (;;)
	{
	  c = *src++;
	  *dest++ = c;
	  if (c == '\0')
	    break;
	  c = *src++;
	  *dest++ = c;
	  if (c == '\0')
	    break;
	  c = *src++;
	  *dest++ = c;
	  if (c == '\0')
	    break;
	  c = *src++;
	  *dest++ = c;
	  if (c == '\0')
	    break;
	  if (--n4 == 0)
	    goto last_chars;
	}
      n -= dest - s;
      goto zero_fill;
    }

 last_chars:
  n &= 3;
  if (n == 0)
    return dest;

  for (;;)
    {
      c = *src++;
      --n;
      *dest++ = c;
      if (c == '\0')
	break;
      if (n == 0)
	return dest;
    }

 zero_fill:
  while (n-- > 0)
    dest[n] = '\0';

  return dest - 1;
}
