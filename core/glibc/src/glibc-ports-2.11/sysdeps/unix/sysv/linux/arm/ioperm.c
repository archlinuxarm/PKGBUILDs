/* Copyright (C) 1998, 1999, 2003, 2005, 2008 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Phil Blundell, based on the Alpha version by
   David Mosberger.

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

/* I/O port access on the ARM is something of a fiction.  What we do is to
   map an appropriate area of /dev/mem into user space so that a program
   can blast away at the hardware in such a way as to generate I/O cycles
   on the bus.  To insulate user code from dependencies on particular
   hardware we don't allow calls to inb() and friends to be inlined, but
   force them to come through code in here every time.  Performance-critical
   registers tend to be memory mapped these days so this should be no big
   problem.  */

/* Once upon a time this file used mprotect to enable and disable
   access to particular areas of I/O space.  Unfortunately the
   mprotect syscall also has the side effect of enabling caching for
   the area affected (this is a kernel limitation).  So we now just
   enable all the ports all of the time.  */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/mman.h>

#include <linux/version.h>
#include <sys/sysctl.h>

#define PATH_ARM_SYSTYPE	"/etc/arm_systype"
#define PATH_CPUINFO		"/proc/cpuinfo"

#define MAX_PORT	0x10000

static struct {
  unsigned long int	base;
  unsigned long int	io_base;
  unsigned int		shift;
  unsigned int		initdone;	/* since all the above could be 0 */
} io;

#define IO_BASE_FOOTBRIDGE	0x7c000000
#define IO_SHIFT_FOOTBRIDGE	0

static struct platform {
  const char		*name;
  unsigned long int	io_base;
  unsigned int		shift;
} platform[] = {
  /* All currently supported platforms are in fact the same. :-)  */
  {"Chalice-CATS",	IO_BASE_FOOTBRIDGE,	IO_SHIFT_FOOTBRIDGE},
  {"DEC-EBSA285",	IO_BASE_FOOTBRIDGE,	IO_SHIFT_FOOTBRIDGE},
  {"Corel-NetWinder",	IO_BASE_FOOTBRIDGE,	IO_SHIFT_FOOTBRIDGE},
  {"Rebel-NetWinder",	IO_BASE_FOOTBRIDGE,	IO_SHIFT_FOOTBRIDGE},
};

#define IO_ADDR(port)	(io.base + ((port) << io.shift))

/*
 * Initialize I/O system.  There are several ways to get the information
 * we need.  Each is tried in turn until one succeeds.
 *
 * 1. Sysctl (CTL_BUS, CTL_BUS_ISA, ISA_*).  This is the preferred method
 *    but not all kernels support it.
 *
 * 2. Read the value (not the contents) of symlink PATH_ARM_SYSTYPE.
 *    - If it matches one of the entries in the table above, use the
 *      corresponding values.
 *    - If it begins with a number, assume this is a previously
 *      unsupported system and the values encode, in order,
 *      "<io_base>,<port_shift>".
 *
 * 3. Lookup the "system type" field in /proc/cpuinfo.  Again, if it
 *    matches an entry in the platform[] table, use the corresponding
 *    values.
 */

/* The Linux kernel headers renamed this constant between 2.5.26 and
   2.5.27.  It was backported to 2.4 between 2.4.22 and 2.4.23.  */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,23)
# define BUS_ISA CTL_BUS_ISA
#endif

static int
init_iosys (void)
{
  char systype[256];
  int i, n;
  static int iobase_name[] = { CTL_BUS, BUS_ISA, BUS_ISA_PORT_BASE };
  static int ioshift_name[] = { CTL_BUS, BUS_ISA, BUS_ISA_PORT_SHIFT };
  size_t len = sizeof(io.base);

  if (! __sysctl (iobase_name, 3, &io.io_base, &len, NULL, 0)
      && ! __sysctl (ioshift_name, 3, &io.shift, &len, NULL, 0))
    {
      io.initdone = 1;
      return 0;
    }

  n = __readlink (PATH_ARM_SYSTYPE, systype, sizeof (systype) - 1);
  if (n > 0)
    {
      systype[n] = '\0';
      if (isdigit (systype[0]))
	{
	  if (sscanf (systype, "%li,%i", &io.io_base, &io.shift) == 2)
	    {
	      io.initdone = 1;
	      return 0;
	    }
	  /* else we're likely going to fail with the system match below */
	}
    }
  else
    {
      FILE * fp;

      fp = fopen (PATH_CPUINFO, "r");
      if (! fp)
	return -1;
      while ((n = fscanf (fp, "Hardware\t: %256[^\n]\n", systype))
	     != EOF)
	{
	  if (n == 1)
	    break;
	  else
	    fgets_unlocked (systype, 256, fp);
	}
      fclose (fp);

      if (n == EOF)
	{
	  /* this can happen if the format of /proc/cpuinfo changes...  */
	  fprintf (stderr,
		   "ioperm: Unable to determine system type.\n"
		   "\t(May need " PATH_ARM_SYSTYPE " symlink?)\n");
	  __set_errno (ENODEV);
	  return -1;
	}
    }

  /* translate systype name into i/o system: */
  for (i = 0; i < sizeof (platform) / sizeof (platform[0]); ++i)
    {
      if (strcmp (platform[i].name, systype) == 0)
	{
	  io.shift = platform[i].shift;
	  io.io_base = platform[i].io_base;
	  io.initdone = 1;
	  return 0;
	}
    }

  /* systype is not a known platform name... */
  __set_errno (ENODEV);
  return -1;
}

int
_ioperm (unsigned long int from, unsigned long int num, int turn_on)
{
  if (! io.initdone && init_iosys () < 0)
    return -1;

  /* this test isn't as silly as it may look like; consider overflows! */
  if (from >= MAX_PORT || from + num > MAX_PORT)
    {
      __set_errno (EINVAL);
      return -1;
    }

  if (turn_on)
    {
      if (! io.base)
	{
	  int fd;

	  fd = __open ("/dev/mem", O_RDWR);
	  if (fd < 0)
	    return -1;

	  io.base =
	    (unsigned long int) __mmap (0, MAX_PORT << io.shift,
					PROT_READ | PROT_WRITE,
					MAP_SHARED, fd, io.io_base);
	  __close (fd);
	  if ((long) io.base == -1)
	    return -1;
	}
    }

  return 0;
}


int
_iopl (unsigned int level)
{
    if (level > 3)
      {
	__set_errno (EINVAL);
	return -1;
      }
    if (level)
      {
	return _ioperm (0, MAX_PORT, 1);
      }
    return 0;
}


void
_outb (unsigned char b, unsigned long int port)
{
  *((volatile unsigned char *)(IO_ADDR (port))) = b;
}


void
_outw (unsigned short b, unsigned long int port)
{
  *((volatile unsigned short *)(IO_ADDR (port))) = b;
}


void
_outl (unsigned int b, unsigned long int port)
{
  *((volatile unsigned long *)(IO_ADDR (port))) = b;
}


unsigned int
_inb (unsigned long int port)
{
  return *((volatile unsigned char *)(IO_ADDR (port)));
}


unsigned int
_inw (unsigned long int port)
{
  return *((volatile unsigned short *)(IO_ADDR (port)));
}


unsigned int
_inl (unsigned long int port)
{
  return *((volatile unsigned long *)(IO_ADDR (port)));
}

weak_alias (_ioperm, ioperm);
weak_alias (_iopl, iopl);
weak_alias (_inb, inb);
weak_alias (_inw, inw);
weak_alias (_inl, inl);
weak_alias (_outb, outb);
weak_alias (_outw, outw);
weak_alias (_outl, outl);
