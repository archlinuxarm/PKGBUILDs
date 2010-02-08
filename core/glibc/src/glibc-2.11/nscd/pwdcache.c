/* Cache handling for passwd lookup.
   Copyright (C) 1998-2008, 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation; version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <alloca.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <libintl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <stackinfo.h>

#include "nscd.h"
#include "dbg_log.h"
#ifdef HAVE_SENDFILE
# include <kernel-features.h>
#endif

/* This is the standard reply in case the service is disabled.  */
static const pw_response_header disabled =
{
  .version = NSCD_VERSION,
  .found = -1,
  .pw_name_len = 0,
  .pw_passwd_len = 0,
  .pw_uid = -1,
  .pw_gid = -1,
  .pw_gecos_len = 0,
  .pw_dir_len = 0,
  .pw_shell_len = 0
};

/* This is the struct describing how to write this record.  */
const struct iovec pwd_iov_disabled =
{
  .iov_base = (void *) &disabled,
  .iov_len = sizeof (disabled)
};


/* This is the standard reply in case we haven't found the dataset.  */
static const pw_response_header notfound =
{
  .version = NSCD_VERSION,
  .found = 0,
  .pw_name_len = 0,
  .pw_passwd_len = 0,
  .pw_uid = -1,
  .pw_gid = -1,
  .pw_gecos_len = 0,
  .pw_dir_len = 0,
  .pw_shell_len = 0
};


static void
cache_addpw (struct database_dyn *db, int fd, request_header *req,
	     const void *key, struct passwd *pwd, uid_t owner,
	     struct hashentry *const he, struct datahead *dh, int errval)
{
  ssize_t total;
  ssize_t written;
  time_t t = time (NULL);

  /* We allocate all data in one memory block: the iov vector,
     the response header and the dataset itself.  */
  struct dataset
  {
    struct datahead head;
    pw_response_header resp;
    char strdata[0];
  } *dataset;

  assert (offsetof (struct dataset, resp) == offsetof (struct datahead, data));

