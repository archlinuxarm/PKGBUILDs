/* Copyright (C) 1991, 92, 93, 94, 95, 97 Free Software Foundation, Inc.
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

#include <hurd.h>
pid_t _hurd_pid, _hurd_ppid, _hurd_pgrp;
int _hurd_orphaned;

static void
init_pids (void)
{
  __USEPORT (PROC,
	     ({
	       __proc_getpids (port, &_hurd_pid, &_hurd_ppid, &_hurd_orphaned);
	       __proc_getpgrp (port, _hurd_pid, &_hurd_pgrp);
	     }));

  (void) &init_pids;		/* Avoid "defined but not used" warning.  */
}

text_set_element (_hurd_proc_subinit, init_pids);

#include <hurd/msg_server.h>
#include "set-hooks.h"
#include <cthreads.h>

DEFINE_HOOK (_hurd_pgrp_changed_hook, (pid_t));

/* These let user threads synchronize with an operation which changes ids.  */
unsigned int _hurd_pids_changed_stamp;
struct condition _hurd_pids_changed_sync;

kern_return_t
_S_msg_proc_newids (mach_port_t me,
		    task_t task,
		    pid_t ppid, pid_t pgrp, int orphaned)
{
  int pgrp_changed;

  if (task != __mach_task_self ())
    return EPERM;

  __mach_port_deallocate (__mach_task_self (), task);

  pgrp_changed = pgrp != _hurd_pgrp;
  _hurd_ppid = ppid;
  _hurd_pgrp = pgrp;
  _hurd_orphaned = orphaned;

  if (pgrp_changed)
    /* Run things that want notification of a pgrp change.  */
    RUN_HOOK (_hurd_pgrp_changed_hook, (pgrp));

  /* Notify any waiting user threads that the id change as been completed.  */
  ++_hurd_pids_changed_stamp;

  return 0;
}
