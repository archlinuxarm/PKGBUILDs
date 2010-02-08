/* Copyright (c) 1998-2001, 2003-2008, 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Thorsten Kukuk <kukuk@suse.de>, 1998.

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

#ifndef _NSCD_H
#define _NSCD_H	1

#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <sys/uio.h>

/* The declarations for the request and response types are in the file
   "nscd-client.h", which should contain everything needed by client
   functions.  */
#include "nscd-client.h"


/* Handle databases.  */
typedef enum
{
  pwddb,
  grpdb,
  hstdb,
  servdb,
  lastdb
} dbtype;


/* Default limit on the number of times a value gets reloaded without
   being used in the meantime.  NSCD does not throw a value out as
   soon as it times out.  It tries to reload the value from the
   server.  Only if the value has not been used for so many rounds it
   is removed.  */
#define DEFAULT_RELOAD_LIMIT 5


/* Time before restarting the process in paranoia mode.  */
#define RESTART_INTERVAL (60 * 60)


/* Stack size for worker threads.  */
#define NSCD_THREAD_STACKSIZE 1024 * 1024 * (sizeof (void *) / 4)

/* Maximum size of stack frames we allow the thread to use.  We use
   80% of the thread stack size.  */
#define MAX_STACK_USE ((8 * NSCD_THREAD_STACKSIZE) / 10)


/* Structure describing dynamic part of one database.  */
struct database_dyn
{
  pthread_rwlock_t lock;
  pthread_cond_t prune_cond;
  pthread_mutex_t prune_lock;
  pthread_mutex_t prune_run_lock;
  time_t wakeup_time;

  int enabled;
  int check_file;
  int inotify_descr;
  int clear_cache;
  int persistent;
  int shared;
  int propagate;
  int reset_res;
  const char filename[16];
  const char *db_filename;
  time_t file_mtime;
  size_t suggested_module;
  size_t max_db_size;

  unsigned long int postimeout;	/* In seconds.  */
  unsigned long int negtimeout;	/* In seconds.  */

  int wr_fd;			/* Writable file descriptor.  */
  int ro_fd;			/* Unwritable file descriptor.  */

  const struct iovec *disabled_iov;

  struct database_pers_head *head;
  char *data;
  size_t memsize;
  pthread_mutex_t memlock;
  bool mmap_used;
  bool last_alloc_failed;
};


/* Paths of the file for the persistent storage.  */
#define _PATH_NSCD_PASSWD_DB	"/var/db/nscd/passwd"
#define _PATH_NSCD_GROUP_DB	"/var/db/nscd/group"
#define _PATH_NSCD_HOSTS_DB	"/var/db/nscd/hosts"
#define _PATH_NSCD_SERVICES_DB	"/var/db/nscd/services"

/* Path used when not using persistent storage.  */
#define _PATH_NSCD_XYZ_DB_TMP	"/var/run/nscd/dbXXXXXX"

/* Maximum alignment requirement we will encounter.  */
#define BLOCK_ALIGN_LOG 3
#define BLOCK_ALIGN (1 << BLOCK_ALIGN_LOG)
#define BLOCK_ALIGN_M1 (BLOCK_ALIGN - 1)

/* Default value for the maximum size of the database files.  */
#define DEFAULT_MAX_DB_SIZE	(32 * 1024 * 1024)

/* Number of bytes of data we initially reserve for each hash table bucket.  */
#define DEFAULT_DATASIZE_PER_BUCKET 1024

/* Default module of hash table.  */
#define DEFAULT_SUGGESTED_MODULE 211


/* Number of seconds between two cache pruning runs if we do not have
   better information when it is really needed.  */
#define CACHE_PRUNE_INTERVAL	15


/* Global variables.  */
extern struct database_dyn dbs[lastdb] attribute_hidden;
extern const char *const dbnames[lastdb];
extern const char *const serv2str[LASTREQ];

extern const struct iovec pwd_iov_disabled;
extern const struct iovec grp_iov_disabled;
extern const struct iovec hst_iov_disabled;
extern const struct iovec serv_iov_disabled;


/* Initial number of threads to run.  */
extern int nthreads;
/* Maximum number of threads to use.  */
extern int max_nthreads;

/* User name to run server processes as.  */
extern const char *server_user;

/* Name and UID of user who is allowed to request statistics.  */
extern const char *stat_user;
extern uid_t stat_uid;

/* Time the server was started.  */
extern time_t start_time;

/* Number of times clients had to wait.  */
extern unsigned long int client_queued;

/* Maximum needed alignment.  */
extern const size_t block_align;

/* Number of times a value is reloaded without being used.  UINT_MAX
   means unlimited.  */
extern unsigned int reload_count;

/* Pagesize minus one.  */
extern uintptr_t pagesize_m1;

