/* Sample builtin to be dynamically loaded with enable -f and create a new
   builtin. */

/* See Makefile for compilation details. */

#include <config.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <stdio.h>

#include "builtins.h"
#include "shell.h"
#include "bashgetopt.h"

/* A builtin `xxx' is normally implemented with an `xxx_builtin' function.
   If you're converting a command that uses the normal Unix argc/argv
   calling convention, use argv = make_builtin_argv (list, &argc) and call
   the original `main' something like `xxx_main'.  Look at cat.c for an
   example.

   Builtins should use internal_getopt to parse options.  It is the same as
   getopt(3), but it takes a WORD_LIST *.  Look at print.c for an example
   of its use.

   If the builtin takes no options, call no_options(list) before doing
   anything else.  If it returns a non-zero value, your builtin should
   immediately return EX_USAGE.  Look at logname.c for an example.

   A builtin command returns EXECUTION_SUCCESS for success and
   EXECUTION_FAILURE to indicate failure. */
int
hello_builtin (list)
     WORD_LIST *list;
{
  printf("hello world\n");
  fflush (stdout);
  return (EXECUTION_SUCCESS);
}

/* An array of strings forming the `long' documentation for a builtin xxx,
   which is printed by `help xxx'.  It must end with a NULL.  By convention,
   the first line is a short description. */
char *hello_doc[] = {
	"Sample builtin.",
	"",
	"this is the long doc for the sample hello builtin",
	(char *)NULL
};

/* The standard structure describing a builtin command.  bash keeps an array
   of these structures.  The flags must include BUILTIN_ENABLED so the
   builtin can be used. */
struct builtin hello_struct = {
	"hello",		/* builtin name */
	hello_builtin,		/* function implementing the builtin */
	BUILTIN_ENABLED,	/* initial flags for builtin */
	hello_doc,		/* array of long documentation strings. */
	"hello",		/* usage synopsis; becomes short_doc */
	0			/* reserved for internal use */
};
	
