/* Copyright (C) 1991, 92, 93, 95, 96, 97 Free Software Foundation, Inc.
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

#include <errno.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <stdarg.h>

/* Perform process tracing functions.  REQUEST is one of the values
   in <sys/ptrace.h>, and determines the action to be taken.
   For all requests except PTRACE_TRACEME, PID specifies the process to be
   traced.

   PID and the other arguments described above for the various requests should
   appear (those that are used for the particular request) as:
     pid_t PID, void *ADDR, int DATA, void *ADDR2
   after PID.  */
int
ptrace (request)
     enum __ptrace_request request;
{
  pid_t pid;
  void *addr;
  void *addr2;
  int data;
  va_list ap;

  switch (request)
    {
    case PTRACE_TRACEME:
    case PTRACE_CONT:
    case PTRACE_KILL:
    case PTRACE_SINGLESTEP:
    case PTRACE_ATTACH:
    case PTRACE_DETACH:
      break;

    case PTRACE_PEEKTEXT:
    case PTRACE_PEEKDATA:
    case PTRACE_PEEKUSER:
    case PTRACE_GETREGS:
    case PTRACE_SETREGS:
#ifdef PTRACE_GETFPREGS
    case PTRACE_GETFPGEGS:
#endif
    case PTRACE_SETFPREGS:
    case PTRACE_GETFPAREGS:
    case PTRACE_SETFPAREGS:
      va_start(ap, request);
      pid = va_arg(ap, pid_t);
      addr = va_arg(ap, void *);
      va_end(ap);
      break;

    case PTRACE_POKETEXT:
    case PTRACE_POKEDATA:
    case PTRACE_POKEUSER:
      va_start(ap, request);
      pid = va_arg(ap, pid_t);
      addr = va_arg(ap, void *);
      data = va_arg(ap, int);
      va_end(ap);
      break;

    case PTRACE_READDATA:
    case PTRACE_WRITEDATA:
    case PTRACE_READTEXT:
    case PTRACE_WRITETEXT:
      va_start(ap, request);
      pid = va_arg(ap, pid_t);
      addr = va_arg(ap, void *);
      data = va_arg(ap, int);
      addr2 = va_arg(ap, void *);
      va_end(ap);
      break;

    default:
      __set_errno (EINVAL);
      return -1;
    }

  __set_errno (ENOSYS);
  return -1;
}


stub_warning (ptrace)
#include <stub-tag.h>
