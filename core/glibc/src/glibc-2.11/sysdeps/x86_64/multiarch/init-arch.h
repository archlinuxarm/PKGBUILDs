/* This file is part of the GNU C Library.
   Copyright (C) 2008, 2009 Free Software Foundation, Inc.

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

#include <sys/param.h>

enum
  {
    COMMON_CPUID_INDEX_1 = 0,
    /* Keep the following line at the end.  */
    COMMON_CPUID_INDEX_MAX
  };

extern struct cpu_features
{
  enum
    {
      arch_kind_unknown = 0,
      arch_kind_intel,
      arch_kind_amd,
      arch_kind_other
    } kind;
  int max_cpuid;
  struct cpuid_registers
  {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
  } cpuid[COMMON_CPUID_INDEX_MAX];
  unsigned int family;
  unsigned int model;
} __cpu_features attribute_hidden;


extern void __init_cpu_features (void) attribute_hidden;
#define INIT_ARCH()\
  do							\
    if (__cpu_features.kind == arch_kind_unknown)	\
      __init_cpu_features ();				\
  while (0)

/* Used from outside libc.so to get access to the CPU features structure.  */
extern const struct cpu_features *__get_cpu_features (void)
     __attribute__ ((const));

#ifndef NOT_IN_libc
# define __get_cpu_features()	(&__cpu_features)
#endif

#define HAS_CPU_FEATURE(idx, reg, bit) \
  ((__get_cpu_features ()->cpuid[idx].reg & (1 << (bit))) != 0)

/* Following are the feature tests used throughout libc.  */

#define HAS_SSE2	HAS_CPU_FEATURE (COMMON_CPUID_INDEX_1, edx, 26)
#define HAS_POPCOUNT	HAS_CPU_FEATURE (COMMON_CPUID_INDEX_1, ecx, 23)
#define HAS_SSE4_2	HAS_CPU_FEATURE (COMMON_CPUID_INDEX_1, ecx, 20)
#define HAS_FMA		HAS_CPU_FEATURE (COMMON_CPUID_INDEX_1, ecx, 12)
