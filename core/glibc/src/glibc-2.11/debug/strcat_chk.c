/* Copyright (C) 1991, 1997, 2003, 2004 Free Software Foundation, Inc.
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


/* Append SRC on the end of DEST.  */
char *
__strcat_chk (dest, src, destlen)
     char *dest;
     const char *src;
     size_t destlen;
{
  char *s1 = dest;
  const char *s2 = src;
  reg_char c;

  /* Find the end of the string.  */
  do
    {
      if (__builtin_expect (destlen-- == 0, 0))
	__chk_fail ();
      c = *s1++;
    }
  while (c != '\0');

  /* Make S1 point before the next character, so we can increment
     it while memory is read (wins on pipelined cpus).  */
  ++destlen;
  s1 -= 2;

  do
    {
      if (__builtin_expect (destlen-- == 0, 0))
	__chk_fail ();
      c = *s2++;
      *++s1 = c;
    }
  while (c != '\0');

  return dest;
}
