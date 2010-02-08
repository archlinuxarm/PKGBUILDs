/* Copyright (C) 2003, 2004, 2006, 2007 Free Software Foundation, Inc.
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

#include <alloca.h>
#include <errno.h>
#include <pthreadP.h>
#include <sysdep.h>
#include <sys/types.h>
#include <shlib-compat.h>


size_t __kernel_cpumask_size attribute_hidden;


/* Determine the current affinity.  As a side affect we learn
   about the size of the cpumask_t in the kernel.  */
int
__determine_cpumask_size (pid_t tid)
{
  INTERNAL_SYSCALL_DECL (err);
  int res;

  size_t psize = 128;
  void *p = alloca (psize);

  while (res = INTERNAL_SYSCALL (sched_getaffinity, err, 3, tid, psize, p),
	 INTERNAL_SYSCALL_ERROR_P (res, err)
	 && INTERNAL_SYSCALL_ERRNO (res, err) == EINVAL)
    p = extend_alloca (p, psize, 2 * psize);

  if (res == 0 || INTERNAL_SYSCALL_ERROR_P (res, err))
    return INTERNAL_SYSCALL_ERRNO (res, err);

  __kernel_cpumask_size = res;

  return 0;
}


int
__pthread_setaffinity_new (pthread_t th, size_t cpusetsize,
			   const cpu_set_t *cpuset)
{
  const struct pthread *pd = (const struct pthread *) th;

  INTERNAL_SYSCALL_DECL (err);
  int res;

  if (__builtin_expect (__kernel_cpumask_size == 0, 0))
    {
      res = __determine_cpumask_size (pd->tid);
      if (res != 0)
	return res;
    }

  /* We now know the size of the kernel cpumask_t.  Make sure the user
     does not request to set a bit beyond that.  */
  for (size_t cnt = __kernel_cpumask_size; cnt < cpusetsize; ++cnt)
    if (((char *) cpuset)[cnt] != '\0')
      /* Found a nonzero byte.  This means the user request cannot be
	 fulfilled.  */
      return EINVAL;

  res = INTERNAL_SYSCALL (sched_setaffinity, err, 3, pd->tid, cpusetsize,
			  cpuset);

#ifdef RESET_VGETCPU_CACHE
  if (!INTERNAL_SYSCALL_ERROR_P (res, err))
    RESET_VGETCPU_CACHE ();
#endif

  return (INTERNAL_SYSCALL_ERROR_P (res, err)
	  ? INTERNAL_SYSCALL_ERRNO (res, err)
	  : 0);
}
versioned_symbol (libpthread, __pthread_setaffinity_new,
		  pthread_setaffinity_np, GLIBC_2_3_4);


#if SHLIB_COMPAT (libpthread, GLIBC_2_3_3, GLIBC_2_3_4)
int
__pthread_setaffinity_old (pthread_t th, cpu_set_t *cpuset)
{
  /* The old interface by default assumed a 1024 processor bitmap.  */
  return __pthread_setaffinity_new (th, 128, cpuset);
}
compat_symbol (libpthread, __pthread_setaffinity_old, pthread_setaffinity_np,
	       GLIBC_2_3_3);
#endif