  if (pwd == NULL)
    {
      if (he != NULL && errval == EAGAIN)
	{
	  /* If we have an old record available but cannot find one
	     now because the service is not available we keep the old
	     record and make sure it does not get removed.  */
	  if (reload_count != UINT_MAX && dh->nreloads == reload_count)
	    /* Do not reset the value if we never not reload the record.  */
	    dh->nreloads = reload_count - 1;

	  written = total = 0;
	}
      else
	{
	  /* We have no data.  This means we send the standard reply for this
	     case.  */
	  written = total = sizeof (notfound);

	  if (fd != -1)
	    written = TEMP_FAILURE_RETRY (send (fd, &notfound, total,
						MSG_NOSIGNAL));

	  dataset = mempool_alloc (db, sizeof (struct dataset) + req->key_len,
				   1);
	  /* If we cannot permanently store the result, so be it.  */
	  if (dataset != NULL)
	    {
	      dataset->head.allocsize = sizeof (struct dataset) + req->key_len;
	      dataset->head.recsize = total;
	      dataset->head.notfound = true;
	      dataset->head.nreloads = 0;
	      dataset->head.usable = true;

	      /* Compute the timeout time.  */
	      dataset->head.timeout = t + db->negtimeout;

	      /* This is the reply.  */
	      memcpy (&dataset->resp, &notfound, total);

	      /* Copy the key data.  */
	      char *key_copy = memcpy (dataset->strdata, key, req->key_len);

	      /* If necessary, we also propagate the data to disk.  */
	      if (db->persistent)
		{
		  // XXX async OK?
		  uintptr_t pval = (uintptr_t) dataset & ~pagesize_m1;
		  msync ((void *) pval,
			 ((uintptr_t) dataset & pagesize_m1)
			 + sizeof (struct dataset) + req->key_len, MS_ASYNC);
		}

	      (void) cache_add (req->type, key_copy, req->key_len,
				&dataset->head, true, db, owner, he == NULL);

	      pthread_rwlock_unlock (&db->lock);

	      /* Mark the old entry as obsolete.  */
	      if (dh != NULL)
		dh->usable = false;
	    }
	}
    }
  else
    {
      /* Determine the I/O structure.  */
      size_t pw_name_len = strlen (pwd->pw_name) + 1;
      size_t pw_passwd_len = strlen (pwd->pw_passwd) + 1;
      size_t pw_gecos_len = strlen (pwd->pw_gecos) + 1;
      size_t pw_dir_len = strlen (pwd->pw_dir) + 1;
      size_t pw_shell_len = strlen (pwd->pw_shell) + 1;
      char *cp;
      const size_t key_len = strlen (key);
      const size_t buf_len = 3 * sizeof (pwd->pw_uid) + key_len + 1;
      char *buf = alloca (buf_len);
      ssize_t n;

      /* We need this to insert the `byuid' entry.  */
      int key_offset;
      n = snprintf (buf, buf_len, "%d%c%n%s", pwd->pw_uid, '\0',
		    &key_offset, (char *) key) + 1;

      written = total = (offsetof (struct dataset, strdata)
			 + pw_name_len + pw_passwd_len
			 + pw_gecos_len + pw_dir_len + pw_shell_len);

      /* If we refill the cache, first assume the reconrd did not
	 change.  Allocate memory on the cache since it is likely
	 discarded anyway.  If it turns out to be necessary to have a
	 new record we can still allocate real memory.  */
      bool alloca_used = false;
      dataset = NULL;

      if (he == NULL)
	dataset = (struct dataset *) mempool_alloc (db, total + n, 1);

      if (dataset == NULL)
	{
	  /* We cannot permanently add the result in the moment.  But
	     we can provide the result as is.  Store the data in some
	     temporary memory.  */
	  dataset = (struct dataset *) alloca (total + n);

	  /* We cannot add this record to the permanent database.  */
	  alloca_used = true;
	}

      dataset->head.allocsize = total + n;
      dataset->head.recsize = total - offsetof (struct dataset, resp);
      dataset->head.notfound = false;
      dataset->head.nreloads = he == NULL ? 0 : (dh->nreloads + 1);
      dataset->head.usable = true;

      /* Compute the timeout time.  */
      dataset->head.timeout = t + db->postimeout;

      dataset->resp.version = NSCD_VERSION;
      dataset->resp.found = 1;
      dataset->resp.pw_name_len = pw_name_len;
      dataset->resp.pw_passwd_len = pw_passwd_len;
      dataset->resp.pw_uid = pwd->pw_uid;
      dataset->resp.pw_gid = pwd->pw_gid;
      dataset->resp.pw_gecos_len = pw_gecos_len;
      dataset->resp.pw_dir_len = pw_dir_len;
      dataset->resp.pw_shell_len = pw_shell_len;

      cp = dataset->strdata;

      /* Copy the strings over into the buffer.  */
      cp = mempcpy (cp, pwd->pw_name, pw_name_len);
      cp = mempcpy (cp, pwd->pw_passwd, pw_passwd_len);
      cp = mempcpy (cp, pwd->pw_gecos, pw_gecos_len);
      cp = mempcpy (cp, pwd->pw_dir, pw_dir_len);
      cp = mempcpy (cp, pwd->pw_shell, pw_shell_len);

      /* Finally the stringified UID value.  */
      memcpy (cp, buf, n);
      char *key_copy = cp + key_offset;
      assert (key_copy == (char *) rawmemchr (cp, '\0') + 1);

      assert (cp == dataset->strdata + total - offsetof (struct dataset,
							 strdata));

      /* Now we can determine whether on refill we have to create a new
	 record or not.  */
      if (he != NULL)
	{
	  assert (fd == -1);

	  if (dataset->head.allocsize == dh->allocsize
	      && dataset->head.recsize == dh->recsize
	      && memcmp (&dataset->resp, dh->data,
			 dh->allocsize - offsetof (struct dataset, resp)) == 0)
	    {
	      /* The data has not changed.  We will just bump the
		 timeout value.  Note that the new record has been
		 allocated on the stack and need not be freed.  */
	      dh->timeout = dataset->head.timeout;
	      ++dh->nreloads;
	    }
	  else
	    {
	      /* We have to create a new record.  Just allocate
		 appropriate memory and copy it.  */
	      struct dataset *newp
		= (struct dataset *) mempool_alloc (db, total + n, 1);
	      if (newp != NULL)
		{
		  /* Adjust pointer into the memory block.  */
		  cp = (char *) newp + (cp - (char *) dataset);
		  key_copy = (char *) newp + (key_copy - (char *) dataset);

		  dataset = memcpy (newp, dataset, total + n);
		  alloca_used = false;
		}

	      /* Mark the old record as obsolete.  */
	      dh->usable = false;
	    }
	}
      else
	{
	  /* We write the dataset before inserting it to the database
	     since while inserting this thread might block and so would
	     unnecessarily let the receiver wait.  */
	  assert (fd != -1);

#ifdef HAVE_SENDFILE
	  if (__builtin_expect (db->mmap_used, 1) && !alloca_used)
	    {
	      assert (db->wr_fd != -1);
	      assert ((char *) &dataset->resp > (char *) db->data);
	      assert ((char *) dataset - (char *) db->head
		      + total
		      <= (sizeof (struct database_pers_head)
                          + db->head->module * sizeof (ref_t)
                          + db->head->data_size));
	      written = sendfileall (fd, db->wr_fd,
				     (char *) &dataset->resp
				     - (char *) db->head, dataset->head.recsize );
# ifndef __ASSUME_SENDFILE
	      if (written == -1 && errno == ENOSYS)
		goto use_write;
# endif
	    }
	  else
# ifndef __ASSUME_SENDFILE
	  use_write:
# endif
#endif
	    written = writeall (fd, &dataset->resp, dataset->head.recsize);
	}


      /* Add the record to the database.  But only if it has not been
	 stored on the stack.  */
      if (! alloca_used)
	{
	  /* If necessary, we also propagate the data to disk.  */
	  if (db->persistent)
	    {
	      // XXX async OK?
	      uintptr_t pval = (uintptr_t) dataset & ~pagesize_m1;
	      msync ((void *) pval,
		     ((uintptr_t) dataset & pagesize_m1) + total + n,
		     MS_ASYNC);
	    }

	  /* NB: in the following code we always must add the entry
	     marked with FIRST first.  Otherwise we end up with
	     dangling "pointers" in case a latter hash entry cannot be
	     added.  */
	  bool first = true;

	  /* If the request was by UID, add that entry first.  */
	  if (req->type == GETPWBYUID)
	    {
	      if (cache_add (GETPWBYUID, cp, key_offset, &dataset->head, true,
			     db, owner, he == NULL) < 0)
		goto out;

	      first = false;
	    }
	  /* If the key is different from the name add a separate entry.  */
	  else if (strcmp (key_copy, dataset->strdata) != 0)
	    {
	      if (cache_add (GETPWBYNAME, key_copy, key_len + 1,
			     &dataset->head, true, db, owner, he == NULL) < 0)
		goto out;

	      first = false;
	    }

	  /* We have to add the value for both, byname and byuid.  */
	  if ((req->type == GETPWBYNAME || db->propagate)
	      && __builtin_expect (cache_add (GETPWBYNAME, dataset->strdata,
					      pw_name_len, &dataset->head,
					      first, db, owner, he == NULL)
				   == 0, 1))
	    {
	      if (req->type == GETPWBYNAME && db->propagate)
		(void) cache_add (GETPWBYUID, cp, key_offset, &dataset->head,
				  false, db, owner, false);
	    }

	out:
	  pthread_rwlock_unlock (&db->lock);
	}
    }

