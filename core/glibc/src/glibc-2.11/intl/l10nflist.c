/* Copyright (C) 1995-2002, 2004, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

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

/* Tell glibc's <string.h> to provide a prototype for stpcpy().
   This must come before <config.h> because <config.h> may include
   <features.h>, and once <features.h> has been included, it's too late.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE	1
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#if defined _LIBC || defined HAVE_ARGZ_H
# include <argz.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#include <stdlib.h>

#include "loadinfo.h"

/* On some strange systems still no definition of NULL is found.  Sigh!  */
#ifndef NULL
# if defined __STDC__ && __STDC__
#  define NULL ((void *) 0)
# else
#  define NULL 0
# endif
#endif

/* @@ end of prolog @@ */

#ifdef _LIBC
/* Rename the non ANSI C functions.  This is required by the standard
   because some ANSI C functions will require linking with this object
   file and the name space must not be polluted.  */
# ifndef stpcpy
#  define stpcpy(dest, src) __stpcpy(dest, src)
# endif
#else
# ifndef HAVE_STPCPY
static char *stpcpy PARAMS ((char *dest, const char *src));
# endif
#endif

/* Define function which are usually not available.  */

#if !defined _LIBC && !defined HAVE___ARGZ_COUNT
/* Returns the number of strings in ARGZ.  */
static size_t argz_count__ PARAMS ((const char *argz, size_t len));

static size_t
argz_count__ (argz, len)
     const char *argz;
     size_t len;
{
  size_t count = 0;
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      argz += part_len + 1;
      len -= part_len + 1;
      count++;
    }
  return count;
}
# undef __argz_count
# define __argz_count(argz, len) argz_count__ (argz, len)
#else
# ifdef _LIBC
#  define __argz_count(argz, len) INTUSE(__argz_count) (argz, len)
# endif
#endif	/* !_LIBC && !HAVE___ARGZ_COUNT */

#if !defined _LIBC && !defined HAVE___ARGZ_STRINGIFY
/* Make '\0' separated arg vector ARGZ printable by converting all the '\0's
   except the last into the character SEP.  */
static void argz_stringify__ PARAMS ((char *argz, size_t len, int sep));

static void
argz_stringify__ (argz, len, sep)
     char *argz;
     size_t len;
     int sep;
{
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      argz += part_len;
      len -= part_len + 1;
      if (len > 0)
	*argz++ = sep;
    }
}
# undef __argz_stringify
# define __argz_stringify(argz, len, sep) argz_stringify__ (argz, len, sep)
#else
# ifdef _LIBC
#  define __argz_stringify(argz, len, sep) \
  INTUSE(__argz_stringify) (argz, len, sep)
# endif
#endif	/* !_LIBC && !HAVE___ARGZ_STRINGIFY */

#if !defined _LIBC && !defined HAVE___ARGZ_NEXT
static char *argz_next__ PARAMS ((char *argz, size_t argz_len,
				  const char *entry));

static char *
argz_next__ (argz, argz_len, entry)
     char *argz;
     size_t argz_len;
     const char *entry;
{
  if (entry)
    {
      if (entry < argz + argz_len)
        entry = strchr (entry, '\0') + 1;

      return entry >= argz + argz_len ? NULL : (char *) entry;
    }
  else
    if (argz_len > 0)
      return argz;
    else
      return 0;
}
# undef __argz_next
# define __argz_next(argz, len, entry) argz_next__ (argz, len, entry)
#endif	/* !_LIBC && !HAVE___ARGZ_NEXT */


/* Return number of bits set in X.  */
static int pop PARAMS ((int x));

static inline int
pop (x)
     int x;
{
  /* We assume that no more than 16 bits are used.  */
  x = ((x & ~0x5555) >> 1) + (x & 0x5555);
  x = ((x & ~0x3333) >> 2) + (x & 0x3333);
  x = ((x >> 4) + x) & 0x0f0f;
  x = ((x >> 8) + x) & 0xff;

  return x;
}


