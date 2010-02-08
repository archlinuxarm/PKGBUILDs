/* Special .init and .fini section support.
   Copyright (C) 1995, 1996, 1997, 2000 Free Software Foundation, Inc.
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

/* This file is compiled into assembly code which is then munged by a sed
   script into two files: crti.s and crtn.s.

   * crti.s puts a function prologue at the beginning of the
   .init and .fini sections and defines global symbols for
   those addresses, so they can be called as functions.

   * crtn.s puts the corresponding function epilogues
   in the .init and .fini sections. */

#include <stdlib.h>

/* We use embedded asm for .section unconditionally, as this makes it
   easier to insert the necessary directives into crtn.S. */
#define SECTION(x) asm (".section " x )

/* Embed an #include to pull in the alignment and .end directives. */
asm ("\n#include \"defs.h\"");

/* The initial common code ends here. */
asm ("\n/*@HEADER_ENDS*/");

/* To determine whether we need .end and .align: */
asm ("\n/*@TESTS_BEGIN*/");
extern void dummy (void (*foo) (void));
void
dummy (void (*foo) (void))
{
  if (foo)
    (*foo) ();
}
asm ("\n/*@TESTS_END*/");

/* The beginning of _init:  */
asm ("\n/*@_init_PROLOG_BEGINS*/");

static void
call_gmon_start(void)
{
  extern void __gmon_start__ (void) __attribute__ ((weak)); /*weak_extern (__gmon_start__);*/
  void (*gmon_start) (void) = __gmon_start__;

  if (gmon_start)
    gmon_start ();
}

SECTION (".init");
extern void __attribute__ ((section (".init"))) _init (void);
void
_init (void)
{
  /* We cannot use the normal constructor mechanism in gcrt1.o because it
     appears before crtbegin.o in the link, so the header elt of .ctors
     would come after the elt for __gmon_start__.  One approach is for
     gcrt1.o to reference a symbol which would be defined by some library
     module which has a constructor; but then user code's constructors
     would come first, and not be profiled.  */
  call_gmon_start ();

  asm ("ALIGN");
  asm("END_INIT");
  /* Now the epilog. */
  asm ("\n/*@_init_PROLOG_ENDS*/");
  asm ("\n/*@_init_EPILOG_BEGINS*/");
  SECTION(".init");
}
asm ("END_INIT");

/* End of the _init epilog, beginning of the _fini prolog. */
asm ("\n/*@_init_EPILOG_ENDS*/");
asm ("\n/*@_fini_PROLOG_BEGINS*/");

SECTION (".fini");
extern void __attribute__ ((section (".fini"))) _fini (void);
void
_fini (void)
{

  /* End of the _fini prolog. */
  asm ("ALIGN");
  asm ("END_FINI");
  asm ("\n/*@_fini_PROLOG_ENDS*/");

  {
    /* Let GCC know that _fini is not a leaf function by having a dummy
       function call here.  We arrange for this call to be omitted from
       either crt file.  */
    extern void i_am_not_a_leaf (void);
    i_am_not_a_leaf ();
  }

  /* Beginning of the _fini epilog. */
  asm ("\n/*@_fini_EPILOG_BEGINS*/");
  SECTION (".fini");
}
asm ("END_FINI");

/* End of the _fini epilog.  Any further generated assembly (e.g. .ident)
   is shared between both crt files. */
asm ("\n/*@_fini_EPILOG_ENDS*/");
asm ("\n/*@TRAILER_BEGINS*/");

/* End of file. */
