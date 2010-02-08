/* Define POSIX options for GNU/Hurd.
   Copyright (C) 1998,2000,2001,2002,2006,2009 Free Software Foundation, Inc.
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

#ifndef _UNISTD_H
#error "Never include this file directly; use <unistd.h> instead."
#endif

#ifndef	_BITS_POSIX_OPT_H
#define	_BITS_POSIX_OPT_H	1


/* Job control is supported.  */
#define	_POSIX_JOB_CONTROL	1

/* Processes have a saved set-user-ID and a saved set-group-ID.  */
#define	_POSIX_SAVED_IDS	1

/* Synchronizing file data is supported, but msync is missing.  */
#undef _POSIX_SYNCHRONIZED_IO

/* The fsync function is present.  */
#define	_POSIX_FSYNC	200809L

/* Mapping of files to memory is supported.  */
#define	_POSIX_MAPPED_FILES	200809L

/* Locking of all memory could be supported in future.  */
#define	_POSIX_MEMLOCK	0

/* Locking of ranges of memory is supported.  */
#define	_POSIX_MEMLOCK_RANGE	200809L

/* Setting of memory protections is supported.  */
#define	_POSIX_MEMORY_PROTECTION	200809L

/* Elements of the `c_cc' member of `struct termios' structure
   can be disabled by using the value _POSIX_VDISABLE.  */
#define _POSIX_VDISABLE			((unsigned char) -1)


/* Different Hurd filesystems might do these differently.
   You must query the particular file with `pathconf' or `fpathconf'.  */
#undef _POSIX_CHOWN_RESTRICTED	/* Only root can change owner of file?  */
#undef _POSIX_NO_TRUNC		/* Overlong file names get error?  */
#undef _POSIX_SYNC_IO		/* File supports O_SYNC et al?  */


/* We do not have the POSIX threads interface.  */
#define _POSIX_THREADS	-1

/* We have the reentrant functions described in POSIX.  */
#define _POSIX_REENTRANT_FUNCTIONS      1
#define _POSIX_THREAD_SAFE_FUNCTIONS	200809L

/* These are all things that won't be supported when _POSIX_THREADS is not.  */
#define _POSIX_THREAD_PRIORITY_SCHEDULING	-1
#define _POSIX_THREAD_ATTR_STACKSIZE		-1
#define _POSIX_THREAD_ATTR_STACKADDR		-1
#define _POSIX_SEMAPHORES			-1

/* Real-time signals are not yet supported.  */
#define _POSIX_REALTIME_SIGNALS	-1

/* Asynchronous I/O might supported with the existing ABI.  */
#define _POSIX_ASYNCHRONOUS_IO	0
/* Alternative name for Unix98.  */
#define _LFS_ASYNCHRONOUS_IO	_POSIX_ASYNCHRONOUS_IO

/* The LFS support in asynchronous I/O is also available.  */
#define _LFS64_ASYNCHRONOUS_IO	_POSIX_ASYNCHRONOUS_IO

/* The rest of the LFS is also available.  */
#define _LFS_LARGEFILE		1
#define _LFS64_LARGEFILE	1
#define _LFS64_STDIO		1

/* POSIX.4 shared memory objects are supported (using regular files).  */
#define _POSIX_SHARED_MEMORY_OBJECTS	_POSIX_MAPPED_FILES

/* CPU-time clocks support needs to be checked at runtime.  */
#define _POSIX_CPUTIME	0

/* Clock support in threads must be also checked at runtime.  */
#define _POSIX_THREAD_CPUTIME	0

/* GNU libc provides regular expression handling.  */
#define _POSIX_REGEXP	1

/* Reader/Writer locks are not available.  */
#define _POSIX_READER_WRITER_LOCKS	-1

/* We have a POSIX shell.  */
#define _POSIX_SHELL	1

/* We cannot support the Timeouts option without _POSIX_THREADS.  */
#define _POSIX_TIMEOUTS	-1

/* The `spawn' function family is supported.  */
#define _POSIX_SPAWN	200809L

/* We do not have POSIX timers, but could in future without ABI change.  */
#define _POSIX_TIMERS	0

/* The barrier functions are not available.  */
#define _POSIX_BARRIERS	-1

/* POSIX message queues could be available in future.  */
#define	_POSIX_MESSAGE_PASSING	0

/* Thread process-shared synchronization is not supported.  */
#define _POSIX_THREAD_PROCESS_SHARED	-1

/* The monotonic clock might be available.  */
#define _POSIX_MONOTONIC_CLOCK	0

/* The clock selection interfaces are available.  */
#define _POSIX_CLOCK_SELECTION	200809L

/* Advisory information interfaces could be available in future.  */
#define _POSIX_ADVISORY_INFO	0

/* IPv6 support is available.  */
#define _POSIX_IPV6	200809L

/* Raw socket support is available.  */
#define _POSIX_RAW_SOCKETS	200809L

/* We have at least one terminal.  */
#define _POSIX2_CHAR_TERM	200809L

/* Neither process nor thread sporadic server interfaces is available.  */
#define _POSIX_SPORADIC_SERVER	-1
#define _POSIX_THREAD_SPORADIC_SERVER	-1

/* trace.h is not available.  */
#define _POSIX_TRACE	-1
#define _POSIX_TRACE_EVENT_FILTER	-1
#define _POSIX_TRACE_INHERIT	-1
#define _POSIX_TRACE_LOG	-1

/* Typed memory objects are not available.  */
#define _POSIX_TYPED_MEMORY_OBJECTS	-1

/* No support for priority inheritance or protection so far.  */
#define _POSIX_THREAD_PRIO_INHERIT	-1
#define _POSIX_THREAD_PRIO_PROTECT	-1


#endif /* bits/posix_opt.h */
