/* Copyright (C) 1991, 1992, 1995, 1996, 1997, 2001, 2002, 2004, 2005, 2009
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

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <libintl.h>
#include <wchar.h>


/* Defined in sys_siglist.c.  */
extern const char *const _sys_siglist[];
extern const char *const _sys_siglist_internal[] attribute_hidden;


/* Print out on stderr a line consisting of the test in S, a colon, a space,
   a message describing the meaning of the signal number SIG and a newline.
   If S is NULL or "", the colon and space are omitted.  */
void
psignal (int sig, const char *s)
{
  const char *colon, *desc;

  if (s == NULL || *s == '\0')
    s = colon = "";
  else
    colon = ": ";

  if (sig >= 0 && sig < NSIG && (desc = INTUSE(_sys_siglist)[sig]) != NULL)
    (void) __fxprintf (NULL, "%s%s%s\n", s, colon, _(desc));
  else
    {
      char *buf;

      if (__asprintf (&buf, _("%s%sUnknown signal %d\n"), s, colon, sig) < 0)
	(void) __fxprintf (NULL, "%s%s%s\n", s, colon, _("Unknown signal"));
      else
	{
	  (void) __fxprintf (NULL, "%s", buf);

	  free (buf);
	}
    }
}
