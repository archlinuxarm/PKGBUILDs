/* Copyright (C) 2003, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2003.

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

#include <assert.h>
#include <errno.h>
#include <pthreadP.h>
#include <string.h>
#include <sysdep.h>
#include <sys/types.h>
#include <shlib-compat.h>


int
__pthread_attr_getaffinity_new (const pthread_attr_t *attr, size_t cpusetsize,
				cpu_set_t *cpuset)
{
  const struct pthread_attr *iattr;

  assert (sizeof (*attr) >= sizeof (struct pthread_attr));
  iattr = (const struct pthread_attr *) attr;

  if (iattr->cpuset != NULL)
    {
      /* Check whether there are any bits set beyond the limits
	 the user requested.  */
      for (size_t cnt = cpusetsize; cnt < iattr->cpusetsize; ++cnt)
	if (((char *) iattr->cpuset)[cnt] != 0)
	  return EINVAL;

      void *p = mempcpy (cpuset, iattr->cpuset, iattr->cpusetsize);
      if (cpusetsize > iattr->cpusetsize)
	memset (p, '\0', cpusetsize - iattr->cpusetsize);
    }
  else
    /* We have no information.  */
    memset (cpuset, -1, cpusetsize);

  return 0;
}
versioned_symbol (libpthread, __pthread_attr_getaffinity_new,
		  pthread_attr_getaffinity_np, GLIBC_2_3_4);


#if SHLIB_COMPAT (libpthread, GLIBC_2_3_3, GLIBC_2_3_4)
int
__pthread_attr_getaffinity_old (const pthread_attr_t *attr, cpu_set_t *cpuset)
{
  /* The old interface by default assumed a 1024 processor bitmap.  */
  return __pthread_attr_getaffinity_new (attr, 128, cpuset);
}
compat_symbol (libpthread, __pthread_attr_getaffinity_old,
	       pthread_attr_getaffinity_np, GLIBC_2_3_3);
#endif
