/* Copyright (C) 1992, 1997 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <stdio.h>

int
main (void)
{
  int err = 0;
  int i, j;
  while (scanf ("%d %d\n", &i, &j) == 2)
    {
      div_t d = div (i, j);
      printf ("%d / %d = %d + %d/%d", i, j, d.quot, d.rem, j);
      if (i == d.quot * j + d.rem)
	fputs ("  OK\n", stdout);
      else
	{
	  fputs ("  FAILED\n", stdout);
	  err = 1;
	}
    }
  return err;
}
