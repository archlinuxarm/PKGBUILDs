/* Copyright (C) 1991,1992,1995,1996,1997,2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   In addition to the permissions in the GNU Lesser General Public
   License, the Free Software Foundation gives you unlimited
   permission to link the compiled version of this file with other
   programs, and to distribute those programs without any restriction
   coming from the use of this file. (The GNU Lesser General Public
   License restrictions do apply in other respects; for example, they
   cover modification of the file, and distribution when not linked
   into another program.)

   Note that people who make modified versions of this file are not
   obligated to grant this special exception for their modified
   versions; it is their choice whether to do so. The GNU Lesser
   General Public License gives permission to release a modified
   version without this exception; this exception also makes it
   possible to release a modified version which carries forward this
   exception.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef	__GNUC__
  #error This file uses GNU C extensions; you must compile with GCC.
#endif

/* The first piece of initialized data.  */
int __data_start = 0;

extern void __libc_init (int argc, char **argv, char **envp);
extern int main (int argc, char **argv, char **envp);

/* Use the stack pointer to access the arguments.  This assumes that
   we can guess how big the frame will be.  */
register long int sp asm("sp");
#ifdef __OPTIMIZE__
#define STACKSIZE 8
#else
#define STACKSIZE 10
#endif

void
__start ()
{
  int argc;
  char **argv, **envp;

  /* Set up the global pointer.  */
  asm volatile ("la $28,_gp");
  argc = ((int *) sp)[STACKSIZE];
  argv = (char **) &((int *) sp)[STACKSIZE + 1];
  envp = &argv[argc + 1];
  __environ = envp;

  __libc_init (argc, argv, envp);
  errno = 0;
  exit (main (argc, argv, envp));
}
