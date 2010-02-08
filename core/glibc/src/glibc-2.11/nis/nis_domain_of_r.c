/* Copyright (c) 1997, 2004, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Thorsten Kukuk <kukuk@vt.uni-paderborn.de>, 1997.

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

#include <errno.h>
#include <string.h>
#include <rpcsvc/nis.h>

nis_name
nis_domain_of_r (const_nis_name name, char *buffer, size_t buflen)
{
  char *cptr;
  size_t cptr_len;

  if (buffer == NULL)
    {
    erange:
      __set_errno (ERANGE);
      return NULL;
    }

  buffer[0] = '\0';

  cptr = strchr (name, '.');

  if (cptr == NULL)
    return buffer;

  ++cptr;
  cptr_len = strlen (cptr);

  if (cptr_len == 0)
    {
      if (buflen < 2)
	goto erange;
      return strcpy (buffer, ".");
    }

  if (__builtin_expect (cptr_len >= buflen, 0))
    {
      __set_errno (ERANGE);
      return NULL;
    }

  return memcpy (buffer, cptr, cptr_len + 1);
}
libnsl_hidden_def (nis_domain_of_r)
