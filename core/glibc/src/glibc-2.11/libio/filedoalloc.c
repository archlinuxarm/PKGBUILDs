/* Copyright (C) 1993, 1997, 2001, 2002 Free Software Foundation, Inc.
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

/*
   Copyright (C) 1990 The Regents of the University of California.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   4. Neither the name of the University nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.*/

/* Modified for GNU iostream by Per Bothner 1991, 1992. */

#ifndef _POSIX_SOURCE
# define _POSIX_SOURCE
#endif
#include "libioP.h"
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __STDC__
#include <stdlib.h>
#include <unistd.h>
#endif

#ifdef _LIBC
# undef isatty
# define isatty(Fd) __isatty (Fd)

# include <device-nrs.h>
#endif

/*
 * Allocate a file buffer, or switch to unbuffered I/O.
 * Per the ANSI C standard, ALL tty devices default to line buffered.
 *
 * As a side effect, we set __SOPT or __SNPT (en/dis-able fseek
 * optimisation) right after the _fstat() that finds the buffer size.
 */

int
_IO_file_doallocate (fp)
     _IO_FILE *fp;
{
  _IO_size_t size;
  char *p;
  struct _G_stat64 st;

#ifndef _LIBC
  /* If _IO_cleanup_registration_needed is non-zero, we should call the
     function it points to.  This is to make sure _IO_cleanup gets called
     on exit.  We call it from _IO_file_doallocate, since that is likely
     to get called by any program that does buffered I/O. */
  if (__builtin_expect (_IO_cleanup_registration_needed != NULL, 0))
    (*_IO_cleanup_registration_needed) ();
#endif

  size = _IO_BUFSIZ;
  if (fp->_fileno >= 0 && __builtin_expect (_IO_SYSSTAT (fp, &st), 0) >= 0)
    {
      if (S_ISCHR (st.st_mode))
	{
	  /* Possibly a tty.  */
	  if (
#ifdef DEV_TTY_P
	      DEV_TTY_P (&st) ||
#endif
	      isatty (fp->_fileno))
	    fp->_flags |= _IO_LINE_BUF;
	}
#if _IO_HAVE_ST_BLKSIZE
      if (st.st_blksize > 0)
	size = st.st_blksize;
#endif
    }
  ALLOC_BUF (p, size, EOF);
  INTUSE(_IO_setb) (fp, p, p + size, 1);
  return 1;
}
INTDEF(_IO_file_doallocate)