  if (__builtin_expect (written != total, 0) && debug_level > 0)
    {
      char buf[256];
      dbg_log (_("short write in %s: %s"),  __FUNCTION__,
	       strerror_r (errno, buf, sizeof (buf)));
    }
}


union keytype
{
  void *v;
  uid_t u;
};


static int
lookup (int type, union keytype key, struct passwd *resultbufp, char *buffer,
	size_t buflen, struct passwd **pwd)
{
  if (type == GETPWBYNAME)
    return __getpwnam_r (key.v, resultbufp, buffer, buflen, pwd);
  else
    return __getpwuid_r (key.u, resultbufp, buffer, buflen, pwd);
}


static void
addpwbyX (struct database_dyn *db, int fd, request_header *req,
	  union keytype key, const char *keystr, uid_t c_uid,
	  struct hashentry *he, struct datahead *dh)
{
  /* Search for the entry matching the key.  Please note that we don't
     look again in the table whether the dataset is now available.  We
     simply insert it.  It does not matter if it is in there twice.  The
     pruning function only will look at the timestamp.  */
  size_t buflen = 1024;
  char *buffer = (char *) alloca (buflen);
  struct passwd resultbuf;
  struct passwd *pwd;
  bool use_malloc = false;
  int errval = 0;

  if (__builtin_expect (debug_level > 0, 0))
    {
      if (he == NULL)
	dbg_log (_("Haven't found \"%s\" in password cache!"), keystr);
      else
	dbg_log (_("Reloading \"%s\" in password cache!"), keystr);
    }

