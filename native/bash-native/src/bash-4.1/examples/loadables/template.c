/* template - example template for loadable builtin */

/* See Makefile for compilation details. */

#include <config.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif
#include "bashansi.h"
#include <stdio.h>
#include <errno.h>

#include "builtins.h"
#include "shell.h"
#include "bashgetopt.h"

#if !defined (errno)
extern int errno;
#endif

extern char *strerror ();

template_builtin (list)
     WORD_LIST *list;
{
  int opt, rval;

  rval = EXECUTION_SUCCESS;
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "")) != -1)
    {
      switch (opt)
	{
	default:
	  builtin_usage ();
	  return (EX_USAGE);
	}
    }
  list = loptend;

  return (rval);
}

char *template_doc[] = {
	"Short description.",
	""
	"Longer description of builtin and usage.",
	(char *)NULL
};

struct builtin template_struct = {
	"template",			/* builtin name */
	template_builtin,		/* function implementing the builtin */
	BUILTIN_ENABLED,		/* initial flags for builtin */
	template_doc,			/* array of long documentation strings. */
	"template",			/* usage synopsis; becomes short_doc */
	0				/* reserved for internal use */
};
