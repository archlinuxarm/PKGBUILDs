/* Copyright (C) 1991, 1992, 1993, 1994, 1995, 1996, 1997, 2007
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

/* Defines RTLD_PRIVATE_ERRNO and USE_DL_SYSINFO.  */
#include <dl-sysdep.h>
#include <tls.h>

#define LOSE asm volatile ("hlt")

#define SNARF_ARGS(entry_sp, argc, argv, envp)				      \
  do									      \
    {									      \
      register char **p;						      \
      argc = (int) *entry_sp;						      \
      argv = (char **) (entry_sp + 1);					      \
      p = argv;								      \
      while (*p++ != NULL)						      \
	;								      \
      if (p >= (char **) argv[0])					      \
	--p;								      \
      envp = p;							      \
    } while (0)

#define CALL_WITH_SP(fn, info, sp) \
  do {									      \
	void **ptr = (void **) sp;					      \
	*--(__typeof (info) *) ptr = info;				      \
	ptr[-1] = ptr;							      \
	--ptr;								      \
    asm volatile ("movl %0, %%esp; call %1" : : 			      \
		  "g" (ptr), "m" (*(long int *) (fn)) : "%esp"); 	      \
  } while (0)

#define RETURN_TO(sp, pc, retval) \
  asm volatile ("movl %0, %%esp; jmp %*%1 # %2" \
		: : "g" (sp), "r" (pc), "a" (retval))


#define STACK_GROWTH_DOWN

/* Get the machine-independent Mach definitions.  */
#include <sysdeps/mach/sysdep.h>


/* This should be rearranged, but at the moment this file provides
   the most useful definitions for assembler syntax details.  */
#undef ENTRY
#undef ALIGN
#include <sysdeps/unix/i386/sysdep.h>
