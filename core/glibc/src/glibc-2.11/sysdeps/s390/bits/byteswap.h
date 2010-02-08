/* Macros to swap the order of bytes in integer values.  s390 version.
   Copyright (C) 2000, 2001, 2002, 2003, 2008 Free Software Foundation, Inc.
   Contributed by Martin Schwidefsky (schwidefsky@de.ibm.com).
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

#if !defined _BYTESWAP_H && !defined _NETINET_IN_H && !defined _ENDIAN_H
# error "Never use <bits/byteswap.h> directly; include <byteswap.h> instead."
#endif

#include <bits/wordsize.h>

#ifndef _BITS_BYTESWAP_H
#define _BITS_BYTESWAP_H 1

#define __bswap_constant_16(x) \
     ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))

/* Swap bytes in 16 bit value. */
#if defined __GNUC__ && __GNUC__ >= 2
# if __WORDSIZE == 64
#  define __bswap_16(x) \
     (__extension__							      \
      ({ unsigned short int __v, __x = (x);	                              \
	 if (__builtin_constant_p (x))					      \
	   __v = __bswap_constant_16 (__x);				      \
	 else {								      \
           unsigned short int __tmp = (unsigned short int) (__x);             \
           __asm__ __volatile__ (                                             \
              "lrvh %0,%1"                                                    \
              : "=&d" (__v) : "m" (__tmp) );                                  \
         }                                                                    \
	 __v; }))
# else
#  define __bswap_16(x) \
     (__extension__							      \
      ({ unsigned short int __v, __x = (x);	                              \
	 if (__builtin_constant_p (x))					      \
	   __v = __bswap_constant_16 (__x);				      \
	 else {								      \
           unsigned short int __tmp = (unsigned short int) (__x);             \
           __asm__ __volatile__ (                                             \
              "sr   %0,%0\n"                                                  \
              "la   1,%1\n"                                                   \
              "icm  %0,2,1(1)\n"                                              \
              "ic   %0,0(1)"                                                  \
              : "=&d" (__v) : "m" (__tmp) : "1");                             \
         }                                                                    \
	 __v; }))
# endif
#else
/* This is better than nothing.  */
static __inline unsigned short int
__bswap_16 (unsigned short int __bsx)
{
  return __bswap_constant_16 (__bsx);
}
#endif

/* Swap bytes in 32 bit value.  */
#define __bswap_constant_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#if defined __GNUC__ && __GNUC__ >= 2
# if __WORDSIZE == 64
#  define __bswap_32(x) \
     (__extension__							      \
      ({ unsigned int __v, __x = (x);				              \
	 if (__builtin_constant_p (x))					      \
	   __v = __bswap_constant_32 (__x);				      \
	 else {								      \
           unsigned int __tmp = (unsigned int) (__x);                         \
           __asm__ __volatile__ (                                             \
              "lrv   %0,%1"                                                   \
              : "=&d" (__v) : "m" (__tmp));                                   \
         }                                                                    \
	 __v; }))
# else
#  define __bswap_32(x) \
     (__extension__							      \
      ({ unsigned int __v, __x = (x);			                      \
	 if (__builtin_constant_p (x))					      \
	   __v = __bswap_constant_32 (__x);				      \
	 else {								      \
           unsigned int __tmp = (unsigned int) (__x);                         \
           __asm__ __volatile__ (                                             \
              "la    1,%1\n"                                                  \
              "icm   %0,8,3(1)\n"                                             \
              "icm   %0,4,2(1)\n"                                             \
              "icm   %0,2,1(1)\n"                                             \
              "ic    %0,0(1)"                                                 \
              : "=&d" (__v) : "m" (__tmp) : "1");                             \
         }                                                                    \
	 __v; }))
# endif
#else
static __inline unsigned int
__bswap_32 (unsigned int __bsx)
{
  return __bswap_constant_32 (__bsx);
}
#endif

/* Swap bytes in 64 bit value.  */
#define __bswap_constant_64(x) \
     ((((x)&0xff00000000000000) >> 56) | (((x)&0x00ff000000000000) >> 40) |  \
      (((x)&0x0000ff0000000000) >> 24) | (((x)&0x000000ff00000000) >>  8) |  \
      (((x)&0x00000000ff000000) <<  8) | (((x)&0x0000000000ff0000) << 24) |  \
      (((x)&0x000000000000ff00) << 40) | (((x)&0x00000000000000ff) << 56))

#if defined __GNUC__ && __GNUC__ >= 2
# if __WORDSIZE == 64
#  define __bswap_64(x) \
     (__extension__							      \
      ({ unsigned long __w, __x = (x);				              \
	 if (__builtin_constant_p (x))					      \
	   __w = __bswap_constant_64 (__x);				      \
	 else {								      \
           unsigned long __tmp = (unsigned long) (__x);                       \
           __asm__ __volatile__ (                                             \
              "lrvg  %0,%1"                                                   \
              : "=&d" (__w) : "m" (__tmp));                                   \
         }                                                                    \
	 __w; }))
# else
#  define __bswap_64(x) \
     __extension__					\
       ({ union { unsigned long long int __ll;		\
	          unsigned long int __l[2]; } __w, __r;	\
          __w.__ll = (x);				\
          __r.__l[0] = __bswap_32 (__w.__l[1]);		\
          __r.__l[1] = __bswap_32 (__w.__l[0]);		\
          __r.__ll; })
# endif
#else
static __inline unsigned long long int
__bswap_64 (unsigned long long int __bsx)
{
  return __bswap_constant_64 (__bsx);
}
#endif

#endif /* _BITS_BYTESWAP_H */
