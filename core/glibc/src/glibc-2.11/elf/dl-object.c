/* Storage management for the chain of loaded shared objects.
   Copyright (C) 1995-2002,2004,2006,2007,2008 Free Software Foundation, Inc.
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ldsodefs.h>

#include <assert.h>


/* Allocate a `struct link_map' for a new object being loaded,
   and enter it into the _dl_loaded list.  */

struct link_map *
internal_function
_dl_new_object (char *realname, const char *libname, int type,
		struct link_map *loader, int mode, Lmid_t nsid)
{
  struct link_map *l;
  int idx;
  size_t libname_len = strlen (libname) + 1;
  struct link_map *new;
  struct libname_list *newname;
#ifdef SHARED
  /* We create the map for the executable before we know whether we have
     auditing libraries and if yes, how many.  Assume the worst.  */
  unsigned int naudit = GLRO(dl_naudit) ?: ((mode & __RTLD_OPENEXEC)
					    ? DL_NNS : 0);
  size_t audit_space = naudit * sizeof (new->l_audit[0]);
#else
# define audit_space 0
#endif

  new = (struct link_map *) calloc (sizeof (*new) + audit_space
				    + sizeof (struct link_map *)
				    + sizeof (*newname) + libname_len, 1);
  if (new == NULL)
    return NULL;

  new->l_real = new;
  new->l_symbolic_searchlist.r_list = (struct link_map **) ((char *) (new + 1)
							    + audit_space);

  new->l_libname = newname
    = (struct libname_list *) (new->l_symbolic_searchlist.r_list + 1);
  newname->name = (char *) memcpy (newname + 1, libname, libname_len);
  /* newname->next = NULL;	We use calloc therefore not necessary.  */
  newname->dont_free = 1;

  new->l_name = realname;
  new->l_type = type;
  new->l_loader = loader;
#if NO_TLS_OFFSET != 0
  new->l_tls_offset = NO_TLS_OFFSET;
#endif
  new->l_ns = nsid;

#ifdef SHARED
  for (unsigned int cnt = 0; cnt < naudit; ++cnt)
    {
      new->l_audit[cnt].cookie = (uintptr_t) new;
      /* new->l_audit[cnt].bindflags = 0; */
    }
#endif

  /* new->l_global = 0;	We use calloc therefore not necessary.  */

  /* Use the 'l_scope_mem' array by default for the the 'l_scope'
     information.  If we need more entries we will allocate a large
     array dynamically.  */
  new->l_scope = new->l_scope_mem;
  new->l_scope_max = sizeof (new->l_scope_mem) / sizeof (new->l_scope_mem[0]);

  /* Counter for the scopes we have to handle.  */
  idx = 0;

  if (GL(dl_ns)[nsid]._ns_loaded != NULL)
    {
      l = GL(dl_ns)[nsid]._ns_loaded;
      while (l->l_next != NULL)
	l = l->l_next;
      new->l_prev = l;
      /* new->l_next = NULL;	Would be necessary but we use calloc.  */
      l->l_next = new;

      /* Add the global scope.  */
      new->l_scope[idx++] = &GL(dl_ns)[nsid]._ns_loaded->l_searchlist;
    }
  else
    GL(dl_ns)[nsid]._ns_loaded = new;
  ++GL(dl_ns)[nsid]._ns_nloaded;
  new->l_serial = GL(dl_load_adds);
  ++GL(dl_load_adds);

  /* If we have no loader the new object acts as it.  */
  if (loader == NULL)
    loader = new;
  else
    /* Determine the local scope.  */
    while (loader->l_loader != NULL)
      loader = loader->l_loader;

  /* Insert the scope if it isn't the global scope we already added.  */
  if (idx == 0 || &loader->l_searchlist != new->l_scope[0])
    {
      if ((mode & RTLD_DEEPBIND) != 0 && idx != 0)
	{
	  new->l_scope[1] = new->l_scope[0];
	  idx = 0;
	}

      new->l_scope[idx] = &loader->l_searchlist;
    }

  new->l_local_scope[0] = &new->l_searchlist;

  /* Don't try to find the origin for the main map which has the name "".  */
  if (realname[0] != '\0')
    {
      size_t realname_len = strlen (realname) + 1;
      char *origin;
      char *cp;

      if (realname[0] == '/')
	{
	  /* It is an absolute path.  Use it.  But we have to make a
	     copy since we strip out the trailing slash.  */
	  cp = origin = (char *) malloc (realname_len);
	  if (origin == NULL)
	    {
	      origin = (char *) -1;
	      goto out;
	    }
	}
      else
	{
	  size_t len = realname_len;
	  char *result = NULL;

	  /* Get the current directory name.  */
	  origin = NULL;
	  do
	    {
	      char *new_origin;

	      len += 128;
	      new_origin = (char *) realloc (origin, len);
	      if (new_origin == NULL)
		/* We exit the loop.  Note that result == NULL.  */
		break;
	      origin = new_origin;
	    }
	  while ((result = __getcwd (origin, len - realname_len)) == NULL
		 && errno == ERANGE);

	  if (result == NULL)
	    {
	      /* We were not able to determine the current directory.
	         Note that free(origin) is OK if origin == NULL.  */
	      free (origin);
	      origin = (char *) -1;
	      goto out;
	    }

	  /* Find the end of the path and see whether we have to add a
	     slash.  We could use rawmemchr but this need not be
	     fast.  */
	  cp = (strchr) (origin, '\0');
	  if (cp[-1] != '/')
	    *cp++ = '/';
	}

      /* Add the real file name.  */
      cp = __mempcpy (cp, realname, realname_len);

      /* Now remove the filename and the slash.  Leave the slash if
	 the name is something like "/foo".  */
      do
	--cp;
      while (*cp != '/');

      if (cp == origin)
	/* Keep the only slash which is the first character.  */
	++cp;
      *cp = '\0';

    out:
      new->l_origin = origin;
    }

  return new;
}
