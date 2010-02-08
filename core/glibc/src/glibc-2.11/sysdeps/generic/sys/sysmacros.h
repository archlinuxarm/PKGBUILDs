/* Definitions of macros to access `dev_t' values.
   Copyright (C) 1996, 1997, 1999 Free Software Foundation, Inc.
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

#ifndef _SYS_SYSMACROS_H
#define _SYS_SYSMACROS_H	1

/* For compatibility we provide alternative names.

   The problem here is that compilers other than GCC probably don't
   have the `long long' type and so `dev_t' is actually an array.  */
#define major(dev) ((int)(((unsigned int) (dev) >> 8) & 0xff))
#define minor(dev) ((int)((dev) & 0xff))
#define makedev(major, minor) (((major) << 8) | (minor))

#endif /* sys/sysmacros.h */
