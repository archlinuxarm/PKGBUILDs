/* bzero -- set a block of memory to zero.  For Intel 80x86, x>=3.
   This file is part of the GNU C Library.
   Copyright (C) 1991,92,93,97,98,99, 05 Free Software Foundation, Inc.
   Contributed by Torbjorn Granlund (tege@sics.se).

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

#undef	bzero
#undef	__bzero

#ifdef	__GNUC__

void
__bzero (dstpp, len)
     void *dstpp;
     size_t len;
{
  /* N.B.: This code is almost verbatim from memset.c.  */
  int d0;
  unsigned long int dstp = (unsigned long int) dstpp;

  /* This explicit register allocation
     improves code very much indeed.  */
  register op_t x asm ("ax");

  x = 0;

  /* Clear the direction flag, so filling will move forward.  */
  asm volatile ("cld");

  /* This threshold value is optimal.  */
  if (len >= 12)
    {
      /* Adjust LEN for the bytes handled in the first loop.  */
      len -= (-dstp) % OPSIZ;

      /* There are at least some bytes to set.
	 No need to test for LEN == 0 in this alignment loop.  */

      /* Fill bytes until DSTP is aligned on a longword boundary.  */
      asm volatile ("rep\n"
		    "stosb" /* %0, %2, %3 */ :
		    "=D" (dstp), "=c" (d0) :
		    "0" (dstp), "1" ((-dstp) % OPSIZ), "a" (x) :
		    "memory");

      /* Fill longwords.  */
      asm volatile ("rep\n"
		    "stosl" /* %0, %2, %3 */ :
		    "=D" (dstp), "=c" (d0) :
		    "0" (dstp), "1" (len / OPSIZ), "a" (x) :
		    "memory");
      len %= OPSIZ;
    }

  /* Write the last few bytes.  */
  asm volatile ("rep\n"
		"stosb" /* %0, %2, %3 */ :
		"=D" (dstp), "=c" (d0) :
		"0" (dstp), "c" (len), "a" (x) :
		"memory");
}
weak_alias (__bzero, bzero)

#else
#include <string/bzero.c>
#endif
