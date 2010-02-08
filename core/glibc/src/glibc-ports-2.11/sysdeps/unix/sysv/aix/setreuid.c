/* Copyright (C) 2000 Free Software Foundation, Inc.
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

#include <unistd.h>

#define ID_EFFECTIVE	0x01
#define ID_REAL		0x02


extern int setuidx (int mask, uid_t uid);

int
__setreuid (uid_t ruid, uid_t euid)
{
  int res;

  if (ruid == euid)
    return setuidx (ID_EFFECTIVE | ID_REAL, euid);

  res = setuidx (ID_REAL, ruid);
  if (res == 0)
    res = setuidx (ID_EFFECTIVE, euid);

  return res;
}
strong_alias (__setreuid, setreuid)
