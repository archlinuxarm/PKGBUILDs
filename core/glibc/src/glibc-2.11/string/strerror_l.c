/* Copyright (C) 2007 Free Software Foundation, Inc.
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

#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>


static __thread char *last_value;


static const char *
translate (const char *str, locale_t loc)
{
  locale_t oldloc = __uselocale (loc);
  const char *res = _(str);
  __uselocale (oldloc);
  return res;
}


/* Return a string describing the errno code in ERRNUM.  */
char *
strerror_l (int errnum, locale_t loc)
{


  if (__builtin_expect (errnum < 0 || errnum >= _sys_nerr_internal
			|| _sys_errlist_internal[errnum] == NULL, 0))
    {
      free (last_value);
      if (__asprintf (&last_value, "%s%d",
		      translate ("Unknown error ", loc), errnum) == -1)
	last_value = NULL;

      return last_value;
    }

  return (char *) translate (_sys_errlist_internal[errnum], loc);
}


#ifdef _LIBC
# ifdef _LIBC_REENTRANT
/* This is called when a thread is exiting to free the last_value string.  */
static void __attribute__ ((section ("__libc_thread_freeres_fn")))
strerror_thread_freeres (void)
{
  free (last_value);
}
text_set_element (__libc_thread_subfreeres, strerror_thread_freeres);
text_set_element (__libc_subfreeres, strerror_thread_freeres);
# endif
#endif
