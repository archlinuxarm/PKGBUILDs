/* O_*, F_*, FD_* bit values for SGI Irix 4.
   Copyright (C) 1994, 1997, 2004 Free Software Foundation, Inc.
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

#ifndef	_FCNTL_H
#error "Never use <bits/fcntl.h> directly; include <fcntl.h> instead."
#endif


/* File access modes for `open' and `fcntl'.  */
#define	O_RDONLY	0	/* Open read-only.  */
#define	O_WRONLY	1	/* Open write-only.  */
#define	O_RDWR		2	/* Open read/write.  */


/* Bits OR'd into the second argument to open.  */
#define	O_CREAT		00400	/* Create file if it doesn't exist.  */
#define	O_EXCL		02000	/* Fail if file already exists.  */
#define	O_TRUNC		01000	/* Truncate file to zero length.  */
#ifdef __USE_MISC
#define	O_SYNC		00020	/* Synchronous writes.  */
#define	O_FSYNC		O_SYNC
#define	O_ASYNC		00100	/* Send SIGIO to owner when data is ready.  */
#endif

/* File status flags for `open' and `fcntl'.  */
#define	O_APPEND	000010	/* Writes append to the file.  */
#ifdef __USE_BSD
#define	O_NDELAY	000004	/* Non-blocking I/O.  */
#endif
#define O_NONBLOCK	000200	/* POSIX.1 non-blocking I/O.  */

/* Mask for file access modes.  This is system-dependent in case
   some system ever wants to define some other flavor of access.  */
#define	O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)

/* Values for the second argument to `fcntl'.  */
#define	F_DUPFD	  	0	/* Duplicate file descriptor.  */
#define	F_GETFD		1	/* Get file descriptor flags.  */
#define	F_SETFD		2	/* Set file descriptor flags.  */
#define	F_GETFL		3	/* Get file status flags.  */
#define	F_SETFL		4	/* Set file status flags.  */
#define	F_GETLK		5	/* Get record locking info.  */
#define	F_SETLK		6	/* Set record locking info.  */
#define	F_SETLKW	7	/* Set record locking info, wait.  */
#ifdef __USE_MISC
#define F_CHKFL         8       /* Check legality of file flag changes.  */
#define F_ALLOCSP       10
#define F_FREESP        11
#define F_SETBSDLK      12      /* Set Berkeley record lock.  */
#define F_SETBSDLKW     13      /* Set Berkeley record lock and wait.  */
#define F_RGETLK        20      /* Get info on a remote lock.  */
#define F_RSETLK        21      /* Set or unlock a remote lock.  */
#define F_RSETLKW       22      /* Set or unlock a remote lock and wait.  */
#endif
#if defined __USE_BSD || defined __USE_UNIX98
#define F_GETOWN        10      /* Get owner; only works on sockets.  */
#define F_SETOWN        11      /* Set owner; only works on sockets.  */
#endif


/* File descriptor flags used with F_GETFD and F_SETFD.  */
#define	FD_CLOEXEC	1	/* Close on exec.  */


#include <bits/types.h>

/* The structure describing an advisory lock.  This is the type of the third
   argument to `fcntl' for the F_GETLK, F_SETLK, and F_SETLKW requests.  */
struct flock
  {
    short int l_type;	/* Type of lock: F_RDLCK, F_WRLCK, or F_UNLCK.  */
    short int l_whence;	/* Where `l_start' is relative to (like `lseek').  */
    __off_t l_start;	/* Offset where the lock begins.  */
    __off_t l_len;	/* Size of the locked area; zero means until EOF.  */
    short int l_sysid;	/* System ID where locking process resides. */
    short int l_pid;	/* Process holding the lock.  */
  };

/* Values for the `l_type' field of a `struct flock'.  */
#define	F_RDLCK	1	/* Read lock.  */
#define	F_WRLCK	2	/* Write lock.  */
#define	F_UNLCK	3	/* Remove lock.  */


/* Define some more compatibility macros to be backward compatible with
   BSD systems which did not managed to hide these kernel macros.  */
#ifdef	__USE_BSD
#define	FAPPEND		O_APPEND
#define	FFSYNC		O_FSYNC
#define	FASYNC		O_ASYNC
#define	FNONBLOCK	O_NONBLOCK
#define	FNDELAY		O_NDELAY
#endif /* Use BSD.  */
