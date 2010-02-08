/* Copyright (C) 1993, 1995, 1997, 1998, 1999, 2000, 2002, 2003, 2004
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

#include <libioP.h>
#include <errno.h>

#include <shlib-compat.h>
#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_2)
int
attribute_compat_text_section
_IO_old_fsetpos (fp, posp)
     _IO_FILE *fp;
     const _IO_fpos_t *posp;
{
  int result;
  CHECK_FILE (fp, EOF);
  _IO_acquire_lock (fp);
  if (_IO_seekpos_unlocked (fp, posp->__pos, _IOS_INPUT|_IOS_OUTPUT)
      == _IO_pos_BAD)
    {
      /* ANSI explicitly requires setting errno to a positive value on
	 failure.  */
#ifdef EIO
      if (errno == 0)
	__set_errno (EIO);
#endif
      result = EOF;
    }
  else
    result = 0;
  _IO_release_lock (fp);
  return result;
}

#ifdef weak_alias
compat_symbol (libc, _IO_old_fsetpos, _IO_fsetpos, GLIBC_2_0);
strong_alias (_IO_old_fsetpos, __old_fsetpos)
compat_symbol (libc, __old_fsetpos, fsetpos, GLIBC_2_0);
#endif

#endif
