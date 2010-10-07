/* Copyright (C) 1999 Free Software Foundation, Inc. */

/* This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * shtty.h -- include the correct system-dependent files to manipulate the
 *	      tty
 */

#ifndef __SH_TTY_H_
#define __SH_TTY_H_

#include "stdc.h"

#if defined (_POSIX_VERSION) && defined (HAVE_TERMIOS_H) && defined (HAVE_TCGETATTR) && !defined (TERMIOS_MISSING)
#  define TERMIOS_TTY_DRIVER
#else
#  if defined (HAVE_TERMIO_H)
#    define TERMIO_TTY_DRIVER
#  else
#    define NEW_TTY_DRIVER
#  endif
#endif

/*
 * The _POSIX_SOURCE define is to avoid multiple symbol definitions
 * between sys/ioctl.h and termios.h.  Ditto for the test against SunOS4
 * and the undefining of several symbols.
 */
      
#ifdef TERMIOS_TTY_DRIVER
#  if (defined (SunOS4) || defined (SunOS5)) && !defined (_POSIX_SOURCE)
#    define _POSIX_SOURCE
#  endif
#  if defined (SunOS4)
#    undef ECHO
#    undef NOFLSH
#    undef TOSTOP
#  endif /* SunOS4 */
#  include <termios.h>
#  define TTYSTRUCT struct termios
#else
#  ifdef TERMIO_TTY_DRIVER
#    include <termio.h>
#    define TTYSTRUCT struct termio
#  else	/* NEW_TTY_DRIVER */
#    include <sgtty.h>
#    define TTYSTRUCT struct sgttyb
#  endif
#endif

/* Functions imported from lib/sh/shtty.c */

/* Get and set terminal attributes for the file descriptor passed as
   an argument. */
extern int ttgetattr __P((int, TTYSTRUCT *));
extern int ttsetattr __P((int, TTYSTRUCT *));

/* Save and restore the terminal's attributes from static storage. */
extern void ttsave __P((void));
extern void ttrestore __P((void));

/* Return the attributes corresponding to the file descriptor (0 or 1)
   passed as an argument. */
extern TTYSTRUCT *ttattr __P((int));

/* These functions only operate on the passed TTYSTRUCT; they don't
   actually change anything with the kernel's current tty settings. */
extern int tt_setonechar __P((TTYSTRUCT *));
extern int tt_setnoecho __P((TTYSTRUCT *));
extern int tt_seteightbit __P((TTYSTRUCT *));
extern int tt_setnocanon __P((TTYSTRUCT *));
extern int tt_setcbreak __P((TTYSTRUCT *));

/* These functions are all generally mutually exclusive.  If you call
   more than one (bracketed with calls to ttsave and ttrestore, of
   course), the right thing will happen, but more system calls will be
   executed than absolutely necessary.  You can do all of this yourself
   with the other functions; these are only conveniences. */

/* These functions work with a given file descriptor and set terminal
   attributes */
extern int ttfd_onechar __P((int, TTYSTRUCT *));
extern int ttfd_noecho __P((int, TTYSTRUCT *));
extern int ttfd_eightbit __P((int, TTYSTRUCT *));
extern int ttfd_nocanon __P((int, TTYSTRUCT *));

extern int ttfd_cbreak __P((int, TTYSTRUCT *));

/* These functions work with fd 0 and the TTYSTRUCT saved with ttsave () */
extern int ttonechar __P((void));
extern int ttnoecho __P((void));
extern int tteightbit __P((void));
extern int ttnocanon __P((void));

extern int ttcbreak __P((void));

#endif
