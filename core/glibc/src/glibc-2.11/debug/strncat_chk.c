/* Copyright (C) 1991, 1997, 2004 Free Software Foundation, Inc.
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

#include <string.h>

#include <memcopy.h>


char *
__strncat_chk (s1, s2, n, s1len)
     char *s1;
     const char *s2;
     size_t n;
     size_t s1len;
{
  reg_char c;
  char *s = s1;

  /* Find the end of S1.  */
  do
    {
      if (__builtin_expect (s1len-- == 0, 0))
	__chk_fail ();
      c = *s1++;
    }
  while (c != '\0');

  /* Make S1 point before next character, so we can increment
     it while memory is read (wins on pipelined cpus).  */
  ++s1len;
  s1 -= 2;

  if (n >= 4)
    {
      size_t n4 = n >> 2;
      do
	{
	  if (__builtin_expect (s1len-- == 0, 0))
	    __chk_fail ();
	  c = *s2++;
	  *++s1 = c;
	  if (c == '\0')
	    return s;
	  if (__builtin_expect (s1len-- == 0, 0))
	    __chk_fail ();
	  c = *s2++;
	  *++s1 = c;
	  if (c == '\0')
	    return s;
	  if (__builtin_expect (s1len-- == 0, 0))
	    __chk_fail ();
	  c = *s2++;
	  *++s1 = c;
	  if (c == '\0')
	    return s;
	  if (__builtin_expect (s1len-- == 0, 0))
	    __chk_fail ();
	  c = *s2++;
	  *++s1 = c;
	  if (c == '\0')
	    return s;
	} while (--n4 > 0);
      n &= 3;
    }

  while (n > 0)
    {
      if (__builtin_expect (s1len-- == 0, 0))
	__chk_fail ();
      c = *s2++;
      *++s1 = c;
      if (c == '\0')
	return s;
      n--;
    }

  if (c != '\0')
    {
      if (__builtin_expect (s1len-- == 0, 0))
	__chk_fail ();
      *++s1 = '\0';
    }

  return s;
}
