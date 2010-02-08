/* Copyright (C) 1992, 1997, 1998, 2000 Free Software Foundation, Inc.
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
#include <include/sys/stat.h>
#include <stdlib.h>
#include <string.h>

/* Return a malloc'd string containing the current directory name.
   If the environment variable `PWD' is set, and its value is correct,
   that value is used.  */

char *
get_current_dir_name (void)
{
  char *pwd;
  struct stat64 dotstat, pwdstat;

  pwd = getenv ("PWD");
  if (pwd != NULL
      && stat64 (".", &dotstat) == 0
      && stat64 (pwd, &pwdstat) == 0
      && pwdstat.st_dev == dotstat.st_dev
      && pwdstat.st_ino == dotstat.st_ino)
    /* The PWD value is correct.  Use it.  */
    return __strdup (pwd);

  return __getcwd ((char *) NULL, 0);
}
