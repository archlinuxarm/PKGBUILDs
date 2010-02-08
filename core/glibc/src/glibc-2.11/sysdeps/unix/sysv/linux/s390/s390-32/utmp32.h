/* The `struct utmp' type, describing entries in the utmp file.  GNU version.
   Copyright (C) 1993, 1996, 1997, 1998, 1999, 2002, 2008
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

#ifndef _UTMP32_H
#define _UTMP32_H 1

#include <paths.h>
#include <sys/time.h>
#include <sys/types.h>
#include <bits/wordsize.h>
#include <utmp.h>

/* The structure describing an entry in the user accounting database.  */
struct utmp32
{
  short int ut_type;		/* Type of login.  */
  pid_t ut_pid;			/* Process ID of login process.  */
  char ut_line[UT_LINESIZE];	/* Devicename.  */
  char ut_id[4];		/* Inittab ID.  */
  char ut_user[UT_NAMESIZE];	/* Username.  */
  char ut_host[UT_HOSTSIZE];	/* Hostname for remote login.  */
  struct exit_status ut_exit;	/* Exit status of a process marked
				   as DEAD_PROCESS.  */
  int32_t ut_session;		/* Session ID, used for windowing.  */
  struct
  {
    int32_t tv_sec;		/* Seconds.  */
    int32_t tv_usec;		/* Microseconds.  */
  } ut_tv;			/* Time entry was made.  */

  int32_t ut_addr_v6[4];	/* Internet address of remote host.  */
  char __unused[20];		/* Reserved for future use.  */
};


#endif  /* utmp32.h  */
