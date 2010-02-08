/* Common code for file-based databases in nss_files module.
   Copyright (C) 1996-1999,2001,2002,2004,2007,2008
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
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <bits/libc-lock.h>
#include "nsswitch.h"

#include <kernel-features.h>

/* These symbols are defined by the including source file:

   ENTNAME -- database name of the structure and functions (hostent, pwent).
   STRUCTURE -- struct name, define only if not ENTNAME (passwd, group).
   DATABASE -- string of the database file's name ("hosts", "passwd").

   NEED_H_ERRNO - defined iff an arg `int *herrnop' is used.

   Also see files-parse.c.
*/

#define ENTNAME_r	CONCAT(ENTNAME,_r)

#define DATAFILE	"/etc/" DATABASE

#ifdef NEED_H_ERRNO
# include <netdb.h>
# define H_ERRNO_PROTO	, int *herrnop
# define H_ERRNO_ARG	, herrnop
# define H_ERRNO_SET(val) (*herrnop = (val))
#else
# define H_ERRNO_PROTO
# define H_ERRNO_ARG
# define H_ERRNO_SET(val) ((void) 0)
#endif

#ifndef EXTRA_ARGS
# define EXTRA_ARGS
# define EXTRA_ARGS_DECL
# define EXTRA_ARGS_VALUE
#endif

/* Locks the static variables in this file.  */
__libc_lock_define_initialized (static, lock)

/* Maintenance of the shared stream open on the database file.  */

static FILE *stream;
static fpos_t position;
static enum { nouse, getent, getby } last_use;
static int keep_stream;

/* Open database file if not already opened.  */
static enum nss_status
internal_setent (int stayopen)
{
  enum nss_status status = NSS_STATUS_SUCCESS;

  if (stream == NULL)
    {
      stream = fopen (DATAFILE, "re");

      if (stream == NULL)
	status = errno == EAGAIN ? NSS_STATUS_TRYAGAIN : NSS_STATUS_UNAVAIL;
      else
	{
#if !defined O_CLOEXEC || !defined __ASSUME_O_CLOEXEC
# ifdef O_CLOEXEC
	  if (__have_o_cloexec <= 0)
# endif
	    {
	      /* We have to make sure the file is  `closed on exec'.  */
	      int result;
	      int flags;

	      result = flags = fcntl (fileno (stream), F_GETFD, 0);
	      if (result >= 0)
		{
# ifdef O_CLOEXEC
		  if (__have_o_cloexec == 0)
		    __have_o_cloexec = (flags & FD_CLOEXEC) == 0 ? -1 : 1;
		  if (__have_o_cloexec < 0)
# endif
		    {
		      flags |= FD_CLOEXEC;
		      result = fcntl (fileno (stream), F_SETFD, flags);
		    }
		}
	      if (result < 0)
		{
		  /* Something went wrong.  Close the stream and return a
		     failure.  */
		  fclose (stream);
		  stream = NULL;
		  status = NSS_STATUS_UNAVAIL;
		}
	    }
#endif
	}
    }
  else
    rewind (stream);

  /* Remember STAYOPEN flag.  */
  if (stream != NULL)
    keep_stream |= stayopen;

  return status;
}


/* Thread-safe, exported version of that.  */
enum nss_status
CONCAT(_nss_files_set,ENTNAME) (int stayopen)
{
  enum nss_status status;

  __libc_lock_lock (lock);

  status = internal_setent (stayopen);

  if (status == NSS_STATUS_SUCCESS && fgetpos (stream, &position) < 0)
    {
      fclose (stream);
      stream = NULL;
      status = NSS_STATUS_UNAVAIL;
    }

  last_use = getent;

  __libc_lock_unlock (lock);

  return status;
}


/* Close the database file.  */
static void
internal_endent (void)
{
  if (stream != NULL)
    {
      fclose (stream);
      stream = NULL;
    }
}


/* Thread-safe, exported version of that.  */
enum nss_status
CONCAT(_nss_files_end,ENTNAME) (void)
{
  __libc_lock_lock (lock);

  internal_endent ();

  /* Reset STAYOPEN flag.  */
  keep_stream = 0;

  __libc_lock_unlock (lock);

  return NSS_STATUS_SUCCESS;
}

/* Parsing the database file into `struct STRUCTURE' data structures.  */

