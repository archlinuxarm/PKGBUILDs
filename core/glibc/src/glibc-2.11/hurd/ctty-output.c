/* _hurd_ctty_output -- Do an output RPC and generate SIGTTOU if necessary.
   Copyright (C) 1995,97,99 Free Software Foundation, Inc.
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
#include <hurd/signal.h>

/* Call *RPC on PORT and/or CTTY.  If a call on CTTY returns EBACKGROUND,
   generate SIGTTOU if appropriate.  */

error_t
_hurd_ctty_output (io_t port, io_t ctty, error_t (*rpc) (io_t))
{
  if (ctty == MACH_PORT_NULL)
    return (*rpc) (port);
  else
    {
      struct hurd_sigstate *ss = _hurd_self_sigstate ();
      error_t err;

      do
	{
	  /* Don't use the ctty io port if we are blocking or ignoring
	     SIGTTOU.  We redo this check at the top of the loop in case
	     the signal handler changed the state.  */
	  __spin_lock (&ss->lock);
	  if (__sigismember (&ss->blocked, SIGTTOU) ||
	      ss->actions[SIGTTOU].sa_handler == SIG_IGN)
	    err = EIO;
	  else
	    err = 0;
	  __spin_unlock (&ss->lock);

	  if (err)
	    return (*rpc) (port);

	  err = (*rpc) (ctty);
	  if (err == EBACKGROUND)
	    {
	      if (_hurd_orphaned)
		/* Our process group is orphaned, so we never generate a
		   signal; we just fail.  */
		err = EIO;
	      else
		{
		  /* Send a SIGTTOU signal to our process group.

		     We must remember here not to clobber ERR, since
		     the loop condition below uses it to recall that
		  we should retry after a stop.  */

		  __USEPORT (CTTYID, _hurd_sig_post (0, SIGTTOU, port));
		  /* XXX what to do if error here? */

		  /* At this point we should have just run the handler for
		     SIGTTOU or resumed after being stopped.  Now this is
		     still a "system call", so check to see if we should
		  restart it.  */
		  __spin_lock (&ss->lock);
		  if (!(ss->actions[SIGTTOU].sa_flags & SA_RESTART))
		    err = EINTR;
		  __spin_unlock (&ss->lock);
		}
	    }
	  /* If the last RPC generated a SIGTTOU, loop to try it again.  */
	} while (err == EBACKGROUND);

      return err;
    }
}
