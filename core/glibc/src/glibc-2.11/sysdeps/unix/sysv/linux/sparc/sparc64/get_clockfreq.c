/* Get frequency of the system processor.  sparc64 version.
   Copyright (C) 2001 Free Software Foundation, Inc.
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

#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <libc-internal.h>
#include <asm/openpromio.h>

static hp_timing_t
__get_clockfreq_via_cpuinfo (void)
{
  hp_timing_t result;
  int fd;

  result = 0;

  fd = open ("/proc/cpuinfo", O_RDONLY);
  if (fd != -1)
    {
      char buf[8192];
      ssize_t n;

      n = read (fd, buf, sizeof buf);
      if (n > 0)
	{
	  char *mhz = memmem (buf, n, "Cpu0ClkTck", 7);

	  if (mhz != NULL)
	    {
	      char *endp = buf + n;

	      /* Search for the beginning of the string.  */
	      while (mhz < endp
		     && (*mhz < '0' || *mhz > '9')
		     && (*mhz < 'a' || *mhz > 'f')
		     && *mhz != '\n')
		++mhz;

	      while (mhz < endp && *mhz != '\n')
		{
		  if ((*mhz >= '0' && *mhz <= '9') ||
		      (*mhz >= 'a' && *mhz <= 'f'))
		    {
		      result <<= 4;
		      if (*mhz >= '0' && *mhz <= '9')
			result += *mhz - '0';
		      else
			result += (*mhz - 'a') + 10;
		    }
		  ++mhz;
		}
	    }
	}

      close (fd);
    }

  return result;
}

static hp_timing_t
__get_clockfreq_via_proc_openprom (void)
{
  hp_timing_t result;
  int obp_fd;

  result = 0;

  obp_fd = open ("/proc/openprom", O_RDONLY);
  if (obp_fd != -1)
    {
      unsigned long int buf[4096 / sizeof (unsigned long int)];
      struct dirent *dirp = (struct dirent *) buf;
      off_t dbase = (off_t) 0;
      ssize_t len;

      while ((len = getdirentries (obp_fd, (char *) dirp,
				   sizeof (buf), &dbase)) > 0)
	{
	  struct dirent *this_dirp = dirp;

	  while (len > 0)
	    {
	      char node[strlen ("/proc/openprom/")
			+ _D_ALLOC_NAMLEN (this_dirp)
			+ strlen ("/clock-frequency")];
	      char *prop;
	      int fd;

	      /* Note that
		   strlen("/clock-frequency") > strlen("/device_type")
	      */
	      __stpcpy (prop = __stpcpy (__stpcpy (node, "/proc/openprom/"),
					 this_dirp->d_name),
			"/device_type");
	      fd = open (node, O_RDONLY);
	      if (fd != -1)
		{
		  char type_string[128];
		  int ret;

		  ret = read (fd, type_string, sizeof (type_string));
		  if (ret > 0 && strncmp (type_string, "'cpu'", 5) == 0)
		    {
		      int clkfreq_fd;

		      __stpcpy (prop, "/clock-frequency");
		      clkfreq_fd = open (node, O_RDONLY);
		      if (fd != -1)
			{
			  if (read (clkfreq_fd, type_string,
				    sizeof (type_string)) > 0)
			    result = (hp_timing_t)
			      strtoull (type_string, NULL, 16);
			  close (clkfreq_fd);
			}
		    }
		  close (fd);
		}

	      if (result != 0)
		break;

	      len -= this_dirp->d_reclen;
	      this_dirp = (struct dirent *)
		((char *) this_dirp + this_dirp->d_reclen);
	    }
	  if (result != 0)
	    break;
	}
      close (obp_fd);
    }

  return result;
}

static hp_timing_t
__get_clockfreq_via_dev_openprom (void)
{
  hp_timing_t result;
  int obp_dev_fd;

  result = 0;

  obp_dev_fd = open ("/dev/openprom", O_RDONLY);
  if (obp_dev_fd != -1)
    {
      char obp_buf[8192];
      struct openpromio *obp_cmd = (struct openpromio *)obp_buf;
      int ret;

      obp_cmd->oprom_size =
	sizeof (obp_buf) - sizeof (unsigned int);
      *(int *) obp_cmd->oprom_array = 0;
      ret = ioctl (obp_dev_fd, OPROMCHILD, (char *) obp_cmd);
      if (ret == 0)
	{
	  int cur_node = *(int *) obp_cmd->oprom_array;

	  while (cur_node != 0 && cur_node != -1)
	    {
	      obp_cmd->oprom_size = sizeof (obp_buf) - sizeof (unsigned int);
	      strcpy (obp_cmd->oprom_array, "device_type");
	      ret = ioctl (obp_dev_fd, OPROMGETPROP, (char *) obp_cmd);
	      if (ret == 0
		  && strncmp (obp_cmd->oprom_array, "cpu", 3) == 0)
		{
		  obp_cmd->oprom_size = (sizeof (obp_buf)
					 - sizeof (unsigned int));
		  strcpy (obp_cmd->oprom_array, "clock-frequency");
		  ret = ioctl (obp_dev_fd, OPROMGETPROP, (char *) obp_cmd);
		  if (ret == 0)
		    result =
		      (hp_timing_t) *(unsigned int *) obp_cmd->oprom_array;
		}
	      obp_cmd->oprom_size = sizeof (obp_buf) - sizeof (unsigned int);
	      *(int *) obp_cmd->oprom_array = cur_node;
	      ret = ioctl (obp_dev_fd, OPROMNEXT, (char *) obp_cmd);
	      if (ret < 0)
		break;
	      cur_node = *(int *)obp_cmd->oprom_array;
	    }
	}
    }

  return result;
}

hp_timing_t
__get_clockfreq (void)
{
  static hp_timing_t result;

  /* If this function was called before, we know the result.  */
  if (result != 0)
    return result;

  /* We first read the information from the /proc/cpuinfo file.
     It contains at least one line like
	Cpu0ClkTick         : 000000002cb41780
     We search for this line and convert the number in an integer.  */
  result = __get_clockfreq_via_cpuinfo ();
  if (result != 0)
    return result;

  /* If that did not work, try to find an OpenPROM node
     with device_type equal to 'cpu' using /dev/openprom
     and fetch the clock-frequency property from there.  */
  result = __get_clockfreq_via_dev_openprom ();
  if (result != 0)
    return result;

  /* Finally, try the same lookup as above but using /proc/openprom.  */
  result = __get_clockfreq_via_proc_openprom ();

  return result;
}
