/* Inner loops of cache daemon.
   Copyright (C) 1998-2007, 2008, 2009 Free Software Foundation, Inc.
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
#include <atomic.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libintl.h>
#include <pthread.h>
#include <pwd.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#ifdef HAVE_EPOLL
# include <sys/epoll.h>
#endif
#ifdef HAVE_INOTIFY
# include <sys/inotify.h>
#endif
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/poll.h>
#ifdef HAVE_SENDFILE
# include <sys/sendfile.h>
#endif
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "nscd.h"
#include "dbg_log.h"
#include "selinux.h"
#include <resolv/resolv.h>
#ifdef HAVE_SENDFILE
# include <kernel-features.h>
#endif


/* Wrapper functions with error checking for standard functions.  */
extern void *xmalloc (size_t n);
extern void *xcalloc (size_t n, size_t s);
extern void *xrealloc (void *o, size_t n);

/* Support to run nscd as an unprivileged user */
const char *server_user;
static uid_t server_uid;
static gid_t server_gid;
const char *stat_user;
uid_t stat_uid;
static gid_t *server_groups;
#ifndef NGROUPS
# define NGROUPS 32
#endif
static int server_ngroups;

static pthread_attr_t attr;

static void begin_drop_privileges (void);
static void finish_drop_privileges (void);

/* Map request type to a string.  */
const char *const serv2str[LASTREQ] =
{
  [GETPWBYNAME] = "GETPWBYNAME",
  [GETPWBYUID] = "GETPWBYUID",
  [GETGRBYNAME] = "GETGRBYNAME",
  [GETGRBYGID] = "GETGRBYGID",
  [GETHOSTBYNAME] = "GETHOSTBYNAME",
  [GETHOSTBYNAMEv6] = "GETHOSTBYNAMEv6",
  [GETHOSTBYADDR] = "GETHOSTBYADDR",
  [GETHOSTBYADDRv6] = "GETHOSTBYADDRv6",
  [SHUTDOWN] = "SHUTDOWN",
  [GETSTAT] = "GETSTAT",
  [INVALIDATE] = "INVALIDATE",
  [GETFDPW] = "GETFDPW",
  [GETFDGR] = "GETFDGR",
  [GETFDHST] = "GETFDHST",
  [GETAI] = "GETAI",
  [INITGROUPS] = "INITGROUPS",
  [GETSERVBYNAME] = "GETSERVBYNAME",
  [GETSERVBYPORT] = "GETSERVBYPORT",
  [GETFDSERV] = "GETFDSERV"
};

/* The control data structures for the services.  */
struct database_dyn dbs[lastdb] =
{
  [pwddb] = {
    .lock = PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP,
    .prune_lock = PTHREAD_MUTEX_INITIALIZER,
    .prune_run_lock = PTHREAD_MUTEX_INITIALIZER,
    .enabled = 0,
    .check_file = 1,
    .persistent = 0,
    .propagate = 1,
    .shared = 0,
    .max_db_size = DEFAULT_MAX_DB_SIZE,
    .suggested_module = DEFAULT_SUGGESTED_MODULE,
   .reset_res = 0,
    .filename = "/etc/passwd",
    .db_filename = _PATH_NSCD_PASSWD_DB,
    .disabled_iov = &pwd_iov_disabled,
    .postimeout = 3600,
    .negtimeout = 20,
    .wr_fd = -1,
    .ro_fd = -1,
    .mmap_used = false
  },
  [grpdb] = {
    .lock = PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP,
    .prune_lock = PTHREAD_MUTEX_INITIALIZER,
    .prune_run_lock = PTHREAD_MUTEX_INITIALIZER,
    .enabled = 0,
    .check_file = 1,
    .persistent = 0,
    .propagate = 1,
    .shared = 0,
    .max_db_size = DEFAULT_MAX_DB_SIZE,
    .suggested_module = DEFAULT_SUGGESTED_MODULE,
    .reset_res = 0,
    .filename = "/etc/group",
    .db_filename = _PATH_NSCD_GROUP_DB,
    .disabled_iov = &grp_iov_disabled,
    .postimeout = 3600,
    .negtimeout = 60,
    .wr_fd = -1,
    .ro_fd = -1,
    .mmap_used = false
  },
  [hstdb] = {
    .lock = PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP,
    .prune_lock = PTHREAD_MUTEX_INITIALIZER,
    .prune_run_lock = PTHREAD_MUTEX_INITIALIZER,
    .enabled = 0,
    .check_file = 1,
    .persistent = 0,
    .propagate = 0,		/* Not used.  */
    .shared = 0,
    .max_db_size = DEFAULT_MAX_DB_SIZE,
    .suggested_module = DEFAULT_SUGGESTED_MODULE,
    .reset_res = 1,
    .filename = "/etc/hosts",
    .db_filename = _PATH_NSCD_HOSTS_DB,
    .disabled_iov = &hst_iov_disabled,
    .postimeout = 3600,
    .negtimeout = 20,
    .wr_fd = -1,
    .ro_fd = -1,
    .mmap_used = false
  },
  [servdb] = {
    .lock = PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP,
    .prune_lock = PTHREAD_MUTEX_INITIALIZER,
    .prune_run_lock = PTHREAD_MUTEX_INITIALIZER,
    .enabled = 0,
    .check_file = 1,
    .persistent = 0,
    .propagate = 0,		/* Not used.  */
    .shared = 0,
    .max_db_size = DEFAULT_MAX_DB_SIZE,
    .suggested_module = DEFAULT_SUGGESTED_MODULE,
    .reset_res = 0,
    .filename = "/etc/services",
    .db_filename = _PATH_NSCD_SERVICES_DB,
    .disabled_iov = &serv_iov_disabled,
    .postimeout = 28800,
    .negtimeout = 20,
    .wr_fd = -1,
    .ro_fd = -1,
    .mmap_used = false
  }
};


/* Mapping of request type to database.  */
static struct
{
  bool data_request;
  struct database_dyn *db;
} const reqinfo[LASTREQ] =
{
  [GETPWBYNAME] = { true, &dbs[pwddb] },
  [GETPWBYUID] = { true, &dbs[pwddb] },
  [GETGRBYNAME] = { true, &dbs[grpdb] },
  [GETGRBYGID] = { true, &dbs[grpdb] },
  [GETHOSTBYNAME] = { true, &dbs[hstdb] },
  [GETHOSTBYNAMEv6] = { true, &dbs[hstdb] },
  [GETHOSTBYADDR] = { true, &dbs[hstdb] },
  [GETHOSTBYADDRv6] = { true, &dbs[hstdb] },
  [SHUTDOWN] = { false, NULL },
  [GETSTAT] = { false, NULL },
  [SHUTDOWN] = { false, NULL },
  [GETFDPW] = { false, &dbs[pwddb] },
  [GETFDGR] = { false, &dbs[grpdb] },
  [GETFDHST] = { false, &dbs[hstdb] },
  [GETAI] = { true, &dbs[hstdb] },
  [INITGROUPS] = { true, &dbs[grpdb] },
  [GETSERVBYNAME] = { true, &dbs[servdb] },
  [GETSERVBYPORT] = { true, &dbs[servdb] },
  [GETFDSERV] = { false, &dbs[servdb] }
};


/* Initial number of threads to use.  */
int nthreads = -1;
/* Maximum number of threads to use.  */
int max_nthreads = 32;

/* Socket for incoming connections.  */
static int sock;

#ifdef HAVE_INOTIFY
/* Inotify descriptor.  */
static int inotify_fd = -1;

/* Watch descriptor for resolver configuration file.  */
static int resolv_conf_descr = -1;
#endif

#ifndef __ASSUME_SOCK_CLOEXEC
/* Negative if SOCK_CLOEXEC is not supported, positive if it is, zero
   before be know the result.  */
static int have_sock_cloexec;
#endif
#ifndef __ASSUME_ACCEPT4
static int have_accept4;
#endif

/* Number of times clients had to wait.  */
unsigned long int client_queued;


ssize_t
writeall (int fd, const void *buf, size_t len)
{
  size_t n = len;
  ssize_t ret;
  do
    {
      ret = TEMP_FAILURE_RETRY (send (fd, buf, n, MSG_NOSIGNAL));
      if (ret <= 0)
	break;
      buf = (const char *) buf + ret;
      n -= ret;
    }
  while (n > 0);
  return ret < 0 ? ret : len - n;
}


#ifdef HAVE_SENDFILE
ssize_t
sendfileall (int tofd, int fromfd, off_t off, size_t len)
{
  ssize_t n = len;
  ssize_t ret;

  do
    {
      ret = TEMP_FAILURE_RETRY (sendfile (tofd, fromfd, &off, n));
      if (ret <= 0)
	break;
      n -= ret;
    }
  while (n > 0);
  return ret < 0 ? ret : len - n;
}
#endif


enum usekey
  {
    use_not = 0,
    /* The following three are not really used, they are symbolic constants.  */
    use_first = 16,
    use_begin = 32,
    use_end = 64,

    use_he = 1,
    use_he_begin = use_he | use_begin,
    use_he_end = use_he | use_end,
#if SEPARATE_KEY
    use_key = 2,
    use_key_begin = use_key | use_begin,
    use_key_end = use_key | use_end,
    use_key_first = use_key_begin | use_first,
#endif
    use_data = 3,
    use_data_begin = use_data | use_begin,
    use_data_end = use_data | use_end,
    use_data_first = use_data_begin | use_first
  };


