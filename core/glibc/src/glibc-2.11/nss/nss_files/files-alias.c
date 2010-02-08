/* Mail alias file parser in nss_files module.
   Copyright (C) 1996,97,98,99,2002,2006,2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#include <aliases.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <bits/libc-lock.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <kernel-features.h>

#include "nsswitch.h"

/* Locks the static variables in this file.  */
__libc_lock_define_initialized (static, lock)

/* Maintenance of the shared stream open on the database file.  */

static FILE *stream;
static fpos_t position;
static enum { nouse, getent, getby } last_use;


static enum nss_status
internal_setent (void)
{
  enum nss_status status = NSS_STATUS_SUCCESS;

  if (stream == NULL)
    {
      stream = fopen ("/etc/aliases", "re");

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

  return status;
}


/* Thread-safe, exported version of that.  */
enum nss_status
_nss_files_setaliasent (void)
{
  enum nss_status status;

  __libc_lock_lock (lock);

  status = internal_setent ();

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
_nss_files_endaliasent (void)
{
  __libc_lock_lock (lock);

  internal_endent ();

  __libc_lock_unlock (lock);

  return NSS_STATUS_SUCCESS;
}

/* Parsing the database file into `struct aliasent' data structures.  */
static enum nss_status
get_next_alias (const char *match, struct aliasent *result,
		char *buffer, size_t buflen, int *errnop)
{
  enum nss_status status = NSS_STATUS_NOTFOUND;
  int ignore = 0;

  result->alias_members_len = 0;

  while (1)
    {
      /* Now we are ready to process the input.  We have to read a
	 line and all its continuations and construct the array of
	 string pointers.  This pointers and the names itself have to
	 be placed in BUFFER.  */
      char *first_unused = buffer;
      size_t room_left = buflen - (buflen % __alignof__ (char *));
      char *line;

      /* Check whether the buffer is large enough for even trying to
         read something.  */
      if (room_left < 2)
	goto no_more_room;

      /* Read the first line.  It must contain the alias name and
	 possibly some alias names.  */
      first_unused[room_left - 1] = '\xff';
      line = fgets_unlocked (first_unused, room_left, stream);
      if (line == NULL)
	/* Nothing to read.  */
	break;
      else if (first_unused[room_left - 1] != '\xff')
	{
	  /* The line is too long for our buffer.  */
	no_more_room:
	  *errnop = ERANGE;
	  status = NSS_STATUS_TRYAGAIN;
	  break;
	}
      else
	{
	  char *cp;

	  /* If we are in IGNORE mode and the first character in the
	     line is a white space we ignore the line and start
	     reading the next.  */
	  if (ignore && isspace (*first_unused))
	    continue;

	  /* Terminate the line for any case.  */
	  cp = strpbrk (first_unused, "#\n");
	  if (cp != NULL)
	    *cp = '\0';

	  /* Skip leading blanks.  */
	  while (isspace (*line))
	    ++line;

	  result->alias_name = first_unused;
	  while (*line != '\0' && *line != ':')
	    *first_unused++ = *line++;
	  if (*line == '\0' || result->alias_name == first_unused)
	    /* No valid name.  Ignore the line.  */
	    continue;

	  *first_unused++ = '\0';
	  if (room_left < (size_t) (first_unused - result->alias_name))
	    goto no_more_room;
	  room_left -= first_unused - result->alias_name;
	  ++line;

	  /* When we search for a specific alias we can avoid all the
	     difficult parts and compare now with the name we are
	     looking for.  If it does not match we simply ignore all
	     lines until the next line containing the start of a new
	     alias is found.  */
	  ignore = (match != NULL
		    && __strcasecmp (result->alias_name, match) != 0);

	  while (! ignore)
	    {
	      while (isspace (*line))
		++line;

	      cp = first_unused;
	      while (*line != '\0' && *line != ',')
		*first_unused++ = *line++;

	      if (first_unused != cp)
		{
		  /* OK, we can have a regular entry or an include
		     request.  */
		  if (*line != '\0')
		    ++line;
		  *first_unused++ = '\0';

		  if (strncmp (cp, ":include:", 9) != 0)
		    {
		      if (room_left < (first_unused - cp) + sizeof (char *))
			goto no_more_room;
		      room_left -= (first_unused - cp) + sizeof (char *);

		      ++result->alias_members_len;
		    }
		  else
		    {
		      /* Oh well, we have to read the addressed file.  */
		      FILE *listfile;
		      char *old_line = NULL;

		      first_unused = cp;

		      listfile = fopen (&cp[9], "r");
		      /* If the file does not exist we simply ignore
			 the statement.  */
		      if (listfile != NULL
			  && (old_line = strdup (line)) != NULL)
			{
			  while (! feof (listfile))
			    {
			      first_unused[room_left - 1] = '\xff';
			      line = fgets_unlocked (first_unused, room_left,
						     listfile);
			      if (line == NULL)
				break;
			      if (first_unused[room_left - 1] != '\xff')
				{
				  free (old_line);
				  goto no_more_room;
				}

			      /* Parse the line.  */
			      cp = strpbrk (line, "#\n");
			      if (cp != NULL)
				*cp = '\0';

			      do
				{
				  while (isspace (*line))
				    ++line;

				  cp = first_unused;
				  while (*line != '\0' && *line != ',')
				    *first_unused++ = *line++;

				  if (*line != '\0')
				    ++line;

				  if (first_unused != cp)
				    {
				      *first_unused++ = '\0';
				      if (room_left < ((first_unused - cp)
						       + __alignof__ (char *)))
					{
					  free (old_line);
					  goto no_more_room;
					}
				      room_left -= ((first_unused - cp)
						    + __alignof__ (char *));
				      ++result->alias_members_len;
				    }
				}
			      while (*line != '\0');
			    }
			  fclose (listfile);

			  first_unused[room_left - 1] = '\0';
			  strncpy (first_unused, old_line, room_left);

			  free (old_line);
			  line = first_unused;

			  if (first_unused[room_left - 1] != '\0')
			    goto no_more_room;
			}
		    }
		}

	      if (*line == '\0')
		{
		  /* Get the next line.  But we must be careful.  We
		     must not read the whole line at once since it
		     might belong to the current alias.  Simply read
		     the first character.  If it is a white space we
		     have a continuation line.  Otherwise it is the
		     beginning of a new alias and we can push back the
		     just read character.  */
		  int ch;

		  ch = fgetc (stream);
		  if (ch == EOF || ch == '\n' || !isspace (ch))
		    {
		      size_t cnt;

		      /* Now prepare the return.  Provide string
			 pointers for the currently selected aliases.  */
		      if (ch != EOF)
			ungetc (ch, stream);

		      /* Adjust the pointer so it is aligned for
			 storing pointers.  */
		      first_unused += __alignof__ (char *) - 1;
		      first_unused -= ((first_unused - (char *) 0)
				       % __alignof__ (char *));
		      result->alias_members = (char **) first_unused;

		      /* Compute addresses of alias entry strings.  */
		      cp = result->alias_name;
		      for (cnt = 0; cnt < result->alias_members_len; ++cnt)
			{
			  cp = strchr (cp, '\0') + 1;
			  result->alias_members[cnt] = cp;
			}

		      status = (result->alias_members_len == 0
				? NSS_STATUS_RETURN : NSS_STATUS_SUCCESS);
		      break;
		    }

		  /* The just read character is a white space and so
		     can be ignored.  */
		  first_unused[room_left - 1] = '\xff';
		  line = fgets_unlocked (first_unused, room_left, stream);
		  if (first_unused[room_left - 1] != '\xff')
		    goto no_more_room;
		  cp = strpbrk (line, "#\n");
		  if (cp != NULL)
		    *cp = '\0';
		}
	    }
	}

      if (status != NSS_STATUS_NOTFOUND)
	/* We read something.  In any case break here.  */
	break;
    }

  return status;
}


enum nss_status
_nss_files_getaliasent_r (struct aliasent *result, char *buffer, size_t buflen,
			  int *errnop)
{
  /* Return next entry in host file.  */
  enum nss_status status = NSS_STATUS_SUCCESS;

  __libc_lock_lock (lock);

  /* Be prepared that the set*ent function was not called before.  */
  if (stream == NULL)
    status = internal_setent ();

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
	  result->alias_local = 1;

	  /* Read lines until we get a definite result.  */
	  do
	    status = get_next_alias (NULL, result, buffer, buflen, errnop);
	  while (status == NSS_STATUS_RETURN);

	  /* If we successfully read an entry remember this position.  */
	  if (status == NSS_STATUS_SUCCESS)
	    fgetpos (stream, &position);
	  else
	    last_use = nouse;
	}
    }

  __libc_lock_unlock (lock);

  return status;
}


enum nss_status
_nss_files_getaliasbyname_r (const char *name, struct aliasent *result,
			     char *buffer, size_t buflen, int *errnop)
{
  /* Return next entry in host file.  */
  enum nss_status status = NSS_STATUS_SUCCESS;

  if (name == NULL)
    {
      __set_errno (EINVAL);
      return NSS_STATUS_UNAVAIL;
    }

  __libc_lock_lock (lock);

  /* Open the stream or rest it.  */
  status = internal_setent ();
  last_use = getby;

  if (status == NSS_STATUS_SUCCESS)
    {
      result->alias_local = 1;

      /* Read lines until we get a definite result.  */
      do
	status = get_next_alias (name, result, buffer, buflen, errnop);
      while (status == NSS_STATUS_RETURN);
    }

  internal_endent ();

  __libc_lock_unlock (lock);

  return status;
}
