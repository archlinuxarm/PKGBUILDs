/* Copyright (C) 1993, 1996, 1997, 1998, 2002, 2003, 2004
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

#include "../libio/libioP.h"
#include <limits.h>

char *
__gets_chk (char *buf, size_t size)
{
  _IO_size_t count;
  int ch;
  char *retval;

  if (size == 0)
    __chk_fail ();

  _IO_acquire_lock (_IO_stdin);
  ch = _IO_getc_unlocked (_IO_stdin);
  if (ch == EOF)
    {
      retval = NULL;
      goto unlock_return;
    }
  if (ch == '\n')
    count = 0;
  else
    {
      /* This is very tricky since a file descriptor may be in the
	 non-blocking mode. The error flag doesn't mean much in this
	 case. We return an error only when there is a new error. */
      int old_error = _IO_stdin->_IO_file_flags & _IO_ERR_SEEN;
      _IO_stdin->_IO_file_flags &= ~_IO_ERR_SEEN;
      buf[0] = (char) ch;
      count = INTUSE(_IO_getline) (_IO_stdin, buf + 1, size - 1, '\n', 0) + 1;
      if (_IO_stdin->_IO_file_flags & _IO_ERR_SEEN)
	{
	  retval = NULL;
	  goto unlock_return;
	}
      else
	_IO_stdin->_IO_file_flags |= old_error;
    }
  if (count >= size)
    __chk_fail ();
  buf[count] = 0;
  retval = buf;
unlock_return:
  _IO_release_lock (_IO_stdin);
  return retval;
}

link_warning (__gets_chk, "the `gets' function is dangerous and should not be used.")