static enum nss_status
internal_getent (struct STRUCTURE *result,
		 char *buffer, size_t buflen, int *errnop H_ERRNO_PROTO
		 EXTRA_ARGS_DECL)
{
  char *p;
  struct parser_data *data = (void *) buffer;
  int linebuflen = buffer + buflen - data->linebuffer;
  int parse_result;

  if (buflen < sizeof *data + 2)
    {
      *errnop = ERANGE;
      H_ERRNO_SET (NETDB_INTERNAL);
      return NSS_STATUS_TRYAGAIN;
    }

  do
    {
      /* Terminate the line so that we can test for overflow.  */
      ((unsigned char *) data->linebuffer)[linebuflen - 1] = '\xff';

      p = fgets_unlocked (data->linebuffer, linebuflen, stream);
      if (p == NULL)
	{
	  /* End of file or read error.  */
	  H_ERRNO_SET (HOST_NOT_FOUND);
	  return NSS_STATUS_NOTFOUND;
	}
      else if (((unsigned char *) data->linebuffer)[linebuflen - 1] != 0xff)
	{
	  /* The line is too long.  Give the user the opportunity to
	     enlarge the buffer.  */
	  *errnop = ERANGE;
	  H_ERRNO_SET (NETDB_INTERNAL);
	  return NSS_STATUS_TRYAGAIN;
	}

      /* Skip leading blanks.  */
      while (isspace (*p))
	++p;
    }
  while (*p == '\0' || *p == '#' /* Ignore empty and comment lines.  */
	 /* Parse the line.  If it is invalid, loop to get the next
	    line of the file to parse.  */
	 || ! (parse_result = parse_line (p, result, data, buflen, errnop
					  EXTRA_ARGS)));

  if (__builtin_expect (parse_result == -1, 0))
    {
      H_ERRNO_SET (NETDB_INTERNAL);
      return NSS_STATUS_TRYAGAIN;
    }

  /* Filled in RESULT with the next entry from the database file.  */
  return NSS_STATUS_SUCCESS;
}


/* Return the next entry from the database file, doing locking.  */
enum nss_status
CONCAT(_nss_files_get,ENTNAME_r) (struct STRUCTURE *result, char *buffer,
				  size_t buflen, int *errnop H_ERRNO_PROTO)
{
  /* Return next entry in host file.  */
  enum nss_status status = NSS_STATUS_SUCCESS;

  __libc_lock_lock (lock);

  /* Be prepared that the set*ent function was not called before.  */
  if (stream == NULL)
    {
      int save_errno = errno;

      status = internal_setent (0);

      __set_errno (save_errno);

      if (status == NSS_STATUS_SUCCESS && fgetpos (stream, &position) < 0)
	{
	  fclose (stream);
	  stream = NULL;
	  status = NSS_STATUS_UNAVAIL;
	}
    }

  if (status == NSS_STATUS_SUCCESS)
    {
      /* If the last use was not by the getent function we need the
	 position the stream.  */
      if (last_use != getent)
	{
	  if (fsetpos (stream, &position) < 0)
	    status = NSS_STATUS_UNAVAIL;
	  else
	    last_use = getent;
	}

      if (status == NSS_STATUS_SUCCESS)
	{
	  status = internal_getent (result, buffer, buflen, errnop
				    H_ERRNO_ARG EXTRA_ARGS_VALUE);

	  /* Remember this position if we were successful.  If the
	     operation failed we give the user a chance to repeat the
	     operation (perhaps the buffer was too small).  */
	  if (status == NSS_STATUS_SUCCESS)
	    fgetpos (stream, &position);
	  else
	    /* We must make sure we reposition the stream the next call.  */
	    last_use = nouse;
	}
    }

  __libc_lock_unlock (lock);

  return status;
}

/* Macro for defining lookup functions for this file-based database.

   NAME is the name of the lookup; e.g. `hostbyname'.

   KEYSIZE and KEYPATTERN are ignored here but used by ../nss_db/db-XXX.c.

   PROTO describes the arguments for the lookup key;
   e.g. `const char *hostname'.

   BREAK_IF_MATCH is a block of code which compares `struct STRUCTURE *result'
   to the lookup key arguments and does `break;' if they match.  */

#define DB_LOOKUP(name, keysize, keypattern, break_if_match, proto...)	      \
enum nss_status								      \
_nss_files_get##name##_r (proto,					      \
			  struct STRUCTURE *result, char *buffer,	      \
			  size_t buflen, int *errnop H_ERRNO_PROTO)	      \
{									      \
  enum nss_status status;						      \
									      \
  __libc_lock_lock (lock);						      \
									      \
  /* Reset file pointer to beginning or open file.  */			      \
  status = internal_setent (keep_stream);				      \
									      \
  if (status == NSS_STATUS_SUCCESS)					      \
    {									      \
      /* Tell getent function that we have repositioned the file pointer.  */ \
      last_use = getby;							      \
									      \
      while ((status = internal_getent (result, buffer, buflen, errnop	      \
					H_ERRNO_ARG EXTRA_ARGS_VALUE))	      \
	     == NSS_STATUS_SUCCESS)					      \
	{ break_if_match }						      \
									      \
      if (! keep_stream)						      \
	internal_endent ();						      \
    }									      \
									      \
  __libc_lock_unlock (lock);						      \
									      \
  return status;							      \
}
