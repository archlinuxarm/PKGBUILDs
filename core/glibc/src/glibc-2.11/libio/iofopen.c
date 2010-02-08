/* Copyright (C) 1993,1997,1998,1999,2000,2002,2003
	Free Software Foundation, Inc.
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
   02111-1307 USA.

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.  */

#include "libioP.h"
#ifdef __STDC__
#include <stdlib.h>
#include <stddef.h>
#endif
#ifdef _LIBC
# include <shlib-compat.h>
#else
# define _IO_new_fopen fopen
#endif

_IO_FILE *
__fopen_maybe_mmap (fp)
     _IO_FILE *fp;
{
#ifdef _G_HAVE_MMAP
  if ((fp->_flags2 & _IO_FLAGS2_MMAP) && (fp->_flags & _IO_NO_WRITES))
    {
      /* Since this is read-only, we might be able to mmap the contents
	 directly.  We delay the decision until the first read attempt by
	 giving it a jump table containing functions that choose mmap or
	 vanilla file operations and reset the jump table accordingly.  */

      if (fp->_mode <= 0)
	_IO_JUMPS ((struct _IO_FILE_plus *) fp) = &_IO_file_jumps_maybe_mmap;
      else
	_IO_JUMPS ((struct _IO_FILE_plus *) fp) = &_IO_wfile_jumps_maybe_mmap;
      fp->_wide_data->_wide_vtable = &_IO_wfile_jumps_maybe_mmap;
    }
#endif
  return fp;
}


_IO_FILE *
__fopen_internal (filename, mode, is32)
     const char *filename;
     const char *mode;
     int is32;
{
  struct locked_FILE
  {
    struct _IO_FILE_plus fp;
#ifdef _IO_MTSAFE_IO
    _IO_lock_t lock;
#endif
    struct _IO_wide_data wd;
  } *new_f = (struct locked_FILE *) malloc (sizeof (struct locked_FILE));

  if (new_f == NULL)
    return NULL;
#ifdef _IO_MTSAFE_IO
  new_f->fp.file._lock = &new_f->lock;
#endif
#if defined _LIBC || defined _GLIBCPP_USE_WCHAR_T
  _IO_no_init (&new_f->fp.file, 0, 0, &new_f->wd, &_IO_wfile_jumps);
#else
  _IO_no_init (&new_f->fp.file, 1, 0, NULL, NULL);
#endif
  _IO_JUMPS (&new_f->fp) = &_IO_file_jumps;
  INTUSE(_IO_file_init) (&new_f->fp);
#if  !_IO_UNIFIED_JUMPTABLES
  new_f->fp.vtable = NULL;
#endif
  if (INTUSE(_IO_file_fopen) ((_IO_FILE *) new_f, filename, mode, is32)
      != NULL)
    return __fopen_maybe_mmap (&new_f->fp.file);

  INTUSE(_IO_un_link) (&new_f->fp);
  free (new_f);
  return NULL;
}

_IO_FILE *
_IO_new_fopen (filename, mode)
     const char *filename;
     const char *mode;
{
  return __fopen_internal (filename, mode, 1);
}

#ifdef _LIBC
strong_alias (_IO_new_fopen, __new_fopen)
versioned_symbol (libc, _IO_new_fopen, _IO_fopen, GLIBC_2_1);
versioned_symbol (libc, __new_fopen, fopen, GLIBC_2_1);
#endif