struct loaded_l10nfile *
_nl_make_l10nflist (l10nfile_list, dirlist, dirlist_len, mask, language,
		    territory, codeset, normalized_codeset, modifier,
		    filename, do_allocate)
     struct loaded_l10nfile **l10nfile_list;
     const char *dirlist;
     size_t dirlist_len;
     int mask;
     const char *language;
     const char *territory;
     const char *codeset;
     const char *normalized_codeset;
     const char *modifier;
     const char *filename;
     int do_allocate;
{
  char *abs_filename;
  struct loaded_l10nfile *last = NULL;
  struct loaded_l10nfile *retval;
  char *cp;
  size_t entries;
  int cnt;

  /* Allocate room for the full file name.  */
  abs_filename = (char *) malloc (dirlist_len
				  + strlen (language)
				  + ((mask & XPG_TERRITORY) != 0
				     ? strlen (territory) + 1 : 0)
				  + ((mask & XPG_CODESET) != 0
				     ? strlen (codeset) + 1 : 0)
				  + ((mask & XPG_NORM_CODESET) != 0
				     ? strlen (normalized_codeset) + 1 : 0)
				  + ((mask & XPG_MODIFIER) != 0
				     ? strlen (modifier) + 1 : 0)
				  + 1 + strlen (filename) + 1);

  if (abs_filename == NULL)
    return NULL;

  retval = NULL;
  last = NULL;

  /* Construct file name.  */
  memcpy (abs_filename, dirlist, dirlist_len);
  __argz_stringify (abs_filename, dirlist_len, ':');
  cp = abs_filename + (dirlist_len - 1);
  *cp++ = '/';
  cp = stpcpy (cp, language);

  if ((mask & XPG_TERRITORY) != 0)
    {
      *cp++ = '_';
      cp = stpcpy (cp, territory);
    }
  if ((mask & XPG_CODESET) != 0)
    {
      *cp++ = '.';
      cp = stpcpy (cp, codeset);
    }
  if ((mask & XPG_NORM_CODESET) != 0)
    {
      *cp++ = '.';
      cp = stpcpy (cp, normalized_codeset);
    }
  if ((mask & XPG_MODIFIER) != 0)
    {
      *cp++ = '@';
      cp = stpcpy (cp, modifier);
    }

  *cp++ = '/';
  stpcpy (cp, filename);

  /* Look in list of already loaded domains whether it is already
     available.  */
  last = NULL;
  for (retval = *l10nfile_list; retval != NULL; retval = retval->next)
    if (retval->filename != NULL)
      {
	int compare = strcmp (retval->filename, abs_filename);
	if (compare == 0)
	  /* We found it!  */
	  break;
	if (compare < 0)
	  {
	    /* It's not in the list.  */
	    retval = NULL;
	    break;
	  }

	last = retval;
      }

  if (retval != NULL || do_allocate == 0)
    {
      free (abs_filename);
      return retval;
    }

  retval = (struct loaded_l10nfile *)
    malloc (sizeof (*retval) + (__argz_count (dirlist, dirlist_len)
				* (1 << pop (mask))
				* sizeof (struct loaded_l10nfile *)));
  if (retval == NULL)
    {
      free (abs_filename);
      return NULL;
    }

  retval->filename = abs_filename;
  /* If more than one directory is in the list this is a pseudo-entry
     which just references others.  We do not try to load data for it,
     ever.  */
  retval->decided = (__argz_count (dirlist, dirlist_len) != 1
		     || ((mask & XPG_CODESET) != 0
			 && (mask & XPG_NORM_CODESET) != 0));
  retval->data = NULL;

  if (last == NULL)
    {
      retval->next = *l10nfile_list;
      *l10nfile_list = retval;
    }
  else
    {
      retval->next = last->next;
      last->next = retval;
    }

  entries = 0;
  /* If the DIRLIST is a real list the RETVAL entry corresponds not to
     a real file.  So we have to use the DIRLIST separation mechanism
     of the inner loop.  */
  cnt = __argz_count (dirlist, dirlist_len) == 1 ? mask - 1 : mask;
  for (; cnt >= 0; --cnt)
    if ((cnt & ~mask) == 0)
      {
	/* Iterate over all elements of the DIRLIST.  */
	char *dir = NULL;

	while ((dir = __argz_next ((char *) dirlist, dirlist_len, dir))
	       != NULL)
	  retval->successor[entries++]
	    = _nl_make_l10nflist (l10nfile_list, dir, strlen (dir) + 1, cnt,
				  language, territory, codeset,
				  normalized_codeset, modifier, filename, 1);
      }
  retval->successor[entries] = NULL;

  return retval;
}

/* Normalize codeset name.  There is no standard for the codeset
   names.  Normalization allows the user to use any of the common
   names.  The return value is dynamically allocated and has to be
   freed by the caller.  */
const char *
_nl_normalize_codeset (codeset, name_len)
     const char *codeset;
     size_t name_len;
{
  int len = 0;
  int only_digit = 1;
  char *retval;
  char *wp;
  size_t cnt;

  for (cnt = 0; cnt < name_len; ++cnt)
    if (isalnum ((unsigned char) codeset[cnt]))
      {
	++len;

	if (isalpha ((unsigned char) codeset[cnt]))
	  only_digit = 0;
      }

  retval = (char *) malloc ((only_digit ? 3 : 0) + len + 1);

  if (retval != NULL)
    {
      if (only_digit)
	wp = stpcpy (retval, "iso");
      else
	wp = retval;

      for (cnt = 0; cnt < name_len; ++cnt)
	if (isalpha ((unsigned char) codeset[cnt]))
	  *wp++ = tolower ((unsigned char) codeset[cnt]);
	else if (isdigit ((unsigned char) codeset[cnt]))
	  *wp++ = codeset[cnt];

      *wp = '\0';
    }

  return (const char *) retval;
}


/* @@ begin of epilog @@ */

/* We don't want libintl.a to depend on any other library.  So we
   avoid the non-standard function stpcpy.  In GNU C Library this
   function is available, though.  Also allow the symbol HAVE_STPCPY
   to be defined.  */
#if !_LIBC && !HAVE_STPCPY
static char *
stpcpy (dest, src)
     char *dest;
     const char *src;
{
  while ((*dest++ = *src++) != '\0')
    /* Do nothing. */ ;
  return dest - 1;
}
#endif