static int
check_use (const char *data, nscd_ssize_t first_free, uint8_t *usemap,
	   enum usekey use, ref_t start, size_t len)
{
  assert (len >= 2);

  if (start > first_free || start + len > first_free
      || (start & BLOCK_ALIGN_M1))
    return 0;

  if (usemap[start] == use_not)
    {
      /* Add the start marker.  */
      usemap[start] = use | use_begin;
      use &= ~use_first;

      while (--len > 0)
	if (usemap[++start] != use_not)
	  return 0;
	else
	  usemap[start] = use;

      /* Add the end marker.  */
      usemap[start] = use | use_end;
    }
  else if ((usemap[start] & ~use_first) == ((use | use_begin) & ~use_first))
    {
      /* Hash entries can't be shared.  */
      if (use == use_he)
	return 0;

      usemap[start] |= (use & use_first);
      use &= ~use_first;

      while (--len > 1)
	if (usemap[++start] != use)
	  return 0;

      if (usemap[++start] != (use | use_end))
	return 0;
    }
  else
    /* Points to a wrong object or somewhere in the middle.  */
    return 0;

  return 1;
}


/* Verify data in persistent database.  */
static int
verify_persistent_db (void *mem, struct database_pers_head *readhead, int dbnr)
{
  assert (dbnr == pwddb || dbnr == grpdb || dbnr == hstdb || dbnr == servdb);

  time_t now = time (NULL);

  struct database_pers_head *head = mem;
  struct database_pers_head head_copy = *head;

  /* Check that the header that was read matches the head in the database.  */
  if (memcmp (head, readhead, sizeof (*head)) != 0)
    return 0;

  /* First some easy tests: make sure the database header is sane.  */
  if (head->version != DB_VERSION
      || head->header_size != sizeof (*head)
      /* We allow a timestamp to be one hour ahead of the current time.
	 This should cover daylight saving time changes.  */
      || head->timestamp > now + 60 * 60 + 60
      || (head->gc_cycle & 1)
      || head->module == 0
      || (size_t) head->module > INT32_MAX / sizeof (ref_t)
      || (size_t) head->data_size > INT32_MAX - head->module * sizeof (ref_t)
      || head->first_free < 0
      || head->first_free > head->data_size
      || (head->first_free & BLOCK_ALIGN_M1) != 0
      || head->maxnentries < 0
      || head->maxnsearched < 0)
    return 0;

  uint8_t *usemap = calloc (head->first_free, 1);
  if (usemap == NULL)
    return 0;

  const char *data = (char *) &head->array[roundup (head->module,
						    ALIGN / sizeof (ref_t))];

  nscd_ssize_t he_cnt = 0;
  for (nscd_ssize_t cnt = 0; cnt < head->module; ++cnt)
    {
      ref_t trail = head->array[cnt];
      ref_t work = trail;
      int tick = 0;

      while (work != ENDREF)
	{
	  if (! check_use (data, head->first_free, usemap, use_he, work,
			   sizeof (struct hashentry)))
	    goto fail;

	  /* Now we know we can dereference the record.  */
	  struct hashentry *here = (struct hashentry *) (data + work);

	  ++he_cnt;

	  /* Make sure the record is for this type of service.  */
	  if (here->type >= LASTREQ
	      || reqinfo[here->type].db != &dbs[dbnr])
	    goto fail;

	  /* Validate boolean field value.  */
	  if (here->first != false && here->first != true)
	    goto fail;

	  if (here->len < 0)
	    goto fail;

	  /* Now the data.  */
	  if (here->packet < 0
	      || here->packet > head->first_free
	      || here->packet + sizeof (struct datahead) > head->first_free)
	    goto fail;

	  struct datahead *dh = (struct datahead *) (data + here->packet);

	  if (! check_use (data, head->first_free, usemap,
			   use_data | (here->first ? use_first : 0),
			   here->packet, dh->allocsize))
	    goto fail;

	  if (dh->allocsize < sizeof (struct datahead)
	      || dh->recsize > dh->allocsize
	      || (dh->notfound != false && dh->notfound != true)
	      || (dh->usable != false && dh->usable != true))
	    goto fail;

	  if (here->key < here->packet + sizeof (struct datahead)
	      || here->key > here->packet + dh->allocsize
	      || here->key + here->len > here->packet + dh->allocsize)
	    {
#if SEPARATE_KEY
	      /* If keys can appear outside of data, this should be done
		 instead.  But gc doesn't mark the data in that case.  */
	      if (! check_use (data, head->first_free, usemap,
			       use_key | (here->first ? use_first : 0),
			       here->key, here->len))
#endif
		goto fail;
	    }

	  work = here->next;

	  if (work == trail)
	    /* A circular list, this must not happen.  */
	    goto fail;
	  if (tick)
	    trail = ((struct hashentry *) (data + trail))->next;
	  tick = 1 - tick;
	}
    }

  if (he_cnt != head->nentries)
    goto fail;

  /* See if all data and keys had at least one reference from
     he->first == true hashentry.  */
  for (ref_t idx = 0; idx < head->first_free; ++idx)
    {
#if SEPARATE_KEY
      if (usemap[idx] == use_key_begin)
	goto fail;
#endif
      if (usemap[idx] == use_data_begin)
	goto fail;
    }

  /* Finally, make sure the database hasn't changed since the first test.  */
  if (memcmp (mem, &head_copy, sizeof (*head)) != 0)
    goto fail;

  free (usemap);
  return 1;

fail:
  free (usemap);
  return 0;
}


#ifdef O_CLOEXEC
# define EXTRA_O_FLAGS O_CLOEXEC
#else
# define EXTRA_O_FLAGS 0
#endif


