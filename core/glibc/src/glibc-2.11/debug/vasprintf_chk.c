/* Copyright (C) 1995,1997,1999-2002,2004,2006,2008,2009
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

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdio_ext.h>
#include "../libio/libioP.h"
#include "../libio/strfile.h"

int
__vasprintf_chk (char **result_ptr, int flags, const char *format,
		 va_list args)
{
  /* Initial size of the buffer to be used.  Will be doubled each time an
     overflow occurs.  */
  const _IO_size_t init_string_size = 100;
  char *string;
  _IO_strfile sf;
  int ret;
  _IO_size_t needed;
  _IO_size_t allocated;
  /* No need to clear the memory here (unlike for open_memstream) since
     we know we will never seek on the stream.  */
  string = (char *) malloc (init_string_size);
  if (string == NULL)
    return -1;
#ifdef _IO_MTSAFE_IO
  sf._sbf._f._lock = NULL;
#endif
  _IO_no_init (&sf._sbf._f, _IO_USER_LOCK, -1, NULL, NULL);
  _IO_JUMPS (&sf._sbf) = &_IO_str_jumps;
  _IO_str_init_static_internal (&sf, string, init_string_size, string);
  sf._sbf._f._flags &= ~_IO_USER_BUF;
  sf._s._allocate_buffer = (_IO_alloc_type) malloc;
  sf._s._free_buffer = (_IO_free_type) free;

  /* For flags > 0 (i.e. __USE_FORTIFY_LEVEL > 1) request that %n
     can only come from read-only format strings.  */
  if (flags > 0)
    sf._sbf._f._flags2 |= _IO_FLAGS2_FORTIFY;

  ret = INTUSE(_IO_vfprintf) (&sf._sbf._f, format, args);
  if (ret < 0)
    {
      free (sf._sbf._f._IO_buf_base);
      return ret;
    }
  /* Only use realloc if the size we need is of the same (binary)
     order of magnitude then the memory we allocated.  */
  needed = sf._sbf._f._IO_write_ptr - sf._sbf._f._IO_write_base + 1;
  allocated = sf._sbf._f._IO_write_end - sf._sbf._f._IO_write_base;
  if ((allocated >> 1) <= needed)
    *result_ptr = (char *) realloc (sf._sbf._f._IO_buf_base, needed);
  else
    {
      *result_ptr = (char *) malloc (needed);
      if (*result_ptr != NULL)
	{
	  memcpy (*result_ptr, sf._sbf._f._IO_buf_base, needed - 1);
	  free (sf._sbf._f._IO_buf_base);
	}
      else
	/* We have no choice, use the buffer we already have.  */
	*result_ptr = (char *) realloc (sf._sbf._f._IO_buf_base, needed);
    }
  if (*result_ptr == NULL)
    *result_ptr = sf._sbf._f._IO_buf_base;
  (*result_ptr)[needed - 1] = '\0';
  return ret;
}
libc_hidden_def (__vasprintf_chk)
