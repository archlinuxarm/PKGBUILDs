/* fpurge - Flushing buffers of a FILE stream. */

/* Copyright (C) 2007 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#include "stdc.h"

#include <stdio.h>

/* Specification.  Same as in ../../externs.h.  */
#define NEED_FPURGE_DECL
#if HAVE_FPURGE
#  define fpurge _bash_fpurge
#endif
extern int fpurge __P((FILE *stream));

#if HAVE___FPURGE                   /* glibc >= 2.2, Haiku, Solaris >= 7 */
# include <stdio_ext.h>
#endif
#include <stdlib.h>

int
fpurge (FILE *fp)
{
#if HAVE___FPURGE                   /* glibc >= 2.2, Haiku, Solaris >= 7 */

  __fpurge (fp);
  /* The __fpurge function does not have a return value.  */
  return 0;

#elif HAVE_FPURGE                   /* FreeBSD, NetBSD, OpenBSD, DragonFly, MacOS X */

  /* Call the system's fpurge function.  */
# undef fpurge
# if !HAVE_DECL_FPURGE
  extern int fpurge (FILE *);
# endif
  int result = fpurge (fp);
# if defined __sferror || defined __DragonFly__ /* FreeBSD, NetBSD, OpenBSD, DragonFly, MacOS X, Cygwin */
  if (result == 0)
    /* Correct the invariants that fpurge broke.
       <stdio.h> on BSD systems says:
         "The following always hold: if _flags & __SRD, _w is 0."
       If this invariant is not fulfilled and the stream is read-write but
       currently writing, subsequent putc or fputc calls will write directly
       into the buffer, although they shouldn't be allowed to.  */
    if ((fp->_flags & __SRD) != 0)
      fp->_w = 0;
# endif
  return result;

#else

  /* Most systems provide FILE as a struct and the necessary bitmask in
     <stdio.h>, because they need it for implementing getc() and putc() as
     fast macros.  */
# if defined _IO_ftrylockfile || __GNU_LIBRARY__ == 1 /* GNU libc, BeOS, Haiku, Linux libc5 */
  fp->_IO_read_end = fp->_IO_read_ptr;
  fp->_IO_write_ptr = fp->_IO_write_base;
  /* Avoid memory leak when there is an active ungetc buffer.  */
  if (fp->_IO_save_base != NULL)
    {
      free (fp->_IO_save_base);
      fp->_IO_save_base = NULL;
    }
  return 0;
# elif defined __sferror || defined __DragonFly__ /* FreeBSD, NetBSD, OpenBSD, DragonFly, MacOS X, Cygwin */
  fp_->_p = fp_->_bf._base;
  fp_->_r = 0;
  fp_->_w = ((fp_->_flags & (__SLBF | __SNBF | __SRD)) == 0 /* fully buffered and not currently reading? */
	     ? fp_->_bf._size
	     : 0);
  /* Avoid memory leak when there is an active ungetc buffer.  */
  if (fp_ub._base != NULL)
    {
      if (fp_ub._base != fp_->_ubuf)
	free (fp_ub._base);
      fp_ub._base = NULL;
    }
  return 0;
# elif defined __EMX__              /* emx+gcc */
  fp->_ptr = fp->_buffer;
  fp->_rcount = 0;
  fp->_wcount = 0;
  fp->_ungetc_count = 0;
  return 0;
# elif defined _IOERR               /* AIX, HP-UX, IRIX, OSF/1, Solaris, OpenServer, mingw */
  fp->_ptr = fp->_base;
  if (fp->_ptr != NULL)
    fp->_cnt = 0;
  return 0;
# elif defined __UCLIBC__           /* uClibc */
#  ifdef __STDIO_BUFFERS
  if (fp->__modeflags & __FLAG_WRITING)
    fp->__bufpos = fp->__bufstart;
  else if (fp->__modeflags & (__FLAG_READONLY | __FLAG_READING))
    fp->__bufpos = fp->__bufread;
#  endif
  return 0;
# elif defined __QNX__              /* QNX */
  fp->_Rback = fp->_Back + sizeof (fp->_Back);
  fp->_Rsave = NULL;
  if (fp->_Mode & 0x2000 /* _MWRITE */)
    /* fp->_Buf <= fp->_Next <= fp->_Wend */
    fp->_Next = fp->_Buf;
  else
    /* fp->_Buf <= fp->_Next <= fp->_Rend */
    fp->_Rend = fp->_Next;
  return 0;
# elif defined __MINT__             /* Atari FreeMiNT */
  if (fp->__pushed_back)
    {
      fp->__bufp = fp->__pushback_bufp;
      fp->__pushed_back = 0;
    }
  /* Preserve the current file position.  */
  if (fp->__target != -1)
    fp->__target += fp->__bufp - fp->__buffer;
  fp->__bufp = fp->__buffer;
  /* Nothing in the buffer, next getc is nontrivial.  */
  fp->__get_limit = fp->__bufp;
  /* Nothing in the buffer, next putc is nontrivial.  */
  fp->__put_limit = fp->__buffer;
  return 0;
# else
# warning "Please port gnulib fpurge.c to your platform! Look at the definitions of fflush, setvbuf and ungetc on your system, then report this to bug-gnulib."
  return 0;
# endif

#endif
}
