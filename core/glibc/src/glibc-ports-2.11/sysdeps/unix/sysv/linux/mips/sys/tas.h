/* Copyright (C) 2000, 2002, 2003, 2004, 2007, 2009
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Maciej W. Rozycki <macro@ds2.pg.gda.pl>, 2000.

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

#ifndef _SYS_TAS_H
#define _SYS_TAS_H 1

#include <features.h>
#include <sgidefs.h>

__BEGIN_DECLS

extern int _test_and_set (int *__p, int __v) __THROW;

#ifdef __USE_EXTERN_INLINES

# ifndef _EXTERN_INLINE
#  define _EXTERN_INLINE __extern_inline
# endif

_EXTERN_INLINE int
__NTH (_test_and_set (int *__p, int __v))
{
  int __r, __t;

  __asm__ __volatile__
    ("/* Inline test and set */\n"
     ".set	push\n\t"
#if _MIPS_SIM == _ABIO32
     ".set	mips2\n\t"
#endif
     "sync\n\t"
     "1:\n\t"
     "ll	%0,%3\n\t"
     "move	%1,%4\n\t"
     "beq	%0,%4,2f\n\t"
     "sc	%1,%2\n\t"
     "beqz	%1,1b\n"
     "sync\n\t"
     ".set	pop\n\t"
     "2:\n\t"
     "/* End test and set */"
     : "=&r" (__r), "=&r" (__t), "=m" (*__p)
     : "m" (*__p), "r" (__v)
     : "memory");

  return __r;
}

#endif /* __USE_EXTERN_INLINES */

__END_DECLS

#endif /* sys/tas.h */
