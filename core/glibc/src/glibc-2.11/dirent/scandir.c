/* Copyright (C) 1992-1998,2000,2002,2003,2009 Free Software Foundation, Inc.
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

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <bits/libc-lock.h>

#ifndef SCANDIR
#define SCANDIR scandir
#define READDIR __readdir
#define DIRENT_TYPE struct dirent
#endif

#ifndef SCANDIR_CANCEL
#define SCANDIR_CANCEL
struct scandir_cancel_struct
{
  DIR *dp;
  void *v;
  size_t cnt;
};

static void
cancel_handler (void *arg)
{
  struct scandir_cancel_struct *cp = arg;
  size_t i;
  void **v = cp->v;

  for (i = 0; i < cp->cnt; ++i)
    free (v[i]);
  free (v);
  (void) __closedir (cp->dp);
}
#endif


int
SCANDIR (dir, namelist, select, cmp)
     const char *dir;
     DIRENT_TYPE ***namelist;
     int (*select) (const DIRENT_TYPE *);
     int (*cmp) (const DIRENT_TYPE **, const DIRENT_TYPE **);
{
  DIR *dp = __opendir (dir);
  DIRENT_TYPE **v = NULL;
  size_t vsize = 0;
  struct scandir_cancel_struct c;
  DIRENT_TYPE *d;
  int save;

  if (dp == NULL)
    return -1;

  save = errno;
  __set_errno (0);

  c.dp = dp;
  c.v = NULL;
  c.cnt = 0;
  __libc_cleanup_push (cancel_handler, &c);

  while ((d = READDIR (dp)) != NULL)
    {
      int use_it = select == NULL;

      if (! use_it)
	{
	  use_it = select (d);
	  /* The select function might have changed errno.  It was
	     zero before and it need to be again to make the latter
	     tests work.  */
	  __set_errno (0);
	}

      if (use_it)
	{
	  DIRENT_TYPE *vnew;
	  size_t dsize;

	  /* Ignore errors from select or readdir */
	  __set_errno (0);

	  if (__builtin_expect (c.cnt == vsize, 0))
	    {
	      DIRENT_TYPE **new;
	      if (vsize == 0)
		vsize = 10;
	      else
		vsize *= 2;
	      new = (DIRENT_TYPE **) realloc (v, vsize * sizeof (*v));
	      if (new == NULL)
		break;
	      v = new;
	      c.v = (void *) v;
	    }

	  dsize = &d->d_name[_D_ALLOC_NAMLEN (d)] - (char *) d;
	  vnew = (DIRENT_TYPE *) malloc (dsize);
	  if (vnew == NULL)
	    break;

	  v[c.cnt++] = (DIRENT_TYPE *) memcpy (vnew, d, dsize);
	}
    }

  if (__builtin_expect (errno, 0) != 0)
    {
      save = errno;

      while (c.cnt > 0)
	free (v[--c.cnt]);
      free (v);
      c.cnt = -1;
    }
  else
    {
      /* Sort the list if we have a comparison function to sort with.  */
      if (cmp != NULL)
	qsort (v, c.cnt, sizeof (*v),
	       (int (*) (const void *, const void *)) cmp);

      *namelist = v;
    }

  __libc_cleanup_pop (0);

  (void) __closedir (dp);
  __set_errno (save);

  return c.cnt;
}