/* Nonzero if paranoia mode is enabled.  */
extern int paranoia;
/* Time after which the process restarts.  */
extern time_t restart_time;
/* How much time between restarts.  */
extern time_t restart_interval;
/* Old current working directory.  */
extern const char *oldcwd;
/* Old user and group ID.  */
extern uid_t old_uid;
extern gid_t old_gid;


/* Prototypes for global functions.  */

/* nscd.c */
extern void termination_handler (int signum) __attribute__ ((__noreturn__));
extern int nscd_open_socket (void);

/* connections.c */
extern void nscd_init (void);
extern void close_sockets (void);
extern void start_threads (void) __attribute__ ((__noreturn__));

/* nscd_conf.c */
extern int nscd_parse_file (const char *fname,
			    struct database_dyn dbs[lastdb]);

/* nscd_stat.c */
extern void send_stats (int fd, struct database_dyn dbs[lastdb]);
extern int receive_print_stats (void) __attribute__ ((__noreturn__));

/* cache.c */
extern struct datahead *cache_search (request_type, void *key, size_t len,
				      struct database_dyn *table,
				      uid_t owner);
extern int cache_add (int type, const void *key, size_t len,
		      struct datahead *packet, bool first,
		      struct database_dyn *table, uid_t owner,
		      bool prune_wakeup);
extern time_t prune_cache (struct database_dyn *table, time_t now, int fd);

/* pwdcache.c */
extern void addpwbyname (struct database_dyn *db, int fd, request_header *req,
			 void *key, uid_t uid);
extern void addpwbyuid (struct database_dyn *db, int fd, request_header *req,
			void *key, uid_t uid);
extern void readdpwbyname (struct database_dyn *db, struct hashentry *he,
			   struct datahead *dh);
extern void readdpwbyuid (struct database_dyn *db, struct hashentry *he,
			  struct datahead *dh);

/* grpcache.c */
extern void addgrbyname (struct database_dyn *db, int fd, request_header *req,
			 void *key, uid_t uid);
extern void addgrbygid (struct database_dyn *db, int fd, request_header *req,
			void *key, uid_t uid);
extern void readdgrbyname (struct database_dyn *db, struct hashentry *he,
			   struct datahead *dh);
extern void readdgrbygid (struct database_dyn *db, struct hashentry *he,
			  struct datahead *dh);

/* hstcache.c */
extern void addhstbyname (struct database_dyn *db, int fd, request_header *req,
			  void *key, uid_t uid);
extern void addhstbyaddr (struct database_dyn *db, int fd, request_header *req,
			  void *key, uid_t uid);
extern void addhstbynamev6 (struct database_dyn *db, int fd,
			    request_header *req, void *key, uid_t uid);
extern void addhstbyaddrv6 (struct database_dyn *db, int fd,
			    request_header *req, void *key, uid_t uid);
extern void readdhstbyname (struct database_dyn *db, struct hashentry *he,
			    struct datahead *dh);
extern void readdhstbyaddr (struct database_dyn *db, struct hashentry *he,
			    struct datahead *dh);
extern void readdhstbynamev6 (struct database_dyn *db, struct hashentry *he,
			      struct datahead *dh);
extern void readdhstbyaddrv6 (struct database_dyn *db, struct hashentry *he,
			      struct datahead *dh);

/* aicache.c */
extern void addhstai (struct database_dyn *db, int fd, request_header *req,
		      void *key, uid_t uid);
extern void readdhstai (struct database_dyn *db, struct hashentry *he,
			struct datahead *dh);


/* initgrcache.c */
extern void addinitgroups (struct database_dyn *db, int fd,
			   request_header *req, void *key, uid_t uid);
extern void readdinitgroups (struct database_dyn *db, struct hashentry *he,
			     struct datahead *dh);

/* servicecache.c */
extern void addservbyname (struct database_dyn *db, int fd,
			   request_header *req, void *key, uid_t uid);
extern void readdservbyname (struct database_dyn *db, struct hashentry *he,
			     struct datahead *dh);
extern void addservbyport (struct database_dyn *db, int fd,
			   request_header *req, void *key, uid_t uid);
extern void readdservbyport (struct database_dyn *db, struct hashentry *he,
			     struct datahead *dh);

/* mem.c */
extern void *mempool_alloc (struct database_dyn *db, size_t len,
			    int data_alloc);
extern void gc (struct database_dyn *db);


/* nscd_setup_thread.c */
extern int setup_thread (struct database_dyn *db);


/* Special version of TEMP_FAILURE_RETRY for functions returning error
   values.  */
#define TEMP_FAILURE_RETRY_VAL(expression) \
  (__extension__							      \
    ({ long int __result;						      \
       do __result = (long int) (expression);				      \
       while (__result == EINTR);					      \
       __result; }))

#endif /* nscd.h */
