/****************************************************************************
 *                                                                          *
 *                          GNAT RUN-TIME COMPONENTS                        *
 *                                                                          *
 *                              C S T R E A M S                             *
 *                                                                          *
 *              Auxiliary C functions for Interfaces.C.Streams              *
 *                                                                          *
 *          Copyright (C) 1992-2009, Free Software Foundation, Inc.         *
 *                                                                          *
 * GNAT is free software;  you can  redistribute it  and/or modify it under *
 * terms of the  GNU General Public License as published  by the Free Soft- *
 * ware  Foundation;  either version 3,  or (at your option) any later ver- *
 * sion.  GNAT is distributed in the hope that it will be useful, but WITH- *
 * OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.                                     *
 *                                                                          *
 * As a special exception under Section 7 of GPL version 3, you are granted *
 * additional permissions described in the GCC Runtime Library Exception,   *
 * version 3.1, as published by the Free Software Foundation.               *
 *                                                                          *
 * You should have received a copy of the GNU General Public License and    *
 * a copy of the GCC Runtime Library Exception along with this program;     *
 * see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see    *
 * <http://www.gnu.org/licenses/>.                                          *
 *                                                                          *
 * GNAT was originally developed  by the GNAT team at  New York University. *
 * Extensive contributions were provided by Ada Core Technologies Inc.      *
 *                                                                          *
 ****************************************************************************/

/* Routines required for implementing routines in Interfaces.C.Streams */

#ifdef __vxworks
#include "vxWorks.h"
#endif

#ifdef IN_RTS
#include "tconfig.h"
#include "tsystem.h"
#include <sys/stat.h>
#else
#include "config.h"
#include "system.h"
#endif

#include "adaint.h"

#ifdef VMS
#include <unixlib.h>
#endif

#ifdef linux
/* Don't use macros on GNU/Linux since they cause incompatible changes between
   glibc 2.0 and 2.1 */

#ifdef stderr
#  undef stderr
#endif
#ifdef stdin
#  undef stdin
#endif
#ifdef stdout
#  undef stdout
#endif

#endif

/* The _IONBF value in MINGW32 stdio.h is wrong.  */
#if defined (WINNT) || defined (_WINNT)
#if OLD_MINGW
#undef _IONBF
#define _IONBF 0004
#endif
#endif

int
__gnat_feof (FILE *stream)
{
  return (feof (stream));
}

int
__gnat_ferror (FILE *stream)
{
   return (ferror (stream));
}

int
__gnat_fileno (FILE *stream)
{
   return (fileno (stream));
}

int
__gnat_is_regular_file_fd (int fd)
{
  int ret;
  struct stat statbuf;

#ifdef __EMX__
  /* Programs using screen I/O may need to reset the FPU after
     initialization of screen-handling related DLL's, so force
     DLL initialization by doing a null-write and then reset the FPU */

  DosWrite (0, &ret, 0, &ret);
  __gnat_init_float();
#endif

  ret = fstat (fd, &statbuf);
  return (!ret && S_ISREG (statbuf.st_mode));
}

/* on some systems, the constants for seek are not defined, if so, then
   provide the conventional definitions */

#ifndef SEEK_SET
#define SEEK_SET 0  /* Set file pointer to offset                           */
#define SEEK_CUR 1  /* Set file pointer to its current value plus offset    */
#define SEEK_END 2  /* Set file pointer to the size of the file plus offset */
#endif

/* if L_tmpnam is not set, use a large number that should be safe */
#ifndef L_tmpnam
#define L_tmpnam 256
#endif

int    __gnat_constant_eof      = EOF;
int    __gnat_constant_iofbf    = _IOFBF;
int    __gnat_constant_iolbf    = _IOLBF;
int    __gnat_constant_ionbf    = _IONBF;
int    __gnat_constant_l_tmpnam = L_tmpnam;
int    __gnat_constant_seek_cur = SEEK_CUR;
int    __gnat_constant_seek_end = SEEK_END;
int    __gnat_constant_seek_set = SEEK_SET;

FILE *
__gnat_constant_stderr (void)
{
  return stderr;
}

FILE *
__gnat_constant_stdin (void)
{
  return stdin;
}

FILE *
__gnat_constant_stdout (void)
{
  return stdout;
}

char *
__gnat_full_name (char *nam, char *buffer)
{
#ifdef RTSS
  /* RTSS applications have no current-directory notion, so RTSS file I/O
     requests must use fully qualified path names, such as:
       c:\temp\MyFile.txt (for a file system object)
       \\.\MyDevice0 (for a device object)
   */
  if (nam[1] == ':' || nam[0] == '\\')
    strcpy (buffer, nam);
  else
    buffer[0] = '\0';

#elif defined(__EMX__) || defined (__MINGW32__)
  /* If this is a device file return it as is; under Windows NT and
     OS/2 a device file end with ":".  */
  if (nam[strlen (nam) - 1] == ':')
    strcpy (buffer, nam);
  else
    {
      char *p;

      _fullpath (buffer, nam, __gnat_max_path_len);

      for (p = buffer; *p; p++)
	if (*p == '/')
	  *p = '\\';
    }

#elif defined (MSDOS)
  _fixpath (nam, buffer);

#elif defined (sgi) || defined (__FreeBSD__)

  /* Use realpath function which resolves links and references to . and ..
     on those Unix systems that support it. Note that GNU/Linux provides it but
     cannot handle more than 5 symbolic links in a full name, so we use the
     getcwd approach instead. */
  realpath (nam, buffer);

#elif defined (VMS)
  strncpy (buffer, __gnat_to_canonical_file_spec (nam), __gnat_max_path_len);

  if (buffer[0] == '/' || strchr (buffer, '!'))  /* '!' means decnet node */
    strncpy (buffer, __gnat_to_host_file_spec (buffer), __gnat_max_path_len);
  else
    {
      char *nambuffer = alloca (__gnat_max_path_len);

      strncpy (nambuffer, buffer, __gnat_max_path_len);
      strncpy
	(buffer, getcwd (buffer, __gnat_max_path_len, 0), __gnat_max_path_len);
      strncat (buffer, "/", __gnat_max_path_len);
      strncat (buffer, nambuffer, __gnat_max_path_len);
      strncpy (buffer, __gnat_to_host_file_spec (buffer), __gnat_max_path_len);
    }

#elif defined (__vxworks)

  /* On VxWorks systems, an absolute path can be represented (depending on
     the host platform) as either /dir/file, or device:/dir/file, or
     device:drive_letter:/dir/file. Use the __gnat_is_absolute_path
     to verify it. */

  int length;

  if (__gnat_is_absolute_path (nam, strlen (nam)))
    strcpy (buffer, nam);

  else
    {
      length = __gnat_max_path_len;
      __gnat_get_current_dir (buffer, &length);
      strncat (buffer, nam, __gnat_max_path_len - length - 1);
    }

#else
  if (nam[0] != '/')
    {
      char *p = getcwd (buffer, __gnat_max_path_len);

      if (p == 0)
	{
	  buffer[0] = '\0';
	  return 0;
	}


      /* If the name returned is an absolute path, it is safe to append '/'
	 to the path and concatenate the name of the file. */
      if (buffer[0] == '/')
	strcat (buffer, "/");

      strcat (buffer, nam);
    }
  else
    strcpy (buffer, nam);
#endif

  return buffer;
}
