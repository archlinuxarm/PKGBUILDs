/* Copyright (C) 1991,1992,1995,1996,1997,2001,2002, 2004
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
   02111-1307 USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <string.h>

#ifndef _LIBC
# define __strcasecmp strcasecmp
# define TOLOWER(Ch) tolower (Ch)
#else
# include <locale/localeinfo.h>
# ifdef USE_IN_EXTENDED_LOCALE_MODEL
#  define __strcasecmp __strcasecmp_l
# endif
# define TOLOWER(Ch) __tolower_l ((Ch), loc)
#endif

#ifdef USE_IN_EXTENDED_LOCALE_MODEL
# define LOCALE_PARAM , loc
# define LOCALE_PARAM_DECL __locale_t loc;
#else
# define LOCALE_PARAM
# define LOCALE_PARAM_DECL
#endif

/* Compare S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexicographically less than,
   equal to or greater than S2.  */
int
__strcasecmp (s1, s2 LOCALE_PARAM)
     const char *s1;
     const char *s2;
     LOCALE_PARAM_DECL
{
#if defined _LIBC && !defined USE_IN_EXTENDED_LOCALE_MODEL
  __locale_t loc = _NL_CURRENT_LOCALE;
#endif
  const unsigned char *p1 = (const unsigned char *) s1;
  const unsigned char *p2 = (const unsigned char *) s2;
  int result;

  if (p1 == p2)
    return 0;

  while ((result = TOLOWER (*p1) - TOLOWER (*p2++)) == 0)
    if (*p1++ == '\0')
      break;

  return result;
}
#ifndef __strcasecmp
libc_hidden_def (__strcasecmp)
weak_alias (__strcasecmp, strcasecmp)
#endif