  while (lookup (req->type, key, &resultbuf, buffer, buflen, &pwd) != 0
	 && (errval = errno) == ERANGE)
    {
      errno = 0;

      if (__builtin_expect (buflen > 32768, 0))
	{
	  char *old_buffer = buffer;
	  buflen *= 2;
	  buffer = (char *) realloc (use_malloc ? buffer : NULL, buflen);
	  if (buffer == NULL)
	    {
	      /* We ran out of memory.  We cannot do anything but
		 sending a negative response.  In reality this should
		 never happen.  */
	      pwd = NULL;
	      buffer = old_buffer;

	      /* We set the error to indicate this is (possibly) a
		 temporary error and that it does not mean the entry
		 is not available at all.  */
	      errval = EAGAIN;
	      break;
	    }
	  use_malloc = true;
	}
      else
	/* Allocate a new buffer on the stack.  If possible combine it
	   with the previously allocated buffer.  */
	buffer = (char *) extend_alloca (buffer, buflen, 2 * buflen);
    }

  /* Add the entry to the cache.  */
  cache_addpw (db, fd, req, keystr, pwd, c_uid, he, dh, errval);

  if (use_malloc)
    free (buffer);
}


void
addpwbyname (struct database_dyn *db, int fd, request_header *req,
	     void *key, uid_t c_uid)
{
  union keytype u = { .v = key };

  addpwbyX (db, fd, req, u, key, c_uid, NULL, NULL);
}


void
readdpwbyname (struct database_dyn *db, struct hashentry *he,
	       struct datahead *dh)
{
  request_header req =
    {
      .type = GETPWBYNAME,
      .key_len = he->len
    };
  union keytype u = { .v = db->data + he->key };

  addpwbyX (db, -1, &req, u, db->data + he->key, he->owner, he, dh);
}


void
addpwbyuid (struct database_dyn *db, int fd, request_header *req,
	    void *key, uid_t c_uid)
{
  char *ep;
  uid_t uid = strtoul ((char *) key, &ep, 10);

  if (*(char *) key == '\0' || *ep != '\0')  /* invalid numeric uid */
    {
      if (debug_level > 0)
        dbg_log (_("Invalid numeric uid \"%s\"!"), (char *) key);

      errno = EINVAL;
      return;
    }

  union keytype u = { .u = uid };

  addpwbyX (db, fd, req, u, key, c_uid, NULL, NULL);
}


void
readdpwbyuid (struct database_dyn *db, struct hashentry *he,
	      struct datahead *dh)
{
  char *ep;
  uid_t uid = strtoul (db->data + he->key, &ep, 10);

  /* Since the key has been added before it must be OK.  */
  assert (*(db->data + he->key) != '\0' && *ep == '\0');

  request_header req =
    {
      .type = GETPWBYUID,
      .key_len = he->len
    };
  union keytype u = { .u = uid };

  addpwbyX (db, -1, &req, u, db->data + he->key, he->owner, he, dh);
}
