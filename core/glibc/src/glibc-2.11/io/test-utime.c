/* Copyright (C) 1994,1996,1997,2000,2002,2003 Free Software Foundation, Inc.
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <time.h>

int
main (int argc, char *argv[])
{
  char file[L_tmpnam];
  struct utimbuf ut;
  struct stat st;
  struct stat stnow;
  time_t now1, now2;
  int fd;

  if (tmpnam (file) == 0)
    {
      perror ("tmpnam");
      return 1;
    }

  fd = creat (file, 0666);
  if (fd < 0)
    {
      perror ("creat");
      return 1;
    }
  close (fd);

  /* Test utime with arg */
  ut.actime = 500000000;
  ut.modtime = 500000001;
  if (utime (file, &ut))
    {
      perror ("utime");
      remove (file);
      return 1;
    }

  if (stat (file, &st))
    {
      perror ("stat");
      remove (file);
      return 1;
    }

  /* Test utime with NULL.
     Since there's a race condition possible here, we check
     the time before and after the call to utime.  */
  now1 = time (NULL);
  if (now1 == (time_t)-1)
    {
      perror ("time");
      remove (file);
      return 1;
    }

  /* The clocks used to set the modification time and that used in the
     time() call need not be the same.  They need not have the same
     precision.  Therefore we delay the following operation by one
     second which makes sure we can compare with second precision.  */
  sleep (1);

  if (utime (file, NULL))
    {
      perror ("utime NULL");
      remove (file);
      return 1;
    }

  sleep (1);

  now2 = time (NULL);
  if (now2 == (time_t)-1)
    {
      perror ("time");
      remove (file);
      return 1;
    }

  if (stat (file, &stnow))
    {
      perror ("stat");
      remove (file);
      return 1;
    }

  remove (file);

  if (st.st_mtime != ut.modtime)
    {
      printf ("modtime %ld != %ld\n", st.st_mtime, ut.modtime);
      return 1;
    }

  if (st.st_atime != ut.actime)
    {
      printf ("actime %ld != %ld\n", st.st_atime, ut.actime);
      return 1;
    }

  if (stnow.st_mtime < now1 || stnow.st_mtime > now2)
    {
      printf ("modtime %ld <%ld >%ld\n", stnow.st_mtime, now1, now2);
      return 1;
    }

  if (stnow.st_atime < now1 || stnow.st_atime > now2)
    {
      printf ("actime %ld <%ld >%ld\n", stnow.st_atime, now1, now2);
      return 1;
    }

  puts ("Test succeeded.");
  return 0;
}
