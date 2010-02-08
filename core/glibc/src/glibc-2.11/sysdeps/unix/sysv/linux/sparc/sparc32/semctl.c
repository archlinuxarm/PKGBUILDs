/* Semctl for architectures where word sized unions are passed indirectly
   Copyright (C) 1995,1997,1998,2000,2002,2003,2004,2006
   	Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, August 1995.

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
#include <stdarg.h>
#include <sys/sem.h>
#include <ipc_priv.h>

#include <sysdep.h>
#include <string.h>
#include <sys/syscall.h>

#include <kernel-features.h>
#include <shlib-compat.h>

struct __old_semid_ds
{
  struct __old_ipc_perm sem_perm;	/* operation permission struct */
  __time_t sem_otime;			/* last semop() time */
  __time_t sem_ctime;			/* last time changed by semctl() */
  struct sem *__sembase;		/* ptr to first semaphore in array */
  struct sem_queue *__sem_pending;	/* pending operations */
  struct sem_queue *__sem_pending_last; /* last pending operation */
  struct sem_undo *__undo;		/* ondo requests on this array */
  unsigned short int sem_nsems;		/* number of semaphores in set */
};

/* Define a `union semun' suitable for Linux here.  */
union semun
{
  int val;			/* value for SETVAL */
  struct semid_ds *buf;		/* buffer for IPC_STAT & IPC_SET */
  unsigned short int *array;	/* array for GETALL & SETALL */
  struct seminfo *__buf;	/* buffer for IPC_INFO */
  struct __old_semid_ds *__old_buf;
};

#include <bp-checks.h>
#include <bp-semctl.h>		/* definition of CHECK_SEMCTL needs union semum */

#ifdef __NR_getuid32
# if __ASSUME_32BITUIDS == 0
/* This variable is shared with all files that need to check for 32bit
   uids.  */
extern int __libc_missing_32bit_uids;
# endif
#endif

/* Return identifier for array of NSEMS semaphores associated with
   KEY.  */
#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_2)
int __old_semctl (int semid, int semnum, int cmd, ...);
#endif
int __new_semctl (int semid, int semnum, int cmd, ...);

#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_2)
int
attribute_compat_text_section
__old_semctl (int semid, int semnum, int cmd, ...)
{
  union semun arg;
  va_list ap;

  /* Get the argument only if required.  */
  arg.buf = NULL;
  switch (cmd)
    {
    case SETVAL:	/* arg.val */
    case GETALL:	/* arg.array */
    case SETALL:
    case IPC_STAT:	/* arg.buf */
    case IPC_SET:
    case SEM_STAT:
    case IPC_INFO:	/* arg.__buf */
    case SEM_INFO:
      va_start (ap, cmd);
      arg = va_arg (ap, union semun);
      va_end (ap);
      break;
    }

  return INLINE_SYSCALL (ipc, 5, IPCOP_semctl, semid, semnum, cmd,
			 CHECK_SEMCTL (&arg, semid, cmd));
}
compat_symbol (libc, __old_semctl, semctl, GLIBC_2_0);
#endif

int
__new_semctl (int semid, int semnum, int cmd, ...)
{
  union semun arg;
  va_list ap;

  /* Get the argument only if required.  */
  arg.buf = NULL;
  switch (cmd)
    {
    case SETVAL:	/* arg.val */
    case GETALL:	/* arg.array */
    case SETALL:
    case IPC_STAT:	/* arg.buf */
    case IPC_SET:
    case SEM_STAT:
    case IPC_INFO:	/* arg.__buf */
    case SEM_INFO:
      va_start (ap, cmd);
      arg = va_arg (ap, union semun);
      va_end (ap);
      break;
    }

#if __ASSUME_32BITUIDS > 0
  return INLINE_SYSCALL (ipc, 5, IPCOP_semctl, semid, semnum, cmd | __IPC_64,
			 CHECK_SEMCTL (&arg, semid, cmd | __IPC_64));
#else
  switch (cmd) {
    case SEM_STAT:
    case IPC_STAT:
    case IPC_SET:
      break;
    default:
      return INLINE_SYSCALL (ipc, 5, IPCOP_semctl, semid, semnum, cmd,
			     CHECK_SEMCTL (&arg, semid, cmd));
  }

  {
    int result;
    struct __old_semid_ds old;
    struct semid_ds *buf;

#ifdef __NR_getuid32
    if (__libc_missing_32bit_uids <= 0)
      {
	if (__libc_missing_32bit_uids < 0)
	  {
	    int save_errno = errno;

	    /* Test presence of new IPC by testing for getuid32 syscall.  */
	    result = INLINE_SYSCALL (getuid32, 0);
	    if (result == -1 && errno == ENOSYS)
	      __libc_missing_32bit_uids = 1;
	    else
	      __libc_missing_32bit_uids = 0;
	    __set_errno(save_errno);
	  }
	if (__libc_missing_32bit_uids <= 0)
	  {
	    result = INLINE_SYSCALL (ipc, 5, IPCOP_semctl, semid, semnum, cmd | __IPC_64,
				     CHECK_SEMCTL (&arg, semid, cmd | __IPC_64));
	    return result;
	  }
      }
#endif

    buf = arg.buf;
    arg.__old_buf = &old;
    if (cmd == IPC_SET)
      {
	old.sem_perm.uid = buf->sem_perm.uid;
	old.sem_perm.gid = buf->sem_perm.gid;
	old.sem_perm.mode = buf->sem_perm.mode;
	if (old.sem_perm.uid != buf->sem_perm.uid ||
	    old.sem_perm.gid != buf->sem_perm.gid)
	  {
	    __set_errno (EINVAL);
	    return -1;
	  }
      }
    result = INLINE_SYSCALL (ipc, 5, IPCOP_semctl, semid, semnum, cmd,
			     CHECK_SEMCTL (&arg, semid, cmd));
    if (result != -1 && cmd != IPC_SET)
      {
	memset(buf, 0, sizeof(*buf));
	buf->sem_perm.__key = old.sem_perm.__key;
	buf->sem_perm.uid = old.sem_perm.uid;
	buf->sem_perm.gid = old.sem_perm.gid;
	buf->sem_perm.cuid = old.sem_perm.cuid;
	buf->sem_perm.cgid = old.sem_perm.cgid;
	buf->sem_perm.mode = old.sem_perm.mode;
	buf->sem_perm.__seq = old.sem_perm.__seq;
	buf->sem_otime = old.sem_otime;
	buf->sem_ctime = old.sem_ctime;
	buf->sem_nsems = old.sem_nsems;
      }
    return result;
  }
#endif
}

versioned_symbol (libc, __new_semctl, semctl, GLIBC_2_2);
