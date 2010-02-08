/* Run-time dynamic linker data structures for loaded ELF shared objects.
   Copyright (C) 2001, 2002, 2003, 2006, 2009 Free Software Foundation, Inc.
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

#ifndef	_LDSODEFS_H

#include <kernel-features.h>

/* Get the real definitions.  */
#include_next <ldsodefs.h>

/* Now define our stuff.  */

/* We have the auxiliary vector.  */
#define HAVE_AUX_VECTOR

/* Used by static binaries to check the auxiliary vector.  */
extern void _dl_aux_init (ElfW(auxv_t) *av) internal_function;

/* Initialization which is normally done by the dynamic linker.  */
extern void _dl_non_dynamic_init (void) internal_function;

/* We can assume that the kernel always provides the AT_UID, AT_EUID,
   AT_GID, and AT_EGID values in the auxiliary vector from 2.4.0 or so on.  */
#if __ASSUME_AT_XID
# define HAVE_AUX_XID
#endif

/* We can assume that the kernel always provides the AT_SECURE value
   in the auxiliary vector from 2.5.74 or so on.  */
#if __ASSUME_AT_SECURE
# define HAVE_AUX_SECURE
#endif

/* Starting with one of the 2.4.0 pre-releases the Linux kernel passes
   up the page size information.  */
#if __ASSUME_AT_PAGESIZE
# define HAVE_AUX_PAGESIZE
#endif

/* Accept binaries which identify the binary as using Linux extensions.  */
#define VALID_ELF_HEADER(hdr,exp,size)	(memcmp (hdr, exp, size) == 0	\
					 || memcmp (hdr, expected2, size) == 0)
#define VALID_ELF_OSABI(osabi)		(osabi == ELFOSABI_SYSV \
					 || osabi == ELFOSABI_LINUX)
#define VALID_ELF_ABIVERSION(ver)	(ver == 0)
#define MORE_ELF_HEADER_DATA \
  static const unsigned char expected2[EI_PAD] =	\
  {							\
    [EI_MAG0] = ELFMAG0,				\
    [EI_MAG1] = ELFMAG1,				\
    [EI_MAG2] = ELFMAG2,				\
    [EI_MAG3] = ELFMAG3,				\
    [EI_CLASS] = ELFW(CLASS),				\
    [EI_DATA] = byteorder,				\
    [EI_VERSION] = EV_CURRENT,				\
    [EI_OSABI] = ELFOSABI_LINUX,			\
    [EI_ABIVERSION] = 0					\
  }

#endif /* ldsodefs.h */
