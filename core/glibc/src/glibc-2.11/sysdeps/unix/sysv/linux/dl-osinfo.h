/* Operating system specific code for generic dynamic loader functions.  Linux.
   Copyright (C) 2000-2002,2004-2008, 2009 Free Software Foundation, Inc.
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

#include <kernel-features.h>
#include <dl-sysdep.h>
#include <fcntl.h>
#include <stdint.h>

#ifndef MIN
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifdef SHARED
/* This is the function used in the dynamic linker to print the fatal error
   message.  */
static void
__attribute__ ((__noreturn__))
dl_fatal (const char *str)
{
  _dl_dprintf (2, str);
  _exit (1);
}
#endif

#define DL_SYSDEP_OSCHECK(FATAL)					      \
  do {									      \
    /* Test whether the kernel is new enough.  This test is only performed    \
       if the library is not compiled to run on all kernels.  */	      \
									      \
    int version = _dl_discover_osversion ();				      \
    if (__builtin_expect (version >= 0, 1))				      \
      {									      \
	if (__builtin_expect (GLRO(dl_osversion) == 0, 1)		      \
	    || GLRO(dl_osversion) > version)				      \
	  GLRO(dl_osversion) = version;					      \
									      \
	/* Now we can test with the required version.  */		      \
	if (__LINUX_KERNEL_VERSION > 0 && version < __LINUX_KERNEL_VERSION)   \
	  /* Not sufficent.  */						      \
	  FATAL ("FATAL: kernel too old\n");				      \
      }									      \
    else if (__LINUX_KERNEL_VERSION > 0)				      \
      FATAL ("FATAL: cannot determine kernel version\n");		      \
  } while (0)

static inline uintptr_t __attribute__ ((always_inline))
_dl_setup_stack_chk_guard (void *dl_random)
{
  uintptr_t ret;
#ifndef __ASSUME_AT_RANDOM
  if (__builtin_expect (dl_random == NULL, 0))
    {
# ifdef ENABLE_STACKGUARD_RANDOMIZE
      int fd = __open ("/dev/urandom", O_RDONLY);
      if (fd >= 0)
	{
	  ssize_t reslen = __read (fd, &ret, sizeof (ret));
	  __close (fd);
	  if (reslen == (ssize_t) sizeof (ret))
	    return ret;
	}
# endif
      ret = 0;
      unsigned char *p = (unsigned char *) &ret;
      p[sizeof (ret) - 1] = 255;
      p[sizeof (ret) - 2] = '\n';
    }
  else
#endif
    /* We need in the moment only 8 bytes on 32-bit platforms and 16
       bytes on 64-bit platforms.  Therefore we can use the data
       directly and not use the kernel-provided data to seed a PRNG.  */
    memcpy (&ret, dl_random, sizeof (ret));
  return ret;
}

static inline uintptr_t __attribute__ ((always_inline))
_dl_setup_pointer_guard (void *dl_random, uintptr_t stack_chk_guard)
{
  uintptr_t ret;
#ifndef __ASSUME_AT_RANDOM
  if (dl_random == NULL)
    {
      ret = stack_chk_guard;
# ifndef HP_TIMING_NONAVAIL
      hp_timing_t now;
      HP_TIMING_NOW (now);
      ret ^= now;
# endif
    }
  else
#endif
    memcpy (&ret, (char *) dl_random + sizeof (ret), sizeof (ret));
  return ret;
}