/* Initialize database information structures.  */
void
nscd_init (void)
{
  /* Look up unprivileged uid/gid/groups before we start listening on the
     socket  */
  if (server_user != NULL)
    begin_drop_privileges ();

  if (nthreads == -1)
    /* No configuration for this value, assume a default.  */
    nthreads = 4;

#ifdef HAVE_INOTIFY
  /* Use inotify to recognize changed files.  */
  inotify_fd = inotify_init1 (IN_NONBLOCK);
# ifndef __ASSUME_IN_NONBLOCK
  if (inotify_fd == -1 && errno == ENOSYS)
    {
      inotify_fd = inotify_init ();
      if (inotify_fd != -1)
	fcntl (inotify_fd, F_SETFL, O_RDONLY | O_NONBLOCK);
    }
# endif
#endif

  for (size_t cnt = 0; cnt < lastdb; ++cnt)
    if (dbs[cnt].enabled)
      {
	pthread_rwlock_init (&dbs[cnt].lock, NULL);
	pthread_mutex_init (&dbs[cnt].memlock, NULL);

	if (dbs[cnt].persistent)
	  {
	    /* Try to open the appropriate file on disk.  */
	    int fd = open (dbs[cnt].db_filename, O_RDWR | EXTRA_O_FLAGS);
	    if (fd != -1)
	      {
		char *msg = NULL;
		struct stat64 st;
		void *mem;
		size_t total;
		struct database_pers_head head;
		ssize_t n = TEMP_FAILURE_RETRY (read (fd, &head,
						      sizeof (head)));
		if (n != sizeof (head) || fstat64 (fd, &st) != 0)
		  {
		  fail_db_errno:
		    /* The code is single-threaded at this point so
		       using strerror is just fine.  */
		    msg = strerror (errno);
		  fail_db:
		    dbg_log (_("invalid persistent database file \"%s\": %s"),
			     dbs[cnt].db_filename, msg);
		    unlink (dbs[cnt].db_filename);
		  }
		else if (head.module == 0 && head.data_size == 0)
		  {
		    /* The file has been created, but the head has not
		       been initialized yet.  */
		    msg = _("uninitialized header");
		    goto fail_db;
		  }
		else if (head.header_size != (int) sizeof (head))
		  {
		    msg = _("header size does not match");
		    goto fail_db;
		  }
		else if ((total = (sizeof (head)
				   + roundup (head.module * sizeof (ref_t),
					      ALIGN)
				   + head.data_size))
			 > st.st_size
			 || total < sizeof (head))
		  {
		    msg = _("file size does not match");
		    goto fail_db;
		  }
		/* Note we map with the maximum size allowed for the
		   database.  This is likely much larger than the
		   actual file size.  This is OK on most OSes since
		   extensions of the underlying file will
		   automatically translate more pages available for
		   memory access.  */
		else if ((mem = mmap (NULL, dbs[cnt].max_db_size,
				      PROT_READ | PROT_WRITE,
				      MAP_SHARED, fd, 0))
			 == MAP_FAILED)
		  goto fail_db_errno;
		else if (!verify_persistent_db (mem, &head, cnt))
		  {
		    munmap (mem, total);
		    msg = _("verification failed");
		    goto fail_db;
		  }
		else
		  {
		    /* Success.  We have the database.  */
		    dbs[cnt].head = mem;
		    dbs[cnt].memsize = total;
		    dbs[cnt].data = (char *)
		      &dbs[cnt].head->array[roundup (dbs[cnt].head->module,
						     ALIGN / sizeof (ref_t))];
		    dbs[cnt].mmap_used = true;

		    if (dbs[cnt].suggested_module > head.module)
		      dbg_log (_("suggested size of table for database %s larger than the persistent database's table"),
			       dbnames[cnt]);

		    dbs[cnt].wr_fd = fd;
		    fd = -1;
		    /* We also need a read-only descriptor.  */
		    if (dbs[cnt].shared)
		      {
			dbs[cnt].ro_fd = open (dbs[cnt].db_filename,
					       O_RDONLY | EXTRA_O_FLAGS);
			if (dbs[cnt].ro_fd == -1)
			  dbg_log (_("\
cannot create read-only descriptor for \"%s\"; no mmap"),
				   dbs[cnt].db_filename);
		      }

		    // XXX Shall we test whether the descriptors actually
		    // XXX point to the same file?
		  }

		/* Close the file descriptors in case something went
		   wrong in which case the variable have not been
		   assigned -1.  */
		if (fd != -1)
		  close (fd);
	      }
	    else if (errno == EACCES)
	      error (EXIT_FAILURE, 0, _("cannot access '%s'"),
		     dbs[cnt].db_filename);
	  }

	if (dbs[cnt].head == NULL)
	  {
	    /* No database loaded.  Allocate the data structure,
	       possibly on disk.  */
	    struct database_pers_head head;
	    size_t total = (sizeof (head)
			    + roundup (dbs[cnt].suggested_module
				       * sizeof (ref_t), ALIGN)
			    + (dbs[cnt].suggested_module
			       * DEFAULT_DATASIZE_PER_BUCKET));

	    /* Try to create the database.  If we do not need a
	       persistent database create a temporary file.  */
	    int fd;
	    int ro_fd = -1;
	    if (dbs[cnt].persistent)
	      {
		fd = open (dbs[cnt].db_filename,
			   O_RDWR | O_CREAT | O_EXCL | O_TRUNC | EXTRA_O_FLAGS,
			   S_IRUSR | S_IWUSR);
		if (fd != -1 && dbs[cnt].shared)
		  ro_fd = open (dbs[cnt].db_filename,
				O_RDONLY | EXTRA_O_FLAGS);
	      }
	    else
	      {
		char fname[] = _PATH_NSCD_XYZ_DB_TMP;
		fd = mkostemp (fname, EXTRA_O_FLAGS);

		/* We do not need the file name anymore after we
		   opened another file descriptor in read-only mode.  */
		if (fd != -1)
		  {
		    if (dbs[cnt].shared)
		      ro_fd = open (fname, O_RDONLY | EXTRA_O_FLAGS);

		    unlink (fname);
		  }
	      }

	    if (fd == -1)
	      {
		if (errno == EEXIST)
		  {
		    dbg_log (_("database for %s corrupted or simultaneously used; remove %s manually if necessary and restart"),
			     dbnames[cnt], dbs[cnt].db_filename);
		    // XXX Correct way to terminate?
		    exit (1);
		  }

		if  (dbs[cnt].persistent)
		  dbg_log (_("cannot create %s; no persistent database used"),
			   dbs[cnt].db_filename);
		else
		  dbg_log (_("cannot create %s; no sharing possible"),
			   dbs[cnt].db_filename);

		dbs[cnt].persistent = 0;
		// XXX remember: no mmap
	      }
	    else
	      {
		/* Tell the user if we could not create the read-only
		   descriptor.  */
		if (ro_fd == -1 && dbs[cnt].shared)
		  dbg_log (_("\
cannot create read-only descriptor for \"%s\"; no mmap"),
			   dbs[cnt].db_filename);

		/* Before we create the header, initialiye the hash
		   table.  So that if we get interrupted if writing
		   the header we can recognize a partially initialized
		   database.  */
		size_t ps = sysconf (_SC_PAGESIZE);
		char tmpbuf[ps];
		assert (~ENDREF == 0);
		memset (tmpbuf, '\xff', ps);

		size_t remaining = dbs[cnt].suggested_module * sizeof (ref_t);
		off_t offset = sizeof (head);

		size_t towrite;
		if (offset % ps != 0)
		  {
		    towrite = MIN (remaining, ps - (offset % ps));
		    if (pwrite (fd, tmpbuf, towrite, offset) != towrite)
		      goto write_fail;
		    offset += towrite;
		    remaining -= towrite;
		  }

		while (remaining > ps)
		  {
		    if (pwrite (fd, tmpbuf, ps, offset) == -1)
		      goto write_fail;
		    offset += ps;
		    remaining -= ps;
		  }

		if (remaining > 0
		    && pwrite (fd, tmpbuf, remaining, offset) != remaining)
		  goto write_fail;

		/* Create the header of the file.  */
		struct database_pers_head head =
		  {
		    .version = DB_VERSION,
		    .header_size = sizeof (head),
		    .module = dbs[cnt].suggested_module,
		    .data_size = (dbs[cnt].suggested_module
				  * DEFAULT_DATASIZE_PER_BUCKET),
		    .first_free = 0
		  };
		void *mem;

		if ((TEMP_FAILURE_RETRY (write (fd, &head, sizeof (head)))
		     != sizeof (head))
		    || (TEMP_FAILURE_RETRY_VAL (posix_fallocate (fd, 0, total))
			!= 0)
		    || (mem = mmap (NULL, dbs[cnt].max_db_size,
				    PROT_READ | PROT_WRITE,
				    MAP_SHARED, fd, 0)) == MAP_FAILED)
		  {
		  write_fail:
		    unlink (dbs[cnt].db_filename);
		    dbg_log (_("cannot write to database file %s: %s"),
			     dbs[cnt].db_filename, strerror (errno));
		    dbs[cnt].persistent = 0;
		  }
		else
		  {
		    /* Success.  */
		    dbs[cnt].head = mem;
		    dbs[cnt].data = (char *)
		      &dbs[cnt].head->array[roundup (dbs[cnt].head->module,
						     ALIGN / sizeof (ref_t))];
		    dbs[cnt].memsize = total;
		    dbs[cnt].mmap_used = true;

		    /* Remember the descriptors.  */
		    dbs[cnt].wr_fd = fd;
		    dbs[cnt].ro_fd = ro_fd;
		    fd = -1;
		    ro_fd = -1;
		  }

		if (fd != -1)
		  close (fd);
		if (ro_fd != -1)
		  close (ro_fd);
	      }
	  }

#if !defined O_CLOEXEC || !defined __ASSUME_O_CLOEXEC
	/* We do not check here whether the O_CLOEXEC provided to the
	   open call was successful or not.  The two fcntl calls are
	   only performed once each per process start-up and therefore
	   is not noticeable at all.  */
	if (paranoia
	    && ((dbs[cnt].wr_fd != -1
		 && fcntl (dbs[cnt].wr_fd, F_SETFD, FD_CLOEXEC) == -1)
		|| (dbs[cnt].ro_fd != -1
		    && fcntl (dbs[cnt].ro_fd, F_SETFD, FD_CLOEXEC) == -1)))
	  {
	    dbg_log (_("\
cannot set socket to close on exec: %s; disabling paranoia mode"),
		     strerror (errno));
	    paranoia = 0;
	  }
#endif

	if (dbs[cnt].head == NULL)
	  {
	    /* We do not use the persistent database.  Just
	       create an in-memory data structure.  */
	    assert (! dbs[cnt].persistent);

	    dbs[cnt].head = xmalloc (sizeof (struct database_pers_head)
				     + (dbs[cnt].suggested_module
					* sizeof (ref_t)));
	    memset (dbs[cnt].head, '\0', sizeof (struct database_pers_head));
	    assert (~ENDREF == 0);
	    memset (dbs[cnt].head->array, '\xff',
		    dbs[cnt].suggested_module * sizeof (ref_t));
	    dbs[cnt].head->module = dbs[cnt].suggested_module;
	    dbs[cnt].head->data_size = (DEFAULT_DATASIZE_PER_BUCKET
					* dbs[cnt].head->module);
	    dbs[cnt].data = xmalloc (dbs[cnt].head->data_size);
	    dbs[cnt].head->first_free = 0;

	    dbs[cnt].shared = 0;
	    assert (dbs[cnt].ro_fd == -1);
	  }

	dbs[cnt].inotify_descr = -1;
	if (dbs[cnt].check_file)
	  {
#ifdef HAVE_INOTIFY
	    if (inotify_fd < 0
		|| (dbs[cnt].inotify_descr
		    = inotify_add_watch (inotify_fd, dbs[cnt].filename,
					 IN_DELETE_SELF | IN_MODIFY)) < 0)
	      /* We cannot notice changes in the main thread.  */
#endif
	      {
		/* We need the modification date of the file.  */
		struct stat64 st;

		if (stat64 (dbs[cnt].filename, &st) < 0)
		  {
		    /* We cannot stat() the file, disable file checking.  */
		    dbg_log (_("cannot stat() file `%s': %s"),
			     dbs[cnt].filename, strerror (errno));
		    dbs[cnt].check_file = 0;
		  }
		else
		  dbs[cnt].file_mtime = st.st_mtime;
	      }
	  }

#ifdef HAVE_INOTIFY
	if (cnt == hstdb && inotify_fd >= -1)
	  /* We also monitor the resolver configuration file.  */
	  resolv_conf_descr = inotify_add_watch (inotify_fd,
						 _PATH_RESCONF,
						 IN_DELETE_SELF | IN_MODIFY);
#endif
      }

  /* Create the socket.  */
#ifndef __ASSUME_SOCK_CLOEXEC
  sock = -1;
  if (have_sock_cloexec >= 0)
#endif
    {
      sock = socket (AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
#ifndef __ASSUME_SOCK_CLOEXEC
      if (have_sock_cloexec == 0)
	have_sock_cloexec = sock != -1 || errno != EINVAL ? 1 : -1;
#endif
    }
#ifndef __ASSUME_SOCK_CLOEXEC
  if (have_sock_cloexec < 0)
    sock = socket (AF_UNIX, SOCK_STREAM, 0);
#endif
  if (sock < 0)
    {
      dbg_log (_("cannot open socket: %s"), strerror (errno));
      exit (errno == EACCES ? 4 : 1);
    }
  /* Bind a name to the socket.  */
  struct sockaddr_un sock_addr;
  sock_addr.sun_family = AF_UNIX;
  strcpy (sock_addr.sun_path, _PATH_NSCDSOCKET);
  if (bind (sock, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) < 0)
    {
      dbg_log ("%s: %s", _PATH_NSCDSOCKET, strerror (errno));
      exit (errno == EACCES ? 4 : 1);
    }

#ifndef __ASSUME_SOCK_CLOEXEC
  if (have_sock_cloexec < 0)
    {
      /* We don't want to get stuck on accept.  */
      int fl = fcntl (sock, F_GETFL);
      if (fl == -1 || fcntl (sock, F_SETFL, fl | O_NONBLOCK) == -1)
	{
	  dbg_log (_("cannot change socket to nonblocking mode: %s"),
		   strerror (errno));
	  exit (1);
	}

      /* The descriptor needs to be closed on exec.  */
      if (paranoia && fcntl (sock, F_SETFD, FD_CLOEXEC) == -1)
	{
	  dbg_log (_("cannot set socket to close on exec: %s"),
		   strerror (errno));
	  exit (1);
	}
    }
#endif

  /* Set permissions for the socket.  */
  chmod (_PATH_NSCDSOCKET, DEFFILEMODE);

  /* Set the socket up to accept connections.  */
  if (listen (sock, SOMAXCONN) < 0)
    {
      dbg_log (_("cannot enable socket to accept connections: %s"),
	       strerror (errno));
      exit (1);
    }

  /* Change to unprivileged uid/gid/groups if specifed in config file */
  if (server_user != NULL)
    finish_drop_privileges ();
}


/* Close the connections.  */
void
close_sockets (void)
{
  close (sock);
}


static void
invalidate_cache (char *key, int fd)
{
  dbtype number;
  int32_t resp;

  for (number = pwddb; number < lastdb; ++number)
    if (strcmp (key, dbnames[number]) == 0)
      {
	if (dbs[number].reset_res)
	  res_init ();

	break;
      }

  if (number == lastdb)
    {
      resp = EINVAL;
      writeall (fd, &resp, sizeof (resp));
      return;
    }

  if (dbs[number].enabled)
    {
      pthread_mutex_lock (&dbs[number].prune_run_lock);
      prune_cache (&dbs[number], LONG_MAX, fd);
      pthread_mutex_unlock (&dbs[number].prune_run_lock);
    }
  else
    {
      resp = 0;
      writeall (fd, &resp, sizeof (resp));
    }
}


#ifdef SCM_RIGHTS
static void
send_ro_fd (struct database_dyn *db, char *key, int fd)
{
  /* If we do not have an read-only file descriptor do nothing.  */
  if (db->ro_fd == -1)
    return;

  /* We need to send some data along with the descriptor.  */
  uint64_t mapsize = (db->head->data_size
		      + roundup (db->head->module * sizeof (ref_t), ALIGN)
		      + sizeof (struct database_pers_head));
  struct iovec iov[2];
  iov[0].iov_base = key;
  iov[0].iov_len = strlen (key) + 1;
  iov[1].iov_base = &mapsize;
  iov[1].iov_len = sizeof (mapsize);

  /* Prepare the control message to transfer the descriptor.  */
  union
  {
    struct cmsghdr hdr;
    char bytes[CMSG_SPACE (sizeof (int))];
  } buf;
  struct msghdr msg = { .msg_iov = iov, .msg_iovlen = 2,
			.msg_control = buf.bytes,
			.msg_controllen = sizeof (buf) };
  struct cmsghdr *cmsg = CMSG_FIRSTHDR (&msg);

  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN (sizeof (int));

  int *ip = (int *) CMSG_DATA (cmsg);
  *ip = db->ro_fd;

  msg.msg_controllen = cmsg->cmsg_len;

  /* Send the control message.  We repeat when we are interrupted but
     everything else is ignored.  */
#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif
  (void) TEMP_FAILURE_RETRY (sendmsg (fd, &msg, MSG_NOSIGNAL));

  if (__builtin_expect (debug_level > 0, 0))
    dbg_log (_("provide access to FD %d, for %s"), db->ro_fd, key);
}
#endif	/* SCM_RIGHTS */


/* Handle new request.  */
static void
handle_request (int fd, request_header *req, void *key, uid_t uid, pid_t pid)
{
  if (__builtin_expect (req->version, NSCD_VERSION) != NSCD_VERSION)
    {
      if (debug_level > 0)
	dbg_log (_("\
cannot handle old request version %d; current version is %d"),
		 req->version, NSCD_VERSION);
      return;
    }

  /* Perform the SELinux check before we go on to the standard checks.  */
  if (selinux_enabled && nscd_request_avc_has_perm (fd, req->type) != 0)
    {
      if (debug_level > 0)
	{
#ifdef SO_PEERCRED
# ifdef PATH_MAX
	  char buf[PATH_MAX];
# else
	  char buf[4096];
# endif

	  snprintf (buf, sizeof (buf), "/proc/%ld/exe", (long int) pid);
	  ssize_t n = readlink (buf, buf, sizeof (buf) - 1);

	  if (n <= 0)
	    dbg_log (_("\
request from %ld not handled due to missing permission"), (long int) pid);
	  else
	    {
	      buf[n] = '\0';
	      dbg_log (_("\
request from '%s' [%ld] not handled due to missing permission"),
		       buf, (long int) pid);
	    }
#else
	  dbg_log (_("request not handled due to missing permission"));
#endif
	}
      return;
    }

  struct database_dyn *db = reqinfo[req->type].db;

  /* See whether we can service the request from the cache.  */
  if (__builtin_expect (reqinfo[req->type].data_request, true))
    {
      if (__builtin_expect (debug_level, 0) > 0)
	{
	  if (req->type == GETHOSTBYADDR || req->type == GETHOSTBYADDRv6)
	    {
	      char buf[INET6_ADDRSTRLEN];

	      dbg_log ("\t%s (%s)", serv2str[req->type],
		       inet_ntop (req->type == GETHOSTBYADDR
				  ? AF_INET : AF_INET6,
				  key, buf, sizeof (buf)));
	    }
	  else
	    dbg_log ("\t%s (%s)", serv2str[req->type], (char *) key);
	}

      /* Is this service enabled?  */
      if (__builtin_expect (!db->enabled, 0))
	{
	  /* No, sent the prepared record.  */
	  if (TEMP_FAILURE_RETRY (send (fd, db->disabled_iov->iov_base,
					db->disabled_iov->iov_len,
					MSG_NOSIGNAL))
	      != (ssize_t) db->disabled_iov->iov_len
	      && __builtin_expect (debug_level, 0) > 0)
	    {
	      /* We have problems sending the result.  */
	      char buf[256];
	      dbg_log (_("cannot write result: %s"),
		       strerror_r (errno, buf, sizeof (buf)));
	    }

	  return;
	}

      /* Be sure we can read the data.  */
      if (__builtin_expect (pthread_rwlock_tryrdlock (&db->lock) != 0, 0))
	{
	  ++db->head->rdlockdelayed;
	  pthread_rwlock_rdlock (&db->lock);
	}

      /* See whether we can handle it from the cache.  */
      struct datahead *cached;
      cached = (struct datahead *) cache_search (req->type, key, req->key_len,
						 db, uid);
      if (cached != NULL)
	{
	  /* Hurray it's in the cache.  */
	  ssize_t nwritten;

#ifdef HAVE_SENDFILE
	  if (__builtin_expect (db->mmap_used, 1))
	    {
	      assert (db->wr_fd != -1);
	      assert ((char *) cached->data > (char *) db->data);
	      assert ((char *) cached->data - (char *) db->head
		      + cached->recsize
		      <= (sizeof (struct database_pers_head)
			  + db->head->module * sizeof (ref_t)
			  + db->head->data_size));
	      nwritten = sendfileall (fd, db->wr_fd,
				      (char *) cached->data
				      - (char *) db->head, cached->recsize);
# ifndef __ASSUME_SENDFILE
	      if (nwritten == -1 && errno == ENOSYS)
		goto use_write;
# endif
	    }
	  else
# ifndef __ASSUME_SENDFILE
	  use_write:
# endif
#endif
	    nwritten = writeall (fd, cached->data, cached->recsize);

	  if (nwritten != cached->recsize
	      && __builtin_expect (debug_level, 0) > 0)
	    {
	      /* We have problems sending the result.  */
	      char buf[256];
	      dbg_log (_("cannot write result: %s"),
		       strerror_r (errno, buf, sizeof (buf)));
	    }

	  pthread_rwlock_unlock (&db->lock);

	  return;
	}

      pthread_rwlock_unlock (&db->lock);
    }
  else if (__builtin_expect (debug_level, 0) > 0)
    {
      if (req->type == INVALIDATE)
	dbg_log ("\t%s (%s)", serv2str[req->type], (char *) key);
      else
	dbg_log ("\t%s", serv2str[req->type]);
    }

  /* Handle the request.  */
  switch (req->type)
    {
    case GETPWBYNAME:
      addpwbyname (db, fd, req, key, uid);
      break;

    case GETPWBYUID:
      addpwbyuid (db, fd, req, key, uid);
      break;

    case GETGRBYNAME:
      addgrbyname (db, fd, req, key, uid);
      break;

    case GETGRBYGID:
      addgrbygid (db, fd, req, key, uid);
      break;

    case GETHOSTBYNAME:
      addhstbyname (db, fd, req, key, uid);
      break;

    case GETHOSTBYNAMEv6:
      addhstbynamev6 (db, fd, req, key, uid);
      break;

    case GETHOSTBYADDR:
      addhstbyaddr (db, fd, req, key, uid);
      break;

    case GETHOSTBYADDRv6:
      addhstbyaddrv6 (db, fd, req, key, uid);
      break;

    case GETAI:
      addhstai (db, fd, req, key, uid);
      break;

    case INITGROUPS:
      addinitgroups (db, fd, req, key, uid);
      break;

    case GETSERVBYNAME:
      addservbyname (db, fd, req, key, uid);
      break;

    case GETSERVBYPORT:
      addservbyport (db, fd, req, key, uid);
      break;

    case GETSTAT:
    case SHUTDOWN:
    case INVALIDATE:
      {
	/* Get the callers credentials.  */
#ifdef SO_PEERCRED
	struct ucred caller;
	socklen_t optlen = sizeof (caller);

	if (getsockopt (fd, SOL_SOCKET, SO_PEERCRED, &caller, &optlen) < 0)
	  {
	    char buf[256];

	    dbg_log (_("error getting caller's id: %s"),
		     strerror_r (errno, buf, sizeof (buf)));
	    break;
	  }

	uid = caller.uid;
#else
	/* Some systems have no SO_PEERCRED implementation.  They don't
	   care about security so we don't as well.  */
	uid = 0;
#endif
      }

      /* Accept shutdown, getstat and invalidate only from root.  For
	 the stat call also allow the user specified in the config file.  */
      if (req->type == GETSTAT)
	{
	  if (uid == 0 || uid == stat_uid)
	    send_stats (fd, dbs);
	}
      else if (uid == 0)
	{
	  if (req->type == INVALIDATE)
	    invalidate_cache (key, fd);
	  else
	    termination_handler (0);
	}
      break;

    case GETFDPW:
    case GETFDGR:
    case GETFDHST:
    case GETFDSERV:
#ifdef SCM_RIGHTS
      send_ro_fd (reqinfo[req->type].db, key, fd);
#endif
      break;

    default:
      /* Ignore the command, it's nothing we know.  */
      break;
    }
}


/* Restart the process.  */
static void
restart (void)
{
  /* First determine the parameters.  We do not use the parameters
     passed to main() since in case nscd is started by running the
     dynamic linker this will not work.  Yes, this is not the usual
     case but nscd is part of glibc and we occasionally do this.  */
  size_t buflen = 1024;
  char *buf = alloca (buflen);
  size_t readlen = 0;
  int fd = open ("/proc/self/cmdline", O_RDONLY);
  if (fd == -1)
    {
      dbg_log (_("\
cannot open /proc/self/cmdline: %s; disabling paranoia mode"),
	       strerror (errno));

      paranoia = 0;
      return;
    }

  while (1)
    {
      ssize_t n = TEMP_FAILURE_RETRY (read (fd, buf + readlen,
					    buflen - readlen));
      if (n == -1)
	{
	  dbg_log (_("\
cannot read /proc/self/cmdline: %s; disabling paranoia mode"),
		   strerror (errno));

	  close (fd);
	  paranoia = 0;
	  return;
	}

      readlen += n;

      if (readlen < buflen)
	break;

      /* We might have to extend the buffer.  */
      size_t old_buflen = buflen;
      char *newp = extend_alloca (buf, buflen, 2 * buflen);
      buf = memmove (newp, buf, old_buflen);
    }

  close (fd);

  /* Parse the command line.  Worst case scenario: every two
     characters form one parameter (one character plus NUL).  */
  char **argv = alloca ((readlen / 2 + 1) * sizeof (argv[0]));
  int argc = 0;

  char *cp = buf;
  while (cp < buf + readlen)
    {
      argv[argc++] = cp;
      cp = (char *) rawmemchr (cp, '\0') + 1;
    }
  argv[argc] = NULL;

  /* Second, change back to the old user if we changed it.  */
  if (server_user != NULL)
    {
      if (setresuid (old_uid, old_uid, old_uid) != 0)
	{
	  dbg_log (_("\
cannot change to old UID: %s; disabling paranoia mode"),
		   strerror (errno));

	  paranoia = 0;
	  return;
	}

      if (setresgid (old_gid, old_gid, old_gid) != 0)
	{
	  dbg_log (_("\
cannot change to old GID: %s; disabling paranoia mode"),
		   strerror (errno));

	  setuid (server_uid);
	  paranoia = 0;
	  return;
	}
    }

  /* Next change back to the old working directory.  */
  if (chdir (oldcwd) == -1)
    {
      dbg_log (_("\
cannot change to old working directory: %s; disabling paranoia mode"),
	       strerror (errno));

      if (server_user != NULL)
	{
	  setuid (server_uid);
	  setgid (server_gid);
	}
      paranoia = 0;
      return;
    }

  /* Synchronize memory.  */
  int32_t certainly[lastdb];
  for (int cnt = 0; cnt < lastdb; ++cnt)
    if (dbs[cnt].enabled)
      {
	/* Make sure nobody keeps using the database.  */
	dbs[cnt].head->timestamp = 0;
	certainly[cnt] = dbs[cnt].head->nscd_certainly_running;
	dbs[cnt].head->nscd_certainly_running = 0;

	if (dbs[cnt].persistent)
	  // XXX async OK?
	  msync (dbs[cnt].head, dbs[cnt].memsize, MS_ASYNC);
      }

  /* The preparations are done.  */
#ifdef PATH_MAX
  char pathbuf[PATH_MAX];
#else
  char pathbuf[256];
#endif
  /* Try to exec the real nscd program so the process name (as reported
     in /proc/PID/status) will be 'nscd', but fall back to /proc/self/exe
     if readlink fails */
  ssize_t n = readlink ("/proc/self/exe", pathbuf, sizeof (pathbuf) - 1);
  if (n == -1)
    execv ("/proc/self/exe", argv);
  else
    {
      pathbuf[n] = '\0';
      execv (pathbuf, argv);
    }

  /* If we come here, we will never be able to re-exec.  */
  dbg_log (_("re-exec failed: %s; disabling paranoia mode"),
	   strerror (errno));

  if (server_user != NULL)
    {
      setuid (server_uid);
      setgid (server_gid);
    }
  if (chdir ("/") != 0)
    dbg_log (_("cannot change current working directory to \"/\": %s"),
	     strerror (errno));
  paranoia = 0;

  /* Reenable the databases.  */
  time_t now = time (NULL);
  for (int cnt = 0; cnt < lastdb; ++cnt)
    if (dbs[cnt].enabled)
      {
	dbs[cnt].head->timestamp = now;
	dbs[cnt].head->nscd_certainly_running = certainly[cnt];
      }
}


/* List of file descriptors.  */
struct fdlist
{
  int fd;
  struct fdlist *next;
};
/* Memory allocated for the list.  */
static struct fdlist *fdlist;
/* List of currently ready-to-read file descriptors.  */
static struct fdlist *readylist;

/* Conditional variable and mutex to signal availability of entries in
   READYLIST.  The condvar is initialized dynamically since we might
   use a different clock depending on availability.  */
static pthread_cond_t readylist_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t readylist_lock = PTHREAD_MUTEX_INITIALIZER;

/* The clock to use with the condvar.  */
static clockid_t timeout_clock = CLOCK_REALTIME;

/* Number of threads ready to handle the READYLIST.  */
static unsigned long int nready;


/* Function for the clean-up threads.  */
static void *
__attribute__ ((__noreturn__))
nscd_run_prune (void *p)
{
  const long int my_number = (long int) p;
  assert (dbs[my_number].enabled);

  int dont_need_update = setup_thread (&dbs[my_number]);

  time_t now = time (NULL);

  /* We are running.  */
  dbs[my_number].head->timestamp = now;

  struct timespec prune_ts;
  if (__builtin_expect (clock_gettime (timeout_clock, &prune_ts) == -1, 0))
    /* Should never happen.  */
    abort ();

  /* Compute the initial timeout time.  Prevent all the timers to go
     off at the same time by adding a db-based value.  */
  prune_ts.tv_sec += CACHE_PRUNE_INTERVAL + my_number;
  dbs[my_number].wakeup_time = now + CACHE_PRUNE_INTERVAL + my_number;

  pthread_mutex_t *prune_lock = &dbs[my_number].prune_lock;
  pthread_mutex_t *prune_run_lock = &dbs[my_number].prune_run_lock;
  pthread_cond_t *prune_cond = &dbs[my_number].prune_cond;

  pthread_mutex_lock (prune_lock);
  while (1)
    {
      /* Wait, but not forever.  */
      int e = 0;
      if (! dbs[my_number].clear_cache)
	e = pthread_cond_timedwait (prune_cond, prune_lock, &prune_ts);
      assert (__builtin_expect (e == 0 || e == ETIMEDOUT, 1));

      time_t next_wait;
      now = time (NULL);
      if (e == ETIMEDOUT || now >= dbs[my_number].wakeup_time
	  || dbs[my_number].clear_cache)
	{
	  /* We will determine the new timout values based on the
	     cache content.  Should there be concurrent additions to
	     the cache which are not accounted for in the cache
	     pruning we want to know about it.  Therefore set the
	     timeout to the maximum.  It will be descreased when adding
	     new entries to the cache, if necessary.  */
	  if (sizeof (time_t) == sizeof (long int))
	    dbs[my_number].wakeup_time = LONG_MAX;
	  else
	    dbs[my_number].wakeup_time = INT_MAX;

	  /* Unconditionally reset the flag.  */
	  time_t prune_now = dbs[my_number].clear_cache ? LONG_MAX : now;
	  dbs[my_number].clear_cache = 0;

	  pthread_mutex_unlock (prune_lock);

	  /* We use a separate lock for running the prune function (instead
	     of keeping prune_lock locked) because this enables concurrent
	     invocations of cache_add which might modify the timeout value.  */
	  pthread_mutex_lock (prune_run_lock);
	  next_wait = prune_cache (&dbs[my_number], prune_now, -1);
	  pthread_mutex_unlock (prune_run_lock);

	  next_wait = MAX (next_wait, CACHE_PRUNE_INTERVAL);
	  /* If clients cannot determine for sure whether nscd is running
	     we need to wake up occasionally to update the timestamp.
	     Wait 90% of the update period.  */
#define UPDATE_MAPPING_TIMEOUT (MAPPING_TIMEOUT * 9 / 10)
	  if (__builtin_expect (! dont_need_update, 0))
	    {
	      next_wait = MIN (UPDATE_MAPPING_TIMEOUT, next_wait);
	      dbs[my_number].head->timestamp = now;
	    }

	  pthread_mutex_lock (prune_lock);

	  /* Make it known when we will wake up again.  */
	  if (now + next_wait < dbs[my_number].wakeup_time)
	    dbs[my_number].wakeup_time = now + next_wait;
	  else
	    next_wait = dbs[my_number].wakeup_time - now;
	}
      else
	/* The cache was just pruned.  Do not do it again now.  Just
	   use the new timeout value.  */
	next_wait = dbs[my_number].wakeup_time - now;

      if (clock_gettime (timeout_clock, &prune_ts) == -1)
	/* Should never happen.  */
	abort ();

      /* Compute next timeout time.  */
      prune_ts.tv_sec += next_wait;
    }
}


/* This is the main loop.  It is replicated in different threads but
   the the use of the ready list makes sure only one thread handles an
   incoming connection.  */
static void *
__attribute__ ((__noreturn__))
nscd_run_worker (void *p)
{
  char buf[256];

  /* Initial locking.  */
  pthread_mutex_lock (&readylist_lock);

  /* One more thread available.  */
  ++nready;

  while (1)
    {
      while (readylist == NULL)
	pthread_cond_wait (&readylist_cond, &readylist_lock);

      struct fdlist *it = readylist->next;
      if (readylist->next == readylist)
	/* Just one entry on the list.  */
	readylist = NULL;
      else
	readylist->next = it->next;

      /* Extract the information and mark the record ready to be used
	 again.  */
      int fd = it->fd;
      it->next = NULL;

      /* One more thread available.  */
      --nready;

      /* We are done with the list.  */
      pthread_mutex_unlock (&readylist_lock);

#ifndef __ASSUME_ACCEPT4
      if (have_accept4 < 0)
	{
	  /* We do not want to block on a short read or so.  */
	  int fl = fcntl (fd, F_GETFL);
	  if (fl == -1 || fcntl (fd, F_SETFL, fl | O_NONBLOCK) == -1)
	    goto close_and_out;
	}
#endif

      /* Now read the request.  */
      request_header req;
      if (__builtin_expect (TEMP_FAILURE_RETRY (read (fd, &req, sizeof (req)))
			    != sizeof (req), 0))
	{
	  /* We failed to read data.  Note that this also might mean we
	     failed because we would have blocked.  */
	  if (debug_level > 0)
	    dbg_log (_("short read while reading request: %s"),
		     strerror_r (errno, buf, sizeof (buf)));
	  goto close_and_out;
	}

      /* Check whether this is a valid request type.  */
      if (req.type < GETPWBYNAME || req.type >= LASTREQ)
	goto close_and_out;

      /* Some systems have no SO_PEERCRED implementation.  They don't
	 care about security so we don't as well.  */
      uid_t uid = -1;
#ifdef SO_PEERCRED
      pid_t pid = 0;

      if (__builtin_expect (debug_level > 0, 0))
	{
	  struct ucred caller;
	  socklen_t optlen = sizeof (caller);

	  if (getsockopt (fd, SOL_SOCKET, SO_PEERCRED, &caller, &optlen) == 0)
	    pid = caller.pid;
	}
#else
      const pid_t pid = 0;
#endif

      /* It should not be possible to crash the nscd with a silly
	 request (i.e., a terribly large key).  We limit the size to 1kb.  */
      if (__builtin_expect (req.key_len, 1) < 0
	  || __builtin_expect (req.key_len, 1) > MAXKEYLEN)
	{
	  if (debug_level > 0)
	    dbg_log (_("key length in request too long: %d"), req.key_len);
	}
      else
	{
	  /* Get the key.  */
	  char keybuf[MAXKEYLEN];

	  if (__builtin_expect (TEMP_FAILURE_RETRY (read (fd, keybuf,
							  req.key_len))
				!= req.key_len, 0))
	    {
	      /* Again, this can also mean we would have blocked.  */
	      if (debug_level > 0)
		dbg_log (_("short read while reading request key: %s"),
			 strerror_r (errno, buf, sizeof (buf)));
	      goto close_and_out;
	    }

	  if (__builtin_expect (debug_level, 0) > 0)
	    {
#ifdef SO_PEERCRED
	      if (pid != 0)
		dbg_log (_("\
handle_request: request received (Version = %d) from PID %ld"),
			 req.version, (long int) pid);
	      else
#endif
		dbg_log (_("\
handle_request: request received (Version = %d)"), req.version);
	    }

	  /* Phew, we got all the data, now process it.  */
	  handle_request (fd, &req, keybuf, uid, pid);
	}

    close_and_out:
      /* We are done.  */
      close (fd);

      /* Re-locking.  */
      pthread_mutex_lock (&readylist_lock);

      /* One more thread available.  */
      ++nready;
    }
  /* NOTREACHED */
}


static unsigned int nconns;

static void
fd_ready (int fd)
{
  pthread_mutex_lock (&readylist_lock);

  /* Find an empty entry in FDLIST.  */
  size_t inner;
  for (inner = 0; inner < nconns; ++inner)
    if (fdlist[inner].next == NULL)
      break;
  assert (inner < nconns);

  fdlist[inner].fd = fd;

  if (readylist == NULL)
    readylist = fdlist[inner].next = &fdlist[inner];
  else
    {
      fdlist[inner].next = readylist->next;
      readylist = readylist->next = &fdlist[inner];
    }

  bool do_signal = true;
  if (__builtin_expect (nready == 0, 0))
    {
      ++client_queued;
      do_signal = false;

      /* Try to start another thread to help out.  */
      pthread_t th;
      if (nthreads < max_nthreads
	  && pthread_create (&th, &attr, nscd_run_worker,
			     (void *) (long int) nthreads) == 0)
	{
	  /* We got another thread.  */
	  ++nthreads;
	  /* The new thread might need a kick.  */
	  do_signal = true;
	}

    }

  pthread_mutex_unlock (&readylist_lock);

  /* Tell one of the worker threads there is work to do.  */
  if (do_signal)
    pthread_cond_signal (&readylist_cond);
}


/* Check whether restarting should happen.  */
static inline int
restart_p (time_t now)
{
  return (paranoia && readylist == NULL && nready == nthreads
	  && now >= restart_time);
}


/* Array for times a connection was accepted.  */
static time_t *starttime;


static void
__attribute__ ((__noreturn__))
main_loop_poll (void)
{
  struct pollfd *conns = (struct pollfd *) xmalloc (nconns
						    * sizeof (conns[0]));

  conns[0].fd = sock;
  conns[0].events = POLLRDNORM;
  size_t nused = 1;
  size_t firstfree = 1;

#ifdef HAVE_INOTIFY
  if (inotify_fd != -1)
    {
      conns[1].fd = inotify_fd;
      conns[1].events = POLLRDNORM;
      nused = 2;
      firstfree = 2;
    }
#endif

  while (1)
    {
      /* Wait for any event.  We wait at most a couple of seconds so
	 that we can check whether we should close any of the accepted
	 connections since we have not received a request.  */
#define MAX_ACCEPT_TIMEOUT 30
#define MIN_ACCEPT_TIMEOUT 5
#define MAIN_THREAD_TIMEOUT \
  (MAX_ACCEPT_TIMEOUT * 1000						      \
   - ((MAX_ACCEPT_TIMEOUT - MIN_ACCEPT_TIMEOUT) * 1000 * nused) / (2 * nconns))

      int n = poll (conns, nused, MAIN_THREAD_TIMEOUT);

      time_t now = time (NULL);

      /* If there is a descriptor ready for reading or there is a new
	 connection, process this now.  */
      if (n > 0)
	{
	  if (conns[0].revents != 0)
	    {
	      /* We have a new incoming connection.  Accept the connection.  */
	      int fd;

#ifndef __ASSUME_ACCEPT4
	      fd = -1;
	      if (have_accept4 >= 0)
#endif
		{
		  fd = TEMP_FAILURE_RETRY (accept4 (sock, NULL, NULL,
						    SOCK_NONBLOCK));
#ifndef __ASSUME_ACCEPT4
		  if (have_accept4 == 0)
		    have_accept4 = fd != -1 || errno != ENOSYS ? 1 : -1;
#endif
		}
#ifndef __ASSUME_ACCEPT4
	      if (have_accept4 < 0)
		fd = TEMP_FAILURE_RETRY (accept (sock, NULL, NULL));
#endif

	      /* Use the descriptor if we have not reached the limit.  */
	      if (fd >= 0)
		{
		  if (firstfree < nconns)
		    {
		      conns[firstfree].fd = fd;
		      conns[firstfree].events = POLLRDNORM;
		      starttime[firstfree] = now;
		      if (firstfree >= nused)
			nused = firstfree + 1;

		      do
			++firstfree;
		      while (firstfree < nused && conns[firstfree].fd != -1);
		    }
		  else
		    /* We cannot use the connection so close it.  */
		    close (fd);
		}

	      --n;
	    }

	  size_t first = 1;
#ifdef HAVE_INOTIFY
	  if (inotify_fd != -1 && conns[1].fd == inotify_fd)
	    {
	      if (conns[1].revents != 0)
		{
		  bool to_clear[lastdb] = { false, };
		  union
		  {
# ifndef PATH_MAX
#  define PATH_MAX 1024
# endif
		    struct inotify_event i;
		    char buf[sizeof (struct inotify_event) + PATH_MAX];
		  } inev;

		  while (1)
		    {
		      ssize_t nb = TEMP_FAILURE_RETRY (read (inotify_fd, &inev,
							     sizeof (inev)));
		      if (nb < (ssize_t) sizeof (struct inotify_event))
			{
			  if (__builtin_expect (nb == -1 && errno != EAGAIN,
						0))
			    {
			      /* Something went wrong when reading the inotify
				 data.  Better disable inotify.  */
			      dbg_log (_("\
disabled inotify after read error %d"),
				       errno);
			      conns[1].fd = -1;
			      firstfree = 1;
			      if (nused == 2)
				nused = 1;
			      close (inotify_fd);
			      inotify_fd = -1;
			    }
			  break;
			}

		      /* Check which of the files changed.  */
		      for (size_t dbcnt = 0; dbcnt < lastdb; ++dbcnt)
			if (inev.i.wd == dbs[dbcnt].inotify_descr)
			  {
			    to_clear[dbcnt] = true;
			    goto next;
			  }

		      if (inev.i.wd == resolv_conf_descr)
			{
			  res_init ();
			  to_clear[hstdb] = true;
			}
		    next:;
		    }

		  /* Actually perform the cache clearing.  */
		  for (size_t dbcnt = 0; dbcnt < lastdb; ++dbcnt)
		    if (to_clear[dbcnt])
		      {
			pthread_mutex_lock (&dbs[dbcnt].prune_lock);
			dbs[dbcnt].clear_cache = 1;
			pthread_mutex_unlock (&dbs[dbcnt].prune_lock);
			pthread_cond_signal (&dbs[dbcnt].prune_cond);
		      }

		  --n;
		}

	      first = 2;
	    }
#endif

	  for (size_t cnt = first; cnt < nused && n > 0; ++cnt)
	    if (conns[cnt].revents != 0)
	      {
		fd_ready (conns[cnt].fd);

		/* Clean up the CONNS array.  */
		conns[cnt].fd = -1;
		if (cnt < firstfree)
		  firstfree = cnt;
		if (cnt == nused - 1)
		  do
		    --nused;
		  while (conns[nused - 1].fd == -1);

		--n;
	      }
	}

      /* Now find entries which have timed out.  */
      assert (nused > 0);

      /* We make the timeout length depend on the number of file
	 descriptors currently used.  */
#define ACCEPT_TIMEOUT \
  (MAX_ACCEPT_TIMEOUT							      \
   - ((MAX_ACCEPT_TIMEOUT - MIN_ACCEPT_TIMEOUT) * nused) / nconns)
      time_t laststart = now - ACCEPT_TIMEOUT;

      for (size_t cnt = nused - 1; cnt > 0; --cnt)
	{
	  if (conns[cnt].fd != -1 && starttime[cnt] < laststart)
	    {
	      /* Remove the entry, it timed out.  */
	      (void) close (conns[cnt].fd);
	      conns[cnt].fd = -1;

	      if (cnt < firstfree)
		firstfree = cnt;
	      if (cnt == nused - 1)
		do
		  --nused;
		while (conns[nused - 1].fd == -1);
	    }
	}

      if (restart_p (now))
	restart ();
    }
}


#ifdef HAVE_EPOLL
static void
main_loop_epoll (int efd)
{
  struct epoll_event ev = { 0, };
  int nused = 1;
  size_t highest = 0;

  /* Add the socket.  */
  ev.events = EPOLLRDNORM;
  ev.data.fd = sock;
  if (epoll_ctl (efd, EPOLL_CTL_ADD, sock, &ev) == -1)
    /* We cannot use epoll.  */
    return;

# ifdef HAVE_INOTIFY
  if (inotify_fd != -1)
    {
      ev.events = EPOLLRDNORM;
      ev.data.fd = inotify_fd;
      if (epoll_ctl (efd, EPOLL_CTL_ADD, inotify_fd, &ev) == -1)
	/* We cannot use epoll.  */
	return;
      nused = 2;
    }
# endif

  while (1)
    {
      struct epoll_event revs[100];
# define nrevs (sizeof (revs) / sizeof (revs[0]))

      int n = epoll_wait (efd, revs, nrevs, MAIN_THREAD_TIMEOUT);

      time_t now = time (NULL);

      for (int cnt = 0; cnt < n; ++cnt)
	if (revs[cnt].data.fd == sock)
	  {
	    /* A new connection.  */
	    int fd;

# ifndef __ASSUME_ACCEPT4
	    fd = -1;
	    if (have_accept4 >= 0)
# endif
	      {
		fd = TEMP_FAILURE_RETRY (accept4 (sock, NULL, NULL,
						  SOCK_NONBLOCK));
# ifndef __ASSUME_ACCEPT4
		if (have_accept4 == 0)
		  have_accept4 = fd != -1 || errno != ENOSYS ? 1 : -1;
# endif
	      }
# ifndef __ASSUME_ACCEPT4
	    if (have_accept4 < 0)
	      fd = TEMP_FAILURE_RETRY (accept (sock, NULL, NULL));
# endif

	    /* Use the descriptor if we have not reached the limit.  */
	    if (fd >= 0)
	      {
		/* Try to add the  new descriptor.  */
		ev.data.fd = fd;
		if (fd >= nconns
		    || epoll_ctl (efd, EPOLL_CTL_ADD, fd, &ev) == -1)
		  /* The descriptor is too large or something went
		     wrong.  Close the descriptor.  */
		  close (fd);
		else
		  {
		    /* Remember when we accepted the connection.  */
		    starttime[fd] = now;

		    if (fd > highest)
		      highest = fd;

		    ++nused;
		  }
	      }
	  }
# ifdef HAVE_INOTIFY
	else if (revs[cnt].data.fd == inotify_fd)
	  {
	    bool to_clear[lastdb] = { false, };
	    union
	    {
	      struct inotify_event i;
	      char buf[sizeof (struct inotify_event) + PATH_MAX];
	    } inev;

	    while (1)
	      {
		ssize_t nb = TEMP_FAILURE_RETRY (read (inotify_fd, &inev,
				 		 sizeof (inev)));
		if (nb < (ssize_t) sizeof (struct inotify_event))
		  {
		    if (__builtin_expect (nb == -1 && errno != EAGAIN, 0))
		      {
			/* Something went wrong when reading the inotify
			   data.  Better disable inotify.  */
			dbg_log (_("disabled inotify after read error %d"),
				 errno);
			(void) epoll_ctl (efd, EPOLL_CTL_DEL, inotify_fd,
					  NULL);
			close (inotify_fd);
			inotify_fd = -1;
		      }
		    break;
		  }

		/* Check which of the files changed.  */
		for (size_t dbcnt = 0; dbcnt < lastdb; ++dbcnt)
		  if (inev.i.wd == dbs[dbcnt].inotify_descr)
		    {
		      to_clear[dbcnt] = true;
		      goto next;
		    }

		if (inev.i.wd == resolv_conf_descr)
		  {
		    res_init ();
		    to_clear[hstdb] = true;
		  }
	      next:;
	      }

	    /* Actually perform the cache clearing.  */
	    for (size_t dbcnt = 0; dbcnt < lastdb; ++dbcnt)
	      if (to_clear[dbcnt])
		{
		  pthread_mutex_lock (&dbs[dbcnt].prune_lock);
		  dbs[dbcnt].clear_cache = 1;
		  pthread_mutex_unlock (&dbs[dbcnt].prune_lock);
		  pthread_cond_signal (&dbs[dbcnt].prune_cond);
		}
	  }
# endif
	else
	  {
	    /* Remove the descriptor from the epoll descriptor.  */
	    (void) epoll_ctl (efd, EPOLL_CTL_DEL, revs[cnt].data.fd, NULL);

	    /* Get a worker to handle the request.  */
	    fd_ready (revs[cnt].data.fd);

	    /* Reset the time.  */
	    starttime[revs[cnt].data.fd] = 0;
	    if (revs[cnt].data.fd == highest)
	      do
		--highest;
	      while (highest > 0 && starttime[highest] == 0);

	    --nused;
	  }

      /*  Now look for descriptors for accepted connections which have
	  no reply in too long of a time.  */
      time_t laststart = now - ACCEPT_TIMEOUT;
      assert (starttime[sock] == 0);
      assert (inotify_fd == -1 || starttime[inotify_fd] == 0);
      for (int cnt = highest; cnt > STDERR_FILENO; --cnt)
	if (starttime[cnt] != 0 && starttime[cnt] < laststart)
	  {
	    /* We are waiting for this one for too long.  Close it.  */
	    (void) epoll_ctl (efd, EPOLL_CTL_DEL, cnt, NULL);

	    (void) close (cnt);

	    starttime[cnt] = 0;
	    if (cnt == highest)
	      --highest;
	  }
	else if (cnt != sock && starttime[cnt] == 0 && cnt == highest)
	  --highest;

      if (restart_p (now))
	restart ();
    }
}
#endif


/* Start all the threads we want.  The initial process is thread no. 1.  */
void
start_threads (void)
{
  /* Initialize the conditional variable we will use.  The only
     non-standard attribute we might use is the clock selection.  */
  pthread_condattr_t condattr;
  pthread_condattr_init (&condattr);

#if defined _POSIX_CLOCK_SELECTION && _POSIX_CLOCK_SELECTION >= 0 \
    && defined _POSIX_MONOTONIC_CLOCK && _POSIX_MONOTONIC_CLOCK >= 0
  /* Determine whether the monotonous clock is available.  */
  struct timespec dummy;
# if _POSIX_MONOTONIC_CLOCK == 0
  if (sysconf (_SC_MONOTONIC_CLOCK) > 0)
# endif
# if _POSIX_CLOCK_SELECTION == 0
    if (sysconf (_SC_CLOCK_SELECTION) > 0)
# endif
      if (clock_getres (CLOCK_MONOTONIC, &dummy) == 0
	  && pthread_condattr_setclock (&condattr, CLOCK_MONOTONIC) == 0)
	timeout_clock = CLOCK_MONOTONIC;
#endif

  /* Create the attribute for the threads.  They are all created
     detached.  */
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  /* Use 1MB stacks, twice as much for 64-bit architectures.  */
  pthread_attr_setstacksize (&attr, NSCD_THREAD_STACKSIZE);

  /* We allow less than LASTDB threads only for debugging.  */
  if (debug_level == 0)
    nthreads = MAX (nthreads, lastdb);

  /* Create the threads which prune the databases.  */
  // XXX Ideally this work would be done by some of the worker threads.
  // XXX But this is problematic since we would need to be able to wake
  // XXX them up explicitly as well as part of the group handling the
  // XXX ready-list.  This requires an operation where we can wait on
  // XXX two conditional variables at the same time.  This operation
  // XXX does not exist (yet).
  for (long int i = 0; i < lastdb; ++i)
    {
      /* Initialize the conditional variable.  */
      if (pthread_cond_init (&dbs[i].prune_cond, &condattr) != 0)
	{
	  dbg_log (_("could not initialize conditional variable"));
	  exit (1);
	}

      pthread_t th;
      if (dbs[i].enabled
	  && pthread_create (&th, &attr, nscd_run_prune, (void *) i) != 0)
	{
	  dbg_log (_("could not start clean-up thread; terminating"));
	  exit (1);
	}
    }

  pthread_condattr_destroy (&condattr);

  for (long int i = 0; i < nthreads; ++i)
    {
      pthread_t th;
      if (pthread_create (&th, &attr, nscd_run_worker, NULL) != 0)
	{
	  if (i == 0)
	    {
	      dbg_log (_("could not start any worker thread; terminating"));
	      exit (1);
	    }

	  break;
	}
    }

  /* Determine how much room for descriptors we should initially
     allocate.  This might need to change later if we cap the number
     with MAXCONN.  */
  const long int nfds = sysconf (_SC_OPEN_MAX);
#define MINCONN 32
#define MAXCONN 16384
  if (nfds == -1 || nfds > MAXCONN)
    nconns = MAXCONN;
  else if (nfds < MINCONN)
    nconns = MINCONN;
  else
    nconns = nfds;

  /* We need memory to pass descriptors on to the worker threads.  */
  fdlist = (struct fdlist *) xcalloc (nconns, sizeof (fdlist[0]));
  /* Array to keep track when connection was accepted.  */
  starttime = (time_t *) xcalloc (nconns, sizeof (starttime[0]));

  /* In the main thread we execute the loop which handles incoming
     connections.  */
#ifdef HAVE_EPOLL
  int efd = epoll_create (100);
  if (efd != -1)
    {
      main_loop_epoll (efd);
      close (efd);
    }
#endif

  main_loop_poll ();
}


/* Look up the uid, gid, and supplementary groups to run nscd as. When
   this function is called, we are not listening on the nscd socket yet so
   we can just use the ordinary lookup functions without causing a lockup  */
static void
begin_drop_privileges (void)
{
  struct passwd *pwd = getpwnam (server_user);

  if (pwd == NULL)
    {
      dbg_log (_("Failed to run nscd as user '%s'"), server_user);
      error (EXIT_FAILURE, 0, _("Failed to run nscd as user '%s'"),
	     server_user);
    }

  server_uid = pwd->pw_uid;
  server_gid = pwd->pw_gid;

  /* Save the old UID/GID if we have to change back.  */
  if (paranoia)
    {
      old_uid = getuid ();
      old_gid = getgid ();
    }

  if (getgrouplist (server_user, server_gid, NULL, &server_ngroups) == 0)
    {
      /* This really must never happen.  */
      dbg_log (_("Failed to run nscd as user '%s'"), server_user);
      error (EXIT_FAILURE, errno, _("initial getgrouplist failed"));
    }

  server_groups = (gid_t *) xmalloc (server_ngroups * sizeof (gid_t));

  if (getgrouplist (server_user, server_gid, server_groups, &server_ngroups)
      == -1)
    {
      dbg_log (_("Failed to run nscd as user '%s'"), server_user);
      error (EXIT_FAILURE, errno, _("getgrouplist failed"));
    }
}


/* Call setgroups(), setgid(), and setuid() to drop root privileges and
   run nscd as the user specified in the configuration file.  */
static void
finish_drop_privileges (void)
{
#if defined HAVE_LIBAUDIT && defined HAVE_LIBCAP
  /* We need to preserve the capabilities to connect to the audit daemon.  */
  cap_t new_caps = preserve_capabilities ();
#endif

  if (setgroups (server_ngroups, server_groups) == -1)
    {
      dbg_log (_("Failed to run nscd as user '%s'"), server_user);
      error (EXIT_FAILURE, errno, _("setgroups failed"));
    }

  int res;
  if (paranoia)
    res = setresgid (server_gid, server_gid, old_gid);
  else
    res = setgid (server_gid);
  if (res == -1)
    {
      dbg_log (_("Failed to run nscd as user '%s'"), server_user);
      perror ("setgid");
      exit (4);
    }

  if (paranoia)
    res = setresuid (server_uid, server_uid, old_uid);
  else
    res = setuid (server_uid);
  if (res == -1)
    {
      dbg_log (_("Failed to run nscd as user '%s'"), server_user);
      perror ("setuid");
      exit (4);
    }

#if defined HAVE_LIBAUDIT && defined HAVE_LIBCAP
  /* Remove the temporary capabilities.  */
  install_real_capabilities (new_caps);
#endif
}
