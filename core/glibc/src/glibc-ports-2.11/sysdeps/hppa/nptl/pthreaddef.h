/* Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
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

/* Default stack size.  */
#define ARCH_STACK_DEFAULT_SIZE	(8 * 1024 * 1024)

/* Required stack pointer alignment at beginning.  */
#define STACK_ALIGN		64

/* Minimal stack size after allocating thread descriptor and guard size.  */
#define MINIMAL_REST_STACK	2048

/* Alignment requirement for TCB, note that this must be larger than STACK_ALIGN  */
#define TCB_ALIGNMENT		STACK_ALIGN


/* Location of current stack frame.  */
#define CURRENT_STACK_FRAME  stack_pointer
register char * stack_pointer __asm__ ("%r30");


/* XXX Until we have a better place keep the definitions here.  */

#define __exit_thread_inline(val) \
  INLINE_SYSCALL (exit, 1, (val))
