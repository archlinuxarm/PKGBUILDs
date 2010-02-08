/* Copyright (C) 1995, 1996, 1997, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, August 1995.

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

#include <sys/msg.h>
#include <errno.h>

/* Send a message to the queue associated with the message queue
   descriptor MSQID.  The parameter MSGP points to a structure
   describing messages where the parameter MSGSZ gives the length
   of the text.  The MSGFLG parameter describes the action taken
   when the limit of the message queue length is reached.  */

int
msgsnd (msqid, msgp, msgsz, msgflg)
     int msqid;
     const void *msgp;
     size_t msgsz;
     int msgflg;
{
  __set_errno (ENOSYS);
  return -1;
}

stub_warning (msgsnd)
#include <stub-tag.h>
