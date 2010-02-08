/* Copyright (C) 1997, 2000 Free Software Foundation, Inc.
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

#ifndef _SYS_POLL_H
# error "Never use <bits/poll.h> directly; include <sys/poll.h> instead."
#endif

/* Event types that can be polled for.  These bits may be set in `events'
   to indicate the interesting event types; they will appear in `revents'
   to indicate the status of the file descriptor.  */
#define POLLIN		0x0001		/* There is data to read.  */
#define POLLPRI		0x0002		/* There is urgent data to read.  */
#define POLLOUT		0x0004		/* Writing now will not block.  */

#ifdef __USE_XOPEN
/* These values are defined in XPG4.2.  */
# define POLLRDNORM	0x0010		/* Normal data may be read.  */
# define POLLRDBAND	0x0020		/* Priority data may be read.  */
# define POLLWRNORM	POLLOUT		/* Writing now will not block.  */
# define POLLWRBAND	0x0040		/* Priority data may be written.  */
#endif

/* Event types always implicitly polled for.  These bits need not be set in
   `events', but they will appear in `revents' to indicate the status of
   the file descriptor.  */
#define POLLERR		0x4000		/* Error condition.  */
#define POLLHUP		0x2000		/* Hung up.  */
#define POLLNVAL	0x8000		/* Invalid polling request.  */
