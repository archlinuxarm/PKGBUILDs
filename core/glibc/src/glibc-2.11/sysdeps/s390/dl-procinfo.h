/* s390 version of processor capability information handling macros.
   Copyright (C) 2006, 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Martin Schwidefsky <schwidefsky@de.ibm.com>, 2006.

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

#ifndef _DL_PROCINFO_H
#define _DL_PROCINFO_H	1
#include <ldsodefs.h>

#define _DL_HWCAP_COUNT 10

#define _DL_PLATFORMS_COUNT	5

/* The kernel provides up to 32 capability bits with elf_hwcap.  */
#define _DL_FIRST_PLATFORM	32
/* Mask to filter out platforms.  */
#define _DL_HWCAP_PLATFORM	(((1ULL << _DL_PLATFORMS_COUNT) - 1) \
				 << _DL_FIRST_PLATFORM)

/* Hardware capablity bit numbers are derived directly from the
   facility indications as stored by the "store facility list" (STFL)
   instruction.  */

enum
{
  HWCAP_S390_ESAN3 = 1 << 0,
  HWCAP_S390_ZARCH = 1 << 1,
  HWCAP_S390_STFLE = 1 << 2,
  HWCAP_S390_MSA = 1 << 3,
  HWCAP_S390_LDISP = 1 << 4,
  HWCAP_S390_EIMM = 1 << 5,
  HWCAP_S390_DFP = 1 << 6,
  HWCAP_S390_HPAGE = 1 << 7,
  HWCAP_S390_ETF3EH = 1 << 8,
  HWCAP_S390_HIGH_GPRS = 1 << 9,
};

#define HWCAP_IMPORTANT (HWCAP_S390_ZARCH | HWCAP_S390_LDISP \
			  | HWCAP_S390_EIMM | HWCAP_S390_DFP)

/* We cannot provide a general printing function.  */
#define _dl_procinfo(word) -1

static inline const char *
__attribute__ ((unused))
_dl_hwcap_string (int idx)
{
  return GLRO(dl_s390_cap_flags)[idx];
};

static inline const char *
__attribute__ ((unused))
_dl_platform_string (int idx)
{
  return GLRO(dl_s390_platforms)[idx - _DL_FIRST_PLATFORM];
};

static inline int
__attribute__ ((unused, always_inline))
_dl_string_hwcap (const char *str)
{
  int i;

  for (i = 0; i < _DL_HWCAP_COUNT; i++)
    {
      if (strcmp (str, GLRO(dl_s390_cap_flags)[i]) == 0)
	return i;
    }
  return -1;
};

static inline int
__attribute__ ((unused, always_inline))
_dl_string_platform (const char *str)
{
  int i;

  if (str != NULL)
    for (i = 0; i < _DL_PLATFORMS_COUNT; ++i)
      {
	if (strcmp (str, GLRO(dl_s390_platforms)[i]) == 0)
	  return _DL_FIRST_PLATFORM + i;
      }
  return -1;
};

#endif /* dl-procinfo.h */
