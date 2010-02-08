/* Map in a shared object's segments from the file.
   Copyright (C) 1995-2005, 2006, 2007, 2009 Free Software Foundation, Inc.
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

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ldsodefs.h>
#include <bits/wordsize.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "dynamic-link.h"
#include <abi-tag.h>
#include <stackinfo.h>
#include <caller.h>
#include <sysdep.h>

#include <dl-dst.h>

/* On some systems, no flag bits are given to specify file mapping.  */
#ifndef MAP_FILE
# define MAP_FILE	0
#endif

/* The right way to map in the shared library files is MAP_COPY, which
   makes a virtual copy of the data at the time of the mmap call; this
   guarantees the mapped pages will be consistent even if the file is
   overwritten.  Some losing VM systems like Linux's lack MAP_COPY.  All we
   get is MAP_PRIVATE, which copies each page when it is modified; this
   means if the file is overwritten, we may at some point get some pages
   from the new version after starting with pages from the old version.

   To make up for the lack and avoid the overwriting problem,
   what Linux does have is MAP_DENYWRITE.  This prevents anyone
   from modifying the file while we have it mapped.  */
#ifndef MAP_COPY
# ifdef MAP_DENYWRITE
#  define MAP_COPY	(MAP_PRIVATE | MAP_DENYWRITE)
# else
#  define MAP_COPY	MAP_PRIVATE
# endif
#endif

/* Some systems link their relocatable objects for another base address
   than 0.  We want to know the base address for these such that we can
   subtract this address from the segment addresses during mapping.
   This results in a more efficient address space usage.  Defaults to
   zero for almost all systems.  */
#ifndef MAP_BASE_ADDR
# define MAP_BASE_ADDR(l)	0
#endif


#include <endian.h>
#if BYTE_ORDER == BIG_ENDIAN
# define byteorder ELFDATA2MSB
#elif BYTE_ORDER == LITTLE_ENDIAN
# define byteorder ELFDATA2LSB
#else
# error "Unknown BYTE_ORDER " BYTE_ORDER
# define byteorder ELFDATANONE
#endif

#define STRING(x) __STRING (x)

/* Handle situations where we have a preferred location in memory for
   the shared objects.  */
#ifdef ELF_PREFERRED_ADDRESS_DATA
ELF_PREFERRED_ADDRESS_DATA;
#endif
#ifndef ELF_PREFERRED_ADDRESS
# define ELF_PREFERRED_ADDRESS(loader, maplength, mapstartpref) (mapstartpref)
#endif
#ifndef ELF_FIXED_ADDRESS
# define ELF_FIXED_ADDRESS(loader, mapstart) ((void) 0)
#endif


int __stack_prot attribute_hidden attribute_relro
#if _STACK_GROWS_DOWN && defined PROT_GROWSDOWN
  = PROT_GROWSDOWN;
#elif _STACK_GROWS_UP && defined PROT_GROWSUP
  = PROT_GROWSUP;
#else
  = 0;
#endif


/* Type for the buffer we put the ELF header and hopefully the program
   header.  This buffer does not really have to be too large.  In most
   cases the program header follows the ELF header directly.  If this
   is not the case all bets are off and we can make the header
   arbitrarily large and still won't get it read.  This means the only
   question is how large are the ELF and program header combined.  The
   ELF header 32-bit files is 52 bytes long and in 64-bit files is 64
   bytes long.  Each program header entry is again 32 and 56 bytes
   long respectively.  I.e., even with a file which has 10 program
   header entries we only have to read 372B/624B respectively.  Add to
   this a bit of margin for program notes and reading 512B and 832B
   for 32-bit and 64-bit files respecitvely is enough.  If this
   heuristic should really fail for some file the code in
   `_dl_map_object_from_fd' knows how to recover.  */
struct filebuf
{
  ssize_t len;
#if __WORDSIZE == 32
# define FILEBUF_SIZE 512
#else
# define FILEBUF_SIZE 832
#endif
  char buf[FILEBUF_SIZE] __attribute__ ((aligned (__alignof (ElfW(Ehdr)))));
};

/* This is the decomposed LD_LIBRARY_PATH search path.  */
static struct r_search_path_struct env_path_list attribute_relro;

/* List of the hardware capabilities we might end up using.  */
static const struct r_strlenpair *capstr attribute_relro;
static size_t ncapstr attribute_relro;
static size_t max_capstrlen attribute_relro;


/* Get the generated information about the trusted directories.  */
#include "trusted-dirs.h"

static const char system_dirs[] = SYSTEM_DIRS;
static const size_t system_dirs_len[] =
{
  SYSTEM_DIRS_LEN
};
#define nsystem_dirs_len \
  (sizeof (system_dirs_len) / sizeof (system_dirs_len[0]))


/* Local version of `strdup' function.  */
static char *
local_strdup (const char *s)
{
  size_t len = strlen (s) + 1;
  void *new = malloc (len);

  if (new == NULL)
    return NULL;

  return (char *) memcpy (new, s, len);
}


static size_t
is_dst (const char *start, const char *name, const char *str,
	int is_path, int secure)
{
  size_t len;
  bool is_curly = false;

  if (name[0] == '{')
    {
      is_curly = true;
      ++name;
    }

  len = 0;
  while (name[len] == str[len] && name[len] != '\0')
    ++len;

  if (is_curly)
    {
      if (name[len] != '}')
	return 0;

      /* Point again at the beginning of the name.  */
      --name;
      /* Skip over closing curly brace and adjust for the --name.  */
      len += 2;
    }
  else if (name[len] != '\0' && name[len] != '/'
	   && (!is_path || name[len] != ':'))
    return 0;

  if (__builtin_expect (secure, 0)
      && ((name[len] != '\0' && (!is_path || name[len] != ':'))
	  || (name != start + 1 && (!is_path || name[-2] != ':'))))
    return 0;

  return len;
}


size_t
_dl_dst_count (const char *name, int is_path)
{
  const char *const start = name;
  size_t cnt = 0;

  do
    {
      size_t len;

      /* $ORIGIN is not expanded for SUID/GUID programs (except if it
	 is $ORIGIN alone) and it must always appear first in path.  */
      ++name;
      if ((len = is_dst (start, name, "ORIGIN", is_path,
			 INTUSE(__libc_enable_secure))) != 0
	  || (len = is_dst (start, name, "PLATFORM", is_path, 0)) != 0
	  || (len = is_dst (start, name, "LIB", is_path, 0)) != 0)
	++cnt;

      name = strchr (name + len, '$');
    }
  while (name != NULL);

  return cnt;
}


char *
_dl_dst_substitute (struct link_map *l, const char *name, char *result,
		    int is_path)
{
  const char *const start = name;
  char *last_elem, *wp;

  /* Now fill the result path.  While copying over the string we keep
     track of the start of the last path element.  When we come accross
     a DST we copy over the value or (if the value is not available)
     leave the entire path element out.  */
  last_elem = wp = result;

  do
    {
      if (__builtin_expect (*name == '$', 0))
	{
	  const char *repl = NULL;
	  size_t len;

	  ++name;
	  if ((len = is_dst (start, name, "ORIGIN", is_path,
			     INTUSE(__libc_enable_secure))) != 0)
	    {
#ifndef SHARED
	      if (l == NULL)
		repl = _dl_get_origin ();
	      else
#endif
		repl = l->l_origin;
	    }
	  else if ((len = is_dst (start, name, "PLATFORM", is_path, 0)) != 0)
	    repl = GLRO(dl_platform);
	  else if ((len = is_dst (start, name, "LIB", is_path, 0)) != 0)
	    repl = DL_DST_LIB;

	  if (repl != NULL && repl != (const char *) -1)
	    {
	      wp = __stpcpy (wp, repl);
	      name += len;
	    }
	  else if (len > 1)
	    {
	      /* We cannot use this path element, the value of the
		 replacement is unknown.  */
	      wp = last_elem;
	      name += len;
	      while (*name != '\0' && (!is_path || *name != ':'))
		++name;
	    }
	  else
	    /* No DST we recognize.  */
	    *wp++ = '$';
	}
      else
	{
	  *wp++ = *name++;
	  if (is_path && *name == ':')
	    last_elem = wp;
	}
    }
  while (*name != '\0');

  *wp = '\0';

  return result;
}


/* Return copy of argument with all recognized dynamic string tokens
   ($ORIGIN and $PLATFORM for now) replaced.  On some platforms it
   might not be possible to determine the path from which the object
   belonging to the map is loaded.  In this case the path element
   containing $ORIGIN is left out.  */
static char *
expand_dynamic_string_token (struct link_map *l, const char *s)
{
  /* We make two runs over the string.  First we determine how large the
     resulting string is and then we copy it over.  Since this is now
     frequently executed operation we are looking here not for performance
     but rather for code size.  */
  size_t cnt;
  size_t total;
  char *result;

  /* Determine the number of DST elements.  */
  cnt = DL_DST_COUNT (s, 1);

  /* If we do not have to replace anything simply copy the string.  */
  if (__builtin_expect (cnt, 0) == 0)
    return local_strdup (s);

  /* Determine the length of the substituted string.  */
  total = DL_DST_REQUIRED (l, s, strlen (s), cnt);

  /* Allocate the necessary memory.  */
  result = (char *) malloc (total + 1);
  if (result == NULL)
    return NULL;

  return _dl_dst_substitute (l, s, result, 1);
}


/* Add `name' to the list of names for a particular shared object.
   `name' is expected to have been allocated with malloc and will
   be freed if the shared object already has this name.
   Returns false if the object already had this name.  */
static void
internal_function
add_name_to_object (struct link_map *l, const char *name)
{
  struct libname_list *lnp, *lastp;
  struct libname_list *newname;
  size_t name_len;

  lastp = NULL;
  for (lnp = l->l_libname; lnp != NULL; lastp = lnp, lnp = lnp->next)
    if (strcmp (name, lnp->name) == 0)
      return;

  name_len = strlen (name) + 1;
  newname = (struct libname_list *) malloc (sizeof *newname + name_len);
  if (newname == NULL)
    {
      /* No more memory.  */
      _dl_signal_error (ENOMEM, name, NULL, N_("cannot allocate name record"));
      return;
    }
  /* The object should have a libname set from _dl_new_object.  */
  assert (lastp != NULL);

  newname->name = memcpy (newname + 1, name, name_len);
  newname->next = NULL;
  newname->dont_free = 0;
  lastp->next = newname;
}

/* Standard search directories.  */
static struct r_search_path_struct rtld_search_dirs attribute_relro;

static size_t max_dirnamelen;

static struct r_search_path_elem **
fillin_rpath (char *rpath, struct r_search_path_elem **result, const char *sep,
	      int check_trusted, const char *what, const char *where)
{
  char *cp;
  size_t nelems = 0;

  while ((cp = __strsep (&rpath, sep)) != NULL)
    {
      struct r_search_path_elem *dirp;
      size_t len = strlen (cp);

      /* `strsep' can pass an empty string.  This has to be
         interpreted as `use the current directory'. */
      if (len == 0)
	{
	  static const char curwd[] = "./";
	  cp = (char *) curwd;
	}

      /* Remove trailing slashes (except for "/").  */
      while (len > 1 && cp[len - 1] == '/')
	--len;

      /* Now add one if there is none so far.  */
      if (len > 0 && cp[len - 1] != '/')
	cp[len++] = '/';

      /* Make sure we don't use untrusted directories if we run SUID.  */
      if (__builtin_expect (check_trusted, 0))
	{
	  const char *trun = system_dirs;
	  size_t idx;
	  int unsecure = 1;

	  /* All trusted directories must be complete names.  */
	  if (cp[0] == '/')
	    {
	      for (idx = 0; idx < nsystem_dirs_len; ++idx)
		{
		  if (len == system_dirs_len[idx]
		      && memcmp (trun, cp, len) == 0)
		    {
		      /* Found it.  */
		      unsecure = 0;
		      break;
		    }

		  trun += system_dirs_len[idx] + 1;
		}
	    }

	  if (unsecure)
	    /* Simply drop this directory.  */
	    continue;
	}

      /* See if this directory is already known.  */
      for (dirp = GL(dl_all_dirs); dirp != NULL; dirp = dirp->next)
	if (dirp->dirnamelen == len && memcmp (cp, dirp->dirname, len) == 0)
	  break;

      if (dirp != NULL)
	{
	  /* It is available, see whether it's on our own list.  */
	  size_t cnt;
	  for (cnt = 0; cnt < nelems; ++cnt)
	    if (result[cnt] == dirp)
	      break;

	  if (cnt == nelems)
	    result[nelems++] = dirp;
	}
      else
	{
	  size_t cnt;
	  enum r_dir_status init_val;
	  size_t where_len = where ? strlen (where) + 1 : 0;

	  /* It's a new directory.  Create an entry and add it.  */
	  dirp = (struct r_search_path_elem *)
	    malloc (sizeof (*dirp) + ncapstr * sizeof (enum r_dir_status)
		    + where_len + len + 1);
	  if (dirp == NULL)
	    _dl_signal_error (ENOMEM, NULL, NULL,
			      N_("cannot create cache for search path"));

	  dirp->dirname = ((char *) dirp + sizeof (*dirp)
			   + ncapstr * sizeof (enum r_dir_status));
	  *((char *) __mempcpy ((char *) dirp->dirname, cp, len)) = '\0';
	  dirp->dirnamelen = len;

	  if (len > max_dirnamelen)
	    max_dirnamelen = len;

	  /* We have to make sure all the relative directories are
	     never ignored.  The current directory might change and
	     all our saved information would be void.  */
	  init_val = cp[0] != '/' ? existing : unknown;
	  for (cnt = 0; cnt < ncapstr; ++cnt)
	    dirp->status[cnt] = init_val;

	  dirp->what = what;
	  if (__builtin_expect (where != NULL, 1))
	    dirp->where = memcpy ((char *) dirp + sizeof (*dirp) + len + 1
				  + (ncapstr * sizeof (enum r_dir_status)),
				  where, where_len);
	  else
	    dirp->where = NULL;

	  dirp->next = GL(dl_all_dirs);
	  GL(dl_all_dirs) = dirp;

	  /* Put it in the result array.  */
	  result[nelems++] = dirp;
	}
    }

  /* Terminate the array.  */
  result[nelems] = NULL;

  return result;
}


static bool
internal_function
decompose_rpath (struct r_search_path_struct *sps,
		 const char *rpath, struct link_map *l, const char *what)
{
  /* Make a copy we can work with.  */
  const char *where = l->l_name;
  char *copy;
  char *cp;
  struct r_search_path_elem **result;
  size_t nelems;
  /* Initialize to please the compiler.  */
  const char *errstring = NULL;

  /* First see whether we must forget the RUNPATH and RPATH from this
     object.  */
  if (__builtin_expect (GLRO(dl_inhibit_rpath) != NULL, 0)
      && !INTUSE(__libc_enable_secure))
    {
      const char *inhp = GLRO(dl_inhibit_rpath);

      do
	{
	  const char *wp = where;

	  while (*inhp == *wp && *wp != '\0')
	    {
	      ++inhp;
	      ++wp;
	    }

	  if (*wp == '\0' && (*inhp == '\0' || *inhp == ':'))
	    {
	      /* This object is on the list of objects for which the
		 RUNPATH and RPATH must not be used.  */
	      sps->dirs = (void *) -1;
	      return false;
	    }

	  while (*inhp != '\0')
	    if (*inhp++ == ':')
	      break;
	}
      while (*inhp != '\0');
    }

  /* Make a writable copy.  At the same time expand possible dynamic
     string tokens.  */
  copy = expand_dynamic_string_token (l, rpath);
  if (copy == NULL)
    {
      errstring = N_("cannot create RUNPATH/RPATH copy");
      goto signal_error;
    }

  /* Count the number of necessary elements in the result array.  */
  nelems = 0;
  for (cp = copy; *cp != '\0'; ++cp)
    if (*cp == ':')
      ++nelems;

  /* Allocate room for the result.  NELEMS + 1 is an upper limit for the
     number of necessary entries.  */
  result = (struct r_search_path_elem **) malloc ((nelems + 1 + 1)
						  * sizeof (*result));
  if (result == NULL)
    {
      free (copy);
      errstring = N_("cannot create cache for search path");
    signal_error:
      _dl_signal_error (ENOMEM, NULL, NULL, errstring);
    }

  fillin_rpath (copy, result, ":", 0, what, where);

  /* Free the copied RPATH string.  `fillin_rpath' make own copies if
     necessary.  */
  free (copy);

  sps->dirs = result;
  /* The caller will change this value if we haven't used a real malloc.  */
  sps->malloced = 1;
  return true;
}

/* Make sure cached path information is stored in *SP
   and return true if there are any paths to search there.  */
static bool
cache_rpath (struct link_map *l,
	     struct r_search_path_struct *sp,
	     int tag,
	     const char *what)
{
  if (sp->dirs == (void *) -1)
    return false;

  if (sp->dirs != NULL)
    return true;

  if (l->l_info[tag] == NULL)
    {
      /* There is no path.  */
      sp->dirs = (void *) -1;
      return false;
    }

  /* Make sure the cache information is available.  */
  return decompose_rpath (sp, (const char *) (D_PTR (l, l_info[DT_STRTAB])
					      + l->l_info[tag]->d_un.d_val),
			  l, what);
}


void
internal_function
_dl_init_paths (const char *llp)
{
  size_t idx;
  const char *strp;
  struct r_search_path_elem *pelem, **aelem;
  size_t round_size;
#ifdef SHARED
  struct link_map *l;
#endif
  /* Initialize to please the compiler.  */
  const char *errstring = NULL;

  /* Fill in the information about the application's RPATH and the
     directories addressed by the LD_LIBRARY_PATH environment variable.  */

  /* Get the capabilities.  */
  capstr = _dl_important_hwcaps (GLRO(dl_platform), GLRO(dl_platformlen),
				 &ncapstr, &max_capstrlen);

  /* First set up the rest of the default search directory entries.  */
  aelem = rtld_search_dirs.dirs = (struct r_search_path_elem **)
    malloc ((nsystem_dirs_len + 1) * sizeof (struct r_search_path_elem *));
  if (rtld_search_dirs.dirs == NULL)
    {
      errstring = N_("cannot create search path array");
    signal_error:
      _dl_signal_error (ENOMEM, NULL, NULL, errstring);
    }

  round_size = ((2 * sizeof (struct r_search_path_elem) - 1
		 + ncapstr * sizeof (enum r_dir_status))
		/ sizeof (struct r_search_path_elem));

  rtld_search_dirs.dirs[0] = (struct r_search_path_elem *)
    malloc ((sizeof (system_dirs) / sizeof (system_dirs[0]))
	    * round_size * sizeof (struct r_search_path_elem));
  if (rtld_search_dirs.dirs[0] == NULL)
    {
      errstring = N_("cannot create cache for search path");
      goto signal_error;
    }

  rtld_search_dirs.malloced = 0;
  pelem = GL(dl_all_dirs) = rtld_search_dirs.dirs[0];
  strp = system_dirs;
  idx = 0;

  do
    {
      size_t cnt;

      *aelem++ = pelem;

      pelem->what = "system search path";
      pelem->where = NULL;

      pelem->dirname = strp;
      pelem->dirnamelen = system_dirs_len[idx];
      strp += system_dirs_len[idx] + 1;

      /* System paths must be absolute.  */
      assert (pelem->dirname[0] == '/');
      for (cnt = 0; cnt < ncapstr; ++cnt)
	pelem->status[cnt] = unknown;

      pelem->next = (++idx == nsystem_dirs_len ? NULL : (pelem + round_size));

      pelem += round_size;
    }
  while (idx < nsystem_dirs_len);

  max_dirnamelen = SYSTEM_DIRS_MAX_LEN;
  *aelem = NULL;

#ifdef SHARED
  /* This points to the map of the main object.  */
  l = GL(dl_ns)[LM_ID_BASE]._ns_loaded;
  if (l != NULL)
    {
      assert (l->l_type != lt_loaded);

      if (l->l_info[DT_RUNPATH])
	{
	  /* Allocate room for the search path and fill in information
	     from RUNPATH.  */
	  decompose_rpath (&l->l_runpath_dirs,
			   (const void *) (D_PTR (l, l_info[DT_STRTAB])
					   + l->l_info[DT_RUNPATH]->d_un.d_val),
			   l, "RUNPATH");

	  /* The RPATH is ignored.  */
	  l->l_rpath_dirs.dirs = (void *) -1;
	}
      else
	{
	  l->l_runpath_dirs.dirs = (void *) -1;

	  if (l->l_info[DT_RPATH])
	    {
	      /* Allocate room for the search path and fill in information
		 from RPATH.  */
	      decompose_rpath (&l->l_rpath_dirs,
			       (const void *) (D_PTR (l, l_info[DT_STRTAB])
					       + l->l_info[DT_RPATH]->d_un.d_val),
			       l, "RPATH");
	      l->l_rpath_dirs.malloced = 0;
	    }
	  else
	    l->l_rpath_dirs.dirs = (void *) -1;
	}
    }
#endif	/* SHARED */

  if (llp != NULL && *llp != '\0')
    {
      size_t nllp;
      const char *cp = llp;
      char *llp_tmp;

#ifdef SHARED
      /* Expand DSTs.  */
      size_t cnt = DL_DST_COUNT (llp, 1);
      if (__builtin_expect (cnt == 0, 1))
	llp_tmp = strdupa (llp);
      else
	{
	  /* Determine the length of the substituted string.  */
	  size_t total = DL_DST_REQUIRED (l, llp, strlen (llp), cnt);

	  /* Allocate the necessary memory.  */
	  llp_tmp = (char *) alloca (total + 1);
	  llp_tmp = _dl_dst_substitute (l, llp, llp_tmp, 1);
	}
#else
      llp_tmp = strdupa (llp);
#endif

      /* Decompose the LD_LIBRARY_PATH contents.  First determine how many
	 elements it has.  */
      nllp = 1;
      while (*cp)
	{
	  if (*cp == ':' || *cp == ';')
	    ++nllp;
	  ++cp;
	}

      env_path_list.dirs = (struct r_search_path_elem **)
	malloc ((nllp + 1) * sizeof (struct r_search_path_elem *));
      if (env_path_list.dirs == NULL)
	{
	  errstring = N_("cannot create cache for search path");
	  goto signal_error;
	}

      (void) fillin_rpath (llp_tmp, env_path_list.dirs, ":;",
			   INTUSE(__libc_enable_secure), "LD_LIBRARY_PATH",
			   NULL);

      if (env_path_list.dirs[0] == NULL)
	{
	  free (env_path_list.dirs);
	  env_path_list.dirs = (void *) -1;
	}

      env_path_list.malloced = 0;
    }
  else
    env_path_list.dirs = (void *) -1;

  /* Remember the last search directory added at startup.  */
  GLRO(dl_init_all_dirs) = GL(dl_all_dirs);
}


static void
__attribute__ ((noreturn, noinline))
lose (int code, int fd, const char *name, char *realname, struct link_map *l,
      const char *msg, struct r_debug *r)
{
  /* The file might already be closed.  */
  if (fd != -1)
    (void) __close (fd);
  if (l != NULL)
    {
      /* Remove the stillborn object from the list and free it.  */
      assert (l->l_next == NULL);
      if (l->l_prev == NULL)
	/* No other module loaded. This happens only in the static library,
	   or in rtld under --verify.  */
	GL(dl_ns)[l->l_ns]._ns_loaded = NULL;
      else
	l->l_prev->l_next = NULL;
      --GL(dl_ns)[l->l_ns]._ns_nloaded;
      free (l);
    }
  free (realname);

  if (r != NULL)
    {
      r->r_state = RT_CONSISTENT;
      _dl_debug_state ();
    }

  _dl_signal_error (code, name, NULL, msg);
}


/* Map in the shared object NAME, actually located in REALNAME, and already
   opened on FD.  */

#ifndef EXTERNAL_MAP_FROM_FD
static
#endif
struct link_map *
_dl_map_object_from_fd (const char *name, int fd, struct filebuf *fbp,
			char *realname, struct link_map *loader, int l_type,
			int mode, void **stack_endp, Lmid_t nsid)
{
  struct link_map *l = NULL;
  const ElfW(Ehdr) *header;
  const ElfW(Phdr) *phdr;
  const ElfW(Phdr) *ph;
  size_t maplength;
  int type;
  struct stat64 st;
  /* Initialize to keep the compiler happy.  */
  const char *errstring = NULL;
  int errval = 0;
  struct r_debug *r = _dl_debug_initialize (0, nsid);
  bool make_consistent = false;

  /* Get file information.  */
  if (__builtin_expect (__fxstat64 (_STAT_VER, fd, &st) < 0, 0))
    {
      errstring = N_("cannot stat shared object");
    call_lose_errno:
      errval = errno;
    call_lose:
      lose (errval, fd, name, realname, l, errstring,
	    make_consistent ? r : NULL);
    }

  /* Look again to see if the real name matched another already loaded.  */
  for (l = GL(dl_ns)[nsid]._ns_loaded; l; l = l->l_next)
    if (l->l_removed == 0 && l->l_ino == st.st_ino && l->l_dev == st.st_dev)
      {
	/* The object is already loaded.
	   Just bump its reference count and return it.  */
	__close (fd);

	/* If the name is not in the list of names for this object add
	   it.  */
	free (realname);
	add_name_to_object (l, name);

	return l;
      }

#ifdef SHARED
  /* When loading into a namespace other than the base one we must
     avoid loading ld.so since there can only be one copy.  Ever.  */
  if (__builtin_expect (nsid != LM_ID_BASE, 0)
      && ((st.st_ino == GL(dl_rtld_map).l_ino
	   && st.st_dev == GL(dl_rtld_map).l_dev)
	  || _dl_name_match_p (name, &GL(dl_rtld_map))))
    {
      /* This is indeed ld.so.  Create a new link_map which refers to
	 the real one for almost everything.  */
      l = _dl_new_object (realname, name, l_type, loader, mode, nsid);
      if (l == NULL)
	goto fail_new;

      /* Refer to the real descriptor.  */
      l->l_real = &GL(dl_rtld_map);

      /* No need to bump the refcount of the real object, ld.so will
	 never be unloaded.  */
      __close (fd);

      return l;
    }
#endif

  if (mode & RTLD_NOLOAD)
    {
      /* We are not supposed to load the object unless it is already
	 loaded.  So return now.  */
      __close (fd);
      return NULL;
    }

  /* Print debugging message.  */
  if (__builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_FILES, 0))
    _dl_debug_printf ("file=%s [%lu];  generating link map\n", name, nsid);

  /* This is the ELF header.  We read it in `open_verify'.  */
  header = (void *) fbp->buf;

#ifndef MAP_ANON
# define MAP_ANON 0
  if (_dl_zerofd == -1)
    {
      _dl_zerofd = _dl_sysdep_open_zero_fill ();
      if (_dl_zerofd == -1)
	{
	  __close (fd);
	  _dl_signal_error (errno, NULL, NULL,
			    N_("cannot open zero fill device"));
	}
    }
#endif

  /* Signal that we are going to add new objects.  */
  if (r->r_state == RT_CONSISTENT)
    {
#ifdef SHARED
      /* Auditing checkpoint: we are going to add new objects.  */
      if ((mode & __RTLD_AUDIT) == 0
	  && __builtin_expect (GLRO(dl_naudit) > 0, 0))
	{
	  struct link_map *head = GL(dl_ns)[nsid]._ns_loaded;
	  /* Do not call the functions for any auditing object.  */
	  if (head->l_auditing == 0)
	    {
	      struct audit_ifaces *afct = GLRO(dl_audit);
	      for (unsigned int cnt = 0; cnt < GLRO(dl_naudit); ++cnt)
		{
		  if (afct->activity != NULL)
		    afct->activity (&head->l_audit[cnt].cookie, LA_ACT_ADD);

		  afct = afct->next;
		}
	    }
	}
#endif

      /* Notify the debugger we have added some objects.  We need to
	 call _dl_debug_initialize in a static program in case dynamic
	 linking has not been used before.  */
      r->r_state = RT_ADD;
      _dl_debug_state ();
      make_consistent = true;
    }
  else
    assert (r->r_state == RT_ADD);

  /* Enter the new object in the list of loaded objects.  */
  l = _dl_new_object (realname, name, l_type, loader, mode, nsid);
  if (__builtin_expect (l == NULL, 0))
    {
#ifdef SHARED
    fail_new:
#endif
      errstring = N_("cannot create shared object descriptor");
      goto call_lose_errno;
    }

  /* Extract the remaining details we need from the ELF header
     and then read in the program header table.  */
  l->l_entry = header->e_entry;
  type = header->e_type;
  l->l_phnum = header->e_phnum;

  maplength = header->e_phnum * sizeof (ElfW(Phdr));
  if (header->e_phoff + maplength <= (size_t) fbp->len)
    phdr = (void *) (fbp->buf + header->e_phoff);
  else
    {
      phdr = alloca (maplength);
      __lseek (fd, header->e_phoff, SEEK_SET);
      if ((size_t) __libc_read (fd, (void *) phdr, maplength) != maplength)
	{
	  errstring = N_("cannot read file data");
	  goto call_lose_errno;
	}
    }

  /* Presumed absent PT_GNU_STACK.  */
  uint_fast16_t stack_flags = PF_R|PF_W|PF_X;

  {
    /* Scan the program header table, collecting its load commands.  */
    struct loadcmd
      {
	ElfW(Addr) mapstart, mapend, dataend, allocend;
	off_t mapoff;
	int prot;
      } loadcmds[l->l_phnum], *c;
    size_t nloadcmds = 0;
    bool has_holes = false;

    /* The struct is initialized to zero so this is not necessary:
    l->l_ld = 0;
    l->l_phdr = 0;
    l->l_addr = 0; */
    for (ph = phdr; ph < &phdr[l->l_phnum]; ++ph)
      switch (ph->p_type)
	{
	  /* These entries tell us where to find things once the file's
	     segments are mapped in.  We record the addresses it says
	     verbatim, and later correct for the run-time load address.  */
	case PT_DYNAMIC:
	  l->l_ld = (void *) ph->p_vaddr;
	  l->l_ldnum = ph->p_memsz / sizeof (ElfW(Dyn));
	  break;

	case PT_PHDR:
	  l->l_phdr = (void *) ph->p_vaddr;
	  break;

	case PT_LOAD:
	  /* A load command tells us to map in part of the file.
	     We record the load commands and process them all later.  */
	  if (__builtin_expect ((ph->p_align & (GLRO(dl_pagesize) - 1)) != 0,
				0))
	    {
	      errstring = N_("ELF load command alignment not page-aligned");
	      goto call_lose;
	    }
	  if (__builtin_expect (((ph->p_vaddr - ph->p_offset)
				 & (ph->p_align - 1)) != 0, 0))
	    {
	      errstring
		= N_("ELF load command address/offset not properly aligned");
	      goto call_lose;
	    }

	  c = &loadcmds[nloadcmds++];
	  c->mapstart = ph->p_vaddr & ~(GLRO(dl_pagesize) - 1);
	  c->mapend = ((ph->p_vaddr + ph->p_filesz + GLRO(dl_pagesize) - 1)
		       & ~(GLRO(dl_pagesize) - 1));
	  c->dataend = ph->p_vaddr + ph->p_filesz;
	  c->allocend = ph->p_vaddr + ph->p_memsz;
	  c->mapoff = ph->p_offset & ~(GLRO(dl_pagesize) - 1);

	  /* Determine whether there is a gap between the last segment
	     and this one.  */
	  if (nloadcmds > 1 && c[-1].mapend != c->mapstart)
	    has_holes = true;

	  /* Optimize a common case.  */
#if (PF_R | PF_W | PF_X) == 7 && (PROT_READ | PROT_WRITE | PROT_EXEC) == 7
	  c->prot = (PF_TO_PROT
		     >> ((ph->p_flags & (PF_R | PF_W | PF_X)) * 4)) & 0xf;
#else
	  c->prot = 0;
	  if (ph->p_flags & PF_R)
	    c->prot |= PROT_READ;
	  if (ph->p_flags & PF_W)
	    c->prot |= PROT_WRITE;
	  if (ph->p_flags & PF_X)
	    c->prot |= PROT_EXEC;
#endif
	  break;

	case PT_TLS:
	  if (ph->p_memsz == 0)
	    /* Nothing to do for an empty segment.  */
	    break;

	  l->l_tls_blocksize = ph->p_memsz;
	  l->l_tls_align = ph->p_align;
	  if (ph->p_align == 0)
	    l->l_tls_firstbyte_offset = 0;
	  else
	    l->l_tls_firstbyte_offset = ph->p_vaddr & (ph->p_align - 1);
	  l->l_tls_initimage_size = ph->p_filesz;
	  /* Since we don't know the load address yet only store the
	     offset.  We will adjust it later.  */
	  l->l_tls_initimage = (void *) ph->p_vaddr;

	  /* If not loading the initial set of shared libraries,
	     check whether we should permit loading a TLS segment.  */
	  if (__builtin_expect (l->l_type == lt_library, 1)
	      /* If GL(dl_tls_dtv_slotinfo_list) == NULL, then rtld.c did
		 not set up TLS data structures, so don't use them now.  */
	      || __builtin_expect (GL(dl_tls_dtv_slotinfo_list) != NULL, 1))
	    {
	      /* Assign the next available module ID.  */
	      l->l_tls_modid = _dl_next_tls_modid ();
	      break;
	    }

#ifdef SHARED
	  if (l->l_prev == NULL || (mode & __RTLD_AUDIT) != 0)
	    /* We are loading the executable itself when the dynamic linker
	       was executed directly.  The setup will happen later.  */
	    break;

	  /* In a static binary there is no way to tell if we dynamically
	     loaded libpthread.  */
	  if (GL(dl_error_catch_tsd) == &_dl_initial_error_catch_tsd)
#endif
	    {
	      /* We have not yet loaded libpthread.
		 We can do the TLS setup right now!  */

	      void *tcb;

	      /* The first call allocates TLS bookkeeping data structures.
		 Then we allocate the TCB for the initial thread.  */
	      if (__builtin_expect (_dl_tls_setup (), 0)
		  || __builtin_expect ((tcb = _dl_allocate_tls (NULL)) == NULL,
				       0))
		{
		  errval = ENOMEM;
		  errstring = N_("\
cannot allocate TLS data structures for initial thread");
		  goto call_lose;
		}

	      /* Now we install the TCB in the thread register.  */
	      errstring = TLS_INIT_TP (tcb, 0);
	      if (__builtin_expect (errstring == NULL, 1))
		{
		  /* Now we are all good.  */
		  l->l_tls_modid = ++GL(dl_tls_max_dtv_idx);
		  break;
		}

	      /* The kernel is too old or somesuch.  */
	      errval = 0;
	      _dl_deallocate_tls (tcb, 1);
	      goto call_lose;
	    }

	  /* Uh-oh, the binary expects TLS support but we cannot
	     provide it.  */
	  errval = 0;
	  errstring = N_("cannot handle TLS data");
	  goto call_lose;
	  break;

	case PT_GNU_STACK:
	  stack_flags = ph->p_flags;
	  break;

	case PT_GNU_RELRO:
	  l->l_relro_addr = ph->p_vaddr;
	  l->l_relro_size = ph->p_memsz;
	  break;
	}

    if (__builtin_expect (nloadcmds == 0, 0))
      {
	/* This only happens for a bogus object that will be caught with
	   another error below.  But we don't want to go through the
	   calculations below using NLOADCMDS - 1.  */
	errstring = N_("object file has no loadable segments");
	goto call_lose;
      }

    /* Now process the load commands and map segments into memory.  */
    c = loadcmds;

    /* Length of the sections to be loaded.  */
    maplength = loadcmds[nloadcmds - 1].allocend - c->mapstart;

    if (__builtin_expect (type, ET_DYN) == ET_DYN)
      {
	/* This is a position-independent shared object.  We can let the
	   kernel map it anywhere it likes, but we must have space for all
	   the segments in their specified positions relative to the first.
	   So we map the first segment without MAP_FIXED, but with its
	   extent increased to cover all the segments.  Then we remove
	   access from excess portion, and there is known sufficient space
	   there to remap from the later segments.

	   As a refinement, sometimes we have an address that we would
	   prefer to map such objects at; but this is only a preference,
	   the OS can do whatever it likes. */
	ElfW(Addr) mappref;
	mappref = (ELF_PREFERRED_ADDRESS (loader, maplength,
					  c->mapstart & GLRO(dl_use_load_bias))
		   - MAP_BASE_ADDR (l));

	/* Remember which part of the address space this object uses.  */
	l->l_map_start = (ElfW(Addr)) __mmap ((void *) mappref, maplength,
					      c->prot,
					      MAP_COPY|MAP_FILE,
					      fd, c->mapoff);
	if (__builtin_expect ((void *) l->l_map_start == MAP_FAILED, 0))
	  {
	  map_error:
	    errstring = N_("failed to map segment from shared object");
	    goto call_lose_errno;
	  }

	l->l_map_end = l->l_map_start + maplength;
	l->l_addr = l->l_map_start - c->mapstart;

	if (has_holes)
	  /* Change protection on the excess portion to disallow all access;
	     the portions we do not remap later will be inaccessible as if
	     unallocated.  Then jump into the normal segment-mapping loop to
	     handle the portion of the segment past the end of the file
	     mapping.  */
	  __mprotect ((caddr_t) (l->l_addr + c->mapend),
		      loadcmds[nloadcmds - 1].mapstart - c->mapend,
		      PROT_NONE);

	l->l_contiguous = 1;

	goto postmap;
      }

    /* This object is loaded at a fixed address.  This must never
       happen for objects loaded with dlopen().  */
    if (__builtin_expect ((mode & __RTLD_OPENEXEC) == 0, 0))
      {
	errstring = N_("cannot dynamically load executable");
	goto call_lose;
      }

    /* Notify ELF_PREFERRED_ADDRESS that we have to load this one
       fixed.  */
    ELF_FIXED_ADDRESS (loader, c->mapstart);


    /* Remember which part of the address space this object uses.  */
    l->l_map_start = c->mapstart + l->l_addr;
    l->l_map_end = l->l_map_start + maplength;
    l->l_contiguous = !has_holes;

    while (c < &loadcmds[nloadcmds])
      {
	if (c->mapend > c->mapstart
	    /* Map the segment contents from the file.  */
	    && (__mmap ((void *) (l->l_addr + c->mapstart),
			c->mapend - c->mapstart, c->prot,
			MAP_FIXED|MAP_COPY|MAP_FILE,
			fd, c->mapoff)
		== MAP_FAILED))
	  goto map_error;

      postmap:
	if (c->prot & PROT_EXEC)
	  l->l_text_end = l->l_addr + c->mapend;

	if (l->l_phdr == 0
	    && (ElfW(Off)) c->mapoff <= header->e_phoff
	    && ((size_t) (c->mapend - c->mapstart + c->mapoff)
		>= header->e_phoff + header->e_phnum * sizeof (ElfW(Phdr))))
	  /* Found the program header in this segment.  */
	  l->l_phdr = (void *) (c->mapstart + header->e_phoff - c->mapoff);

	if (c->allocend > c->dataend)
	  {
	    /* Extra zero pages should appear at the end of this segment,
	       after the data mapped from the file.   */
	    ElfW(Addr) zero, zeroend, zeropage;

	    zero = l->l_addr + c->dataend;
	    zeroend = l->l_addr + c->allocend;
	    zeropage = ((zero + GLRO(dl_pagesize) - 1)
			& ~(GLRO(dl_pagesize) - 1));

	    if (zeroend < zeropage)
	      /* All the extra data is in the last page of the segment.
		 We can just zero it.  */
	      zeropage = zeroend;

	    if (zeropage > zero)
	      {
		/* Zero the final part of the last page of the segment.  */
		if (__builtin_expect ((c->prot & PROT_WRITE) == 0, 0))
		  {
		    /* Dag nab it.  */
		    if (__mprotect ((caddr_t) (zero
					       & ~(GLRO(dl_pagesize) - 1)),
				    GLRO(dl_pagesize), c->prot|PROT_WRITE) < 0)
		      {
			errstring = N_("cannot change memory protections");
			goto call_lose_errno;
		      }
		  }
		memset ((void *) zero, '\0', zeropage - zero);
		if (__builtin_expect ((c->prot & PROT_WRITE) == 0, 0))
		  __mprotect ((caddr_t) (zero & ~(GLRO(dl_pagesize) - 1)),
			      GLRO(dl_pagesize), c->prot);
	      }

	    if (zeroend > zeropage)
	      {
		/* Map the remaining zero pages in from the zero fill FD.  */
		caddr_t mapat;
		mapat = __mmap ((caddr_t) zeropage, zeroend - zeropage,
				c->prot, MAP_ANON|MAP_PRIVATE|MAP_FIXED,
				-1, 0);
		if (__builtin_expect (mapat == MAP_FAILED, 0))
		  {
		    errstring = N_("cannot map zero-fill pages");
		    goto call_lose_errno;
		  }
	      }
	  }

	++c;
      }
  }

  if (l->l_ld == 0)
    {
      if (__builtin_expect (type == ET_DYN, 0))
	{
	  errstring = N_("object file has no dynamic section");
	  goto call_lose;
	}
    }
  else
    l->l_ld = (ElfW(Dyn) *) ((ElfW(Addr)) l->l_ld + l->l_addr);

  elf_get_dynamic_info (l, NULL);

  /* Make sure we are not dlopen'ing an object that has the
     DF_1_NOOPEN flag set.  */
  if (__builtin_expect (l->l_flags_1 & DF_1_NOOPEN, 0)
      && (mode & __RTLD_DLOPEN))
    {
      /* We are not supposed to load this object.  Free all resources.  */
      __munmap ((void *) l->l_map_start, l->l_map_end - l->l_map_start);

      if (!l->l_libname->dont_free)
	free (l->l_libname);

      if (l->l_phdr_allocated)
	free ((void *) l->l_phdr);

      errstring = N_("shared object cannot be dlopen()ed");
      goto call_lose;
    }

  if (l->l_phdr == NULL)
    {
      /* The program header is not contained in any of the segments.
	 We have to allocate memory ourself and copy it over from out
	 temporary place.  */
      ElfW(Phdr) *newp = (ElfW(Phdr) *) malloc (header->e_phnum
						* sizeof (ElfW(Phdr)));
      if (newp == NULL)
	{
	  errstring = N_("cannot allocate memory for program header");
	  goto call_lose_errno;
	}

      l->l_phdr = memcpy (newp, phdr,
			  (header->e_phnum * sizeof (ElfW(Phdr))));
      l->l_phdr_allocated = 1;
    }
  else
    /* Adjust the PT_PHDR value by the runtime load address.  */
    l->l_phdr = (ElfW(Phdr) *) ((ElfW(Addr)) l->l_phdr + l->l_addr);

  if (__builtin_expect ((stack_flags &~ GL(dl_stack_flags)) & PF_X, 0))
    {
      if (__builtin_expect (__check_caller (RETURN_ADDRESS (0), allow_ldso),
			    0) != 0)
	{
	  errstring = N_("invalid caller");
	  goto call_lose;
	}

      /* The stack is presently not executable, but this module
	 requires that it be executable.  We must change the
	 protection of the variable which contains the flags used in
	 the mprotect calls.  */
#ifdef SHARED
      if ((mode & (__RTLD_DLOPEN | __RTLD_AUDIT)) == __RTLD_DLOPEN)
	{
	  const uintptr_t p = (uintptr_t) &__stack_prot & -GLRO(dl_pagesize);
	  const size_t s = (uintptr_t) (&__stack_prot + 1) - p;

	  struct link_map *const m = &GL(dl_rtld_map);
	  const uintptr_t relro_end = ((m->l_addr + m->l_relro_addr
					+ m->l_relro_size)
				       & -GLRO(dl_pagesize));
	  if (__builtin_expect (p + s <= relro_end, 1))
	    {
	      /* The variable lies in the region protected by RELRO.  */
	      __mprotect ((void *) p, s, PROT_READ|PROT_WRITE);
	      __stack_prot |= PROT_READ|PROT_WRITE|PROT_EXEC;
	      __mprotect ((void *) p, s, PROT_READ);
	    }
	  else
	    __stack_prot |= PROT_READ|PROT_WRITE|PROT_EXEC;
	}
      else
#endif
	__stack_prot |= PROT_READ|PROT_WRITE|PROT_EXEC;

#ifdef check_consistency
      check_consistency ();
#endif

      errval = (*GL(dl_make_stack_executable_hook)) (stack_endp);
      if (errval)
	{
	  errstring = N_("\
cannot enable executable stack as shared object requires");
	  goto call_lose;
	}
    }

  /* Adjust the address of the TLS initialization image.  */
  if (l->l_tls_initimage != NULL)
    l->l_tls_initimage = (char *) l->l_tls_initimage + l->l_addr;

  /* We are done mapping in the file.  We no longer need the descriptor.  */
  if (__builtin_expect (__close (fd) != 0, 0))
    {
      errstring = N_("cannot close file descriptor");
      goto call_lose_errno;
    }
  /* Signal that we closed the file.  */
  fd = -1;

  if (l->l_type == lt_library && type == ET_EXEC)
    l->l_type = lt_executable;

  l->l_entry += l->l_addr;

  if (__builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_FILES, 0))
    _dl_debug_printf ("\
  dynamic: 0x%0*lx  base: 0x%0*lx   size: 0x%0*Zx\n\
    entry: 0x%0*lx  phdr: 0x%0*lx  phnum:   %*u\n\n",
			   (int) sizeof (void *) * 2,
			   (unsigned long int) l->l_ld,
			   (int) sizeof (void *) * 2,
			   (unsigned long int) l->l_addr,
			   (int) sizeof (void *) * 2, maplength,
			   (int) sizeof (void *) * 2,
			   (unsigned long int) l->l_entry,
			   (int) sizeof (void *) * 2,
			   (unsigned long int) l->l_phdr,
			   (int) sizeof (void *) * 2, l->l_phnum);

  /* Set up the symbol hash table.  */
  _dl_setup_hash (l);

  /* If this object has DT_SYMBOLIC set modify now its scope.  We don't
     have to do this for the main map.  */
  if ((mode & RTLD_DEEPBIND) == 0
      && __builtin_expect (l->l_info[DT_SYMBOLIC] != NULL, 0)
      && &l->l_searchlist != l->l_scope[0])
    {
      /* Create an appropriate searchlist.  It contains only this map.
	 This is the definition of DT_SYMBOLIC in SysVr4.  */
      l->l_symbolic_searchlist.r_list[0] = l;
      l->l_symbolic_searchlist.r_nlist = 1;

      /* Now move the existing entries one back.  */
      memmove (&l->l_scope[1], &l->l_scope[0],
	       (l->l_scope_max - 1) * sizeof (l->l_scope[0]));

      /* Now add the new entry.  */
      l->l_scope[0] = &l->l_symbolic_searchlist;
    }

  /* Remember whether this object must be initialized first.  */
  if (l->l_flags_1 & DF_1_INITFIRST)
    GL(dl_initfirst) = l;

  /* Finally the file information.  */
  l->l_dev = st.st_dev;
  l->l_ino = st.st_ino;

  /* When we profile the SONAME might be needed for something else but
     loading.  Add it right away.  */
  if (__builtin_expect (GLRO(dl_profile) != NULL, 0)
      && l->l_info[DT_SONAME] != NULL)
    add_name_to_object (l, ((const char *) D_PTR (l, l_info[DT_STRTAB])
			    + l->l_info[DT_SONAME]->d_un.d_val));

#ifdef SHARED
  /* Auditing checkpoint: we have a new object.  */
  if (__builtin_expect (GLRO(dl_naudit) > 0, 0)
      && !GL(dl_ns)[l->l_ns]._ns_loaded->l_auditing)
    {
      struct audit_ifaces *afct = GLRO(dl_audit);
      for (unsigned int cnt = 0; cnt < GLRO(dl_naudit); ++cnt)
	{
	  if (afct->objopen != NULL)
	    {
	      l->l_audit[cnt].bindflags
		= afct->objopen (l, nsid, &l->l_audit[cnt].cookie);

	      l->l_audit_any_plt |= l->l_audit[cnt].bindflags != 0;
	    }

	  afct = afct->next;
	}
    }
#endif

  return l;
}

/* Print search path.  */
static void
print_search_path (struct r_search_path_elem **list,
                   const char *what, const char *name)
{
  char buf[max_dirnamelen + max_capstrlen];
  int first = 1;

  _dl_debug_printf (" search path=");

  while (*list != NULL && (*list)->what == what) /* Yes, ==.  */
    {
      char *endp = __mempcpy (buf, (*list)->dirname, (*list)->dirnamelen);
      size_t cnt;

      for (cnt = 0; cnt < ncapstr; ++cnt)
	if ((*list)->status[cnt] != nonexisting)
	  {
	    char *cp = __mempcpy (endp, capstr[cnt].str, capstr[cnt].len);
	    if (cp == buf || (cp == buf + 1 && buf[0] == '/'))
	      cp[0] = '\0';
	    else
	      cp[-1] = '\0';

	    _dl_debug_printf_c (first ? "%s" : ":%s", buf);
	    first = 0;
	  }

      ++list;
    }

  if (name != NULL)
    _dl_debug_printf_c ("\t\t(%s from file %s)\n", what,
			name[0] ? name : rtld_progname);
  else
    _dl_debug_printf_c ("\t\t(%s)\n", what);
}

/* Open a file and verify it is an ELF file for this architecture.  We
   ignore only ELF files for other architectures.  Non-ELF files and
   ELF files with different header information cause fatal errors since
   this could mean there is something wrong in the installation and the
   user might want to know about this.  */
static int
open_verify (const char *name, struct filebuf *fbp, struct link_map *loader,
	     int whatcode, bool *found_other_class, bool free_name)
{
  /* This is the expected ELF header.  */
#define ELF32_CLASS ELFCLASS32
#define ELF64_CLASS ELFCLASS64
#ifndef VALID_ELF_HEADER
# define VALID_ELF_HEADER(hdr,exp,size)	(memcmp (hdr, exp, size) == 0)
# define VALID_ELF_OSABI(osabi)		(osabi == ELFOSABI_SYSV)
# define VALID_ELF_ABIVERSION(ver)	(ver == 0)
#elif defined MORE_ELF_HEADER_DATA
  MORE_ELF_HEADER_DATA;
#endif
  static const unsigned char expected[EI_PAD] =
  {
    [EI_MAG0] = ELFMAG0,
    [EI_MAG1] = ELFMAG1,
    [EI_MAG2] = ELFMAG2,
    [EI_MAG3] = ELFMAG3,
    [EI_CLASS] = ELFW(CLASS),
    [EI_DATA] = byteorder,
    [EI_VERSION] = EV_CURRENT,
    [EI_OSABI] = ELFOSABI_SYSV,
    [EI_ABIVERSION] = 0
  };
  static const struct
  {
    ElfW(Word) vendorlen;
    ElfW(Word) datalen;
    ElfW(Word) type;
    char vendor[4];
  } expected_note = { 4, 16, 1, "GNU" };
  /* Initialize it to make the compiler happy.  */
  const char *errstring = NULL;
  int errval = 0;

#ifdef SHARED
  /* Give the auditing libraries a chance.  */
  if (__builtin_expect (GLRO(dl_naudit) > 0, 0) && whatcode != 0
      && loader->l_auditing == 0)
    {
      struct audit_ifaces *afct = GLRO(dl_audit);
      for (unsigned int cnt = 0; cnt < GLRO(dl_naudit); ++cnt)
	{
	  if (afct->objsearch != NULL)
	    {
	      name = afct->objsearch (name, &loader->l_audit[cnt].cookie,
				      whatcode);
	      if (name == NULL)
		/* Ignore the path.  */
		return -1;
	    }

	  afct = afct->next;
	}
    }
#endif

  /* Open the file.  We always open files read-only.  */
  int fd = __open (name, O_RDONLY);
  if (fd != -1)
    {
      ElfW(Ehdr) *ehdr;
      ElfW(Phdr) *phdr, *ph;
      ElfW(Word) *abi_note;
      unsigned int osversion;
      size_t maplength;

      /* We successfully openened the file.  Now verify it is a file
	 we can use.  */
      __set_errno (0);
      fbp->len = __libc_read (fd, fbp->buf, sizeof (fbp->buf));

      /* This is where the ELF header is loaded.  */
      assert (sizeof (fbp->buf) > sizeof (ElfW(Ehdr)));
      ehdr = (ElfW(Ehdr) *) fbp->buf;

      /* Now run the tests.  */
      if (__builtin_expect (fbp->len < (ssize_t) sizeof (ElfW(Ehdr)), 0))
	{
	  errval = errno;
	  errstring = (errval == 0
		       ? N_("file too short") : N_("cannot read file data"));
	call_lose:
	  if (free_name)
	    {
	      char *realname = (char *) name;
	      name = strdupa (realname);
	      free (realname);
	    }
	  lose (errval, fd, name, NULL, NULL, errstring, NULL);
	}

      /* See whether the ELF header is what we expect.  */
      if (__builtin_expect (! VALID_ELF_HEADER (ehdr->e_ident, expected,
						EI_PAD), 0))
	{
	  /* Something is wrong.  */
	  const Elf32_Word *magp = (const void *) ehdr->e_ident;
	  if (*magp !=
#if BYTE_ORDER == LITTLE_ENDIAN
	      ((ELFMAG0 << (EI_MAG0 * 8)) |
	       (ELFMAG1 << (EI_MAG1 * 8)) |
	       (ELFMAG2 << (EI_MAG2 * 8)) |
	       (ELFMAG3 << (EI_MAG3 * 8)))
#else
	      ((ELFMAG0 << (EI_MAG3 * 8)) |
	       (ELFMAG1 << (EI_MAG2 * 8)) |
	       (ELFMAG2 << (EI_MAG1 * 8)) |
	       (ELFMAG3 << (EI_MAG0 * 8)))
#endif
	      )
	    errstring = N_("invalid ELF header");
	  else if (ehdr->e_ident[EI_CLASS] != ELFW(CLASS))
	    {
	      /* This is not a fatal error.  On architectures where
		 32-bit and 64-bit binaries can be run this might
		 happen.  */
	      *found_other_class = true;
	      goto close_and_out;
	    }
	  else if (ehdr->e_ident[EI_DATA] != byteorder)
	    {
	      if (BYTE_ORDER == BIG_ENDIAN)
		errstring = N_("ELF file data encoding not big-endian");
	      else
		errstring = N_("ELF file data encoding not little-endian");
	    }
	  else if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
	    errstring
	      = N_("ELF file version ident does not match current one");
	  /* XXX We should be able so set system specific versions which are
	     allowed here.  */
	  else if (!VALID_ELF_OSABI (ehdr->e_ident[EI_OSABI]))
	    errstring = N_("ELF file OS ABI invalid");
	  else if (!VALID_ELF_ABIVERSION (ehdr->e_ident[EI_ABIVERSION]))
	    errstring = N_("ELF file ABI version invalid");
	  else
	    /* Otherwise we don't know what went wrong.  */
	    errstring = N_("internal error");

	  goto call_lose;
	}

      if (__builtin_expect (ehdr->e_version, EV_CURRENT) != EV_CURRENT)
	{
	  errstring = N_("ELF file version does not match current one");
	  goto call_lose;
	}
      if (! __builtin_expect (elf_machine_matches_host (ehdr), 1))
	goto close_and_out;
      else if (__builtin_expect (ehdr->e_type, ET_DYN) != ET_DYN
	       && __builtin_expect (ehdr->e_type, ET_EXEC) != ET_EXEC)
	{
	  errstring = N_("only ET_DYN and ET_EXEC can be loaded");
	  goto call_lose;
	}
      else if (__builtin_expect (ehdr->e_phentsize, sizeof (ElfW(Phdr)))
	       != sizeof (ElfW(Phdr)))
	{
	  errstring = N_("ELF file's phentsize not the expected size");
	  goto call_lose;
	}

      maplength = ehdr->e_phnum * sizeof (ElfW(Phdr));
      if (ehdr->e_phoff + maplength <= (size_t) fbp->len)
	phdr = (void *) (fbp->buf + ehdr->e_phoff);
      else
	{
	  phdr = alloca (maplength);
	  __lseek (fd, ehdr->e_phoff, SEEK_SET);
	  if ((size_t) __libc_read (fd, (void *) phdr, maplength) != maplength)
	    {
	    read_error:
	      errval = errno;
	      errstring = N_("cannot read file data");
	      goto call_lose;
	    }
	}

      /* Check .note.ABI-tag if present.  */
      for (ph = phdr; ph < &phdr[ehdr->e_phnum]; ++ph)
	if (ph->p_type == PT_NOTE && ph->p_filesz >= 32 && ph->p_align >= 4)
	  {
	    ElfW(Addr) size = ph->p_filesz;

	    if (ph->p_offset + size <= (size_t) fbp->len)
	      abi_note = (void *) (fbp->buf + ph->p_offset);
	    else
	      {
		abi_note = alloca (size);
		__lseek (fd, ph->p_offset, SEEK_SET);
		if (__libc_read (fd, (void *) abi_note, size) != size)
		  goto read_error;
	      }

	    while (memcmp (abi_note, &expected_note, sizeof (expected_note)))
	      {
#define ROUND(len) (((len) + sizeof (ElfW(Word)) - 1) & -sizeof (ElfW(Word)))
		ElfW(Addr) note_size = 3 * sizeof (ElfW(Word))
				       + ROUND (abi_note[0])
				       + ROUND (abi_note[1]);

		if (size - 32 < note_size)
		  {
		    size = 0;
		    break;
		  }
		size -= note_size;
		abi_note = (void *) abi_note + note_size;
	      }

	    if (size == 0)
	      continue;

	    osversion = (abi_note[5] & 0xff) * 65536
			+ (abi_note[6] & 0xff) * 256
			+ (abi_note[7] & 0xff);
	    if (abi_note[4] != __ABI_TAG_OS
		|| (GLRO(dl_osversion) && GLRO(dl_osversion) < osversion))
	      {
	      close_and_out:
		__close (fd);
		__set_errno (ENOENT);
		fd = -1;
	      }

	    break;
	  }
    }

  return fd;
}

/* Try to open NAME in one of the directories in *DIRSP.
   Return the fd, or -1.  If successful, fill in *REALNAME
   with the malloc'd full directory name.  If it turns out
   that none of the directories in *DIRSP exists, *DIRSP is
   replaced with (void *) -1, and the old value is free()d
   if MAY_FREE_DIRS is true.  */

static int
open_path (const char *name, size_t namelen, int preloaded,
	   struct r_search_path_struct *sps, char **realname,
	   struct filebuf *fbp, struct link_map *loader, int whatcode,
	   bool *found_other_class)
{
  struct r_search_path_elem **dirs = sps->dirs;
  char *buf;
  int fd = -1;
  const char *current_what = NULL;
  int any = 0;

  if (__builtin_expect (dirs == NULL, 0))
    /* We're called before _dl_init_paths when loading the main executable
       given on the command line when rtld is run directly.  */
    return -1;

  buf = alloca (max_dirnamelen + max_capstrlen + namelen);
  do
    {
      struct r_search_path_elem *this_dir = *dirs;
      size_t buflen = 0;
      size_t cnt;
      char *edp;
      int here_any = 0;
      int err;

      /* If we are debugging the search for libraries print the path
	 now if it hasn't happened now.  */
      if (__builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_LIBS, 0)
	  && current_what != this_dir->what)
	{
	  current_what = this_dir->what;
	  print_search_path (dirs, current_what, this_dir->where);
	}

      edp = (char *) __mempcpy (buf, this_dir->dirname, this_dir->dirnamelen);
      for (cnt = 0; fd == -1 && cnt < ncapstr; ++cnt)
	{
	  /* Skip this directory if we know it does not exist.  */
	  if (this_dir->status[cnt] == nonexisting)
	    continue;

	  buflen =
	    ((char *) __mempcpy (__mempcpy (edp, capstr[cnt].str,
					    capstr[cnt].len),
				 name, namelen)
	     - buf);

	  /* Print name we try if this is wanted.  */
	  if (__builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_LIBS, 0))
	    _dl_debug_printf ("  trying file=%s\n", buf);

	  fd = open_verify (buf, fbp, loader, whatcode, found_other_class,
			    false);
	  if (this_dir->status[cnt] == unknown)
	    {
	      if (fd != -1)
		this_dir->status[cnt] = existing;
	      /* Do not update the directory information when loading
		 auditing code.  We must try to disturb the program as
		 little as possible.  */
	      else if (loader == NULL
		       || GL(dl_ns)[loader->l_ns]._ns_loaded->l_auditing == 0)
		{
		  /* We failed to open machine dependent library.  Let's
		     test whether there is any directory at all.  */
		  struct stat64 st;

		  buf[buflen - namelen - 1] = '\0';

		  if (__xstat64 (_STAT_VER, buf, &st) != 0
		      || ! S_ISDIR (st.st_mode))
		    /* The directory does not exist or it is no directory.  */
		    this_dir->status[cnt] = nonexisting;
		  else
		    this_dir->status[cnt] = existing;
		}
	    }

	  /* Remember whether we found any existing directory.  */
	  here_any |= this_dir->status[cnt] != nonexisting;

	  if (fd != -1 && __builtin_expect (preloaded, 0)
	      && INTUSE(__libc_enable_secure))
	    {
	      /* This is an extra security effort to make sure nobody can
		 preload broken shared objects which are in the trusted
		 directories and so exploit the bugs.  */
	      struct stat64 st;

	      if (__fxstat64 (_STAT_VER, fd, &st) != 0
		  || (st.st_mode & S_ISUID) == 0)
		{
		  /* The shared object cannot be tested for being SUID
		     or this bit is not set.  In this case we must not
		     use this object.  */
		  __close (fd);
		  fd = -1;
		  /* We simply ignore the file, signal this by setting
		     the error value which would have been set by `open'.  */
		  errno = ENOENT;
		}
	    }
	}

      if (fd != -1)
	{
	  *realname = (char *) malloc (buflen);
	  if (*realname != NULL)
	    {
	      memcpy (*realname, buf, buflen);
	      return fd;
	    }
	  else
	    {
	      /* No memory for the name, we certainly won't be able
		 to load and link it.  */
	      __close (fd);
	      return -1;
	    }
	}
      if (here_any && (err = errno) != ENOENT && err != EACCES)
	/* The file exists and is readable, but something went wrong.  */
	return -1;

      /* Remember whether we found anything.  */
      any |= here_any;
    }
  while (*++dirs != NULL);

  /* Remove the whole path if none of the directories exists.  */
  if (__builtin_expect (! any, 0))
    {
      /* Paths which were allocated using the minimal malloc() in ld.so
	 must not be freed using the general free() in libc.  */
      if (sps->malloced)
	free (sps->dirs);

      /* rtld_search_dirs is attribute_relro, therefore avoid writing
	 into it.  */
      if (sps != &rtld_search_dirs)
	sps->dirs = (void *) -1;
    }

  return -1;
}

/* Map in the shared object file NAME.  */

struct link_map *
internal_function
_dl_map_object (struct link_map *loader, const char *name, int preloaded,
		int type, int trace_mode, int mode, Lmid_t nsid)
{
  int fd;
  char *realname;
  char *name_copy;
  struct link_map *l;
  struct filebuf fb;

  assert (nsid >= 0);
  assert (nsid < GL(dl_nns));

  /* Look for this name among those already loaded.  */
  for (l = GL(dl_ns)[nsid]._ns_loaded; l; l = l->l_next)
    {
      /* If the requested name matches the soname of a loaded object,
	 use that object.  Elide this check for names that have not
	 yet been opened.  */
      if (__builtin_expect (l->l_faked, 0) != 0
	  || __builtin_expect (l->l_removed, 0) != 0)
	continue;
      if (!_dl_name_match_p (name, l))
	{
	  const char *soname;

	  if (__builtin_expect (l->l_soname_added, 1)
	      || l->l_info[DT_SONAME] == NULL)
	    continue;

	  soname = ((const char *) D_PTR (l, l_info[DT_STRTAB])
		    + l->l_info[DT_SONAME]->d_un.d_val);
	  if (strcmp (name, soname) != 0)
	    continue;

	  /* We have a match on a new name -- cache it.  */
	  add_name_to_object (l, soname);
	  l->l_soname_added = 1;
	}

      /* We have a match.  */
      return l;
    }

  /* Display information if we are debugging.  */
  if (__builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_FILES, 0)
      && loader != NULL)
    _dl_debug_printf ("\nfile=%s [%lu];  needed by %s [%lu]\n", name, nsid,
			      loader->l_name[0]
			      ? loader->l_name : rtld_progname, loader->l_ns);

#ifdef SHARED
  /* Give the auditing libraries a chance to change the name before we
     try anything.  */
  if (__builtin_expect (GLRO(dl_naudit) > 0, 0)
      && (loader == NULL || loader->l_auditing == 0))
    {
      struct audit_ifaces *afct = GLRO(dl_audit);
      for (unsigned int cnt = 0; cnt < GLRO(dl_naudit); ++cnt)
	{
	  if (afct->objsearch != NULL)
	    {
	      name = afct->objsearch (name, &loader->l_audit[cnt].cookie,
				      LA_SER_ORIG);
	      if (name == NULL)
		{
		  /* Do not try anything further.  */
		  fd = -1;
		  goto no_file;
		}
	    }

	  afct = afct->next;
	}
    }
#endif

  /* Will be true if we found a DSO which is of the other ELF class.  */
  bool found_other_class = false;

  if (strchr (name, '/') == NULL)
    {
      /* Search for NAME in several places.  */

      size_t namelen = strlen (name) + 1;

      if (__builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_LIBS, 0))
	_dl_debug_printf ("find library=%s [%lu]; searching\n", name, nsid);

      fd = -1;

      /* When the object has the RUNPATH information we don't use any
         RPATHs.  */
      if (loader == NULL || loader->l_info[DT_RUNPATH] == NULL)
	{
	  /* This is the executable's map (if there is one).  Make sure that
	     we do not look at it twice.  */
	  struct link_map *main_map = GL(dl_ns)[LM_ID_BASE]._ns_loaded;
	  bool did_main_map = false;

	  /* First try the DT_RPATH of the dependent object that caused NAME
	     to be loaded.  Then that object's dependent, and on up.  */
	  for (l = loader; l; l = l->l_loader)
	    if (cache_rpath (l, &l->l_rpath_dirs, DT_RPATH, "RPATH"))
	      {
		fd = open_path (name, namelen, preloaded, &l->l_rpath_dirs,
				&realname, &fb, loader, LA_SER_RUNPATH,
				&found_other_class);
		if (fd != -1)
		  break;

		did_main_map |= l == main_map;
	      }

	  /* If dynamically linked, try the DT_RPATH of the executable
             itself.  NB: we do this for lookups in any namespace.  */
	  if (fd == -1 && !did_main_map
	      && main_map != NULL && main_map->l_type != lt_loaded
	      && cache_rpath (main_map, &main_map->l_rpath_dirs, DT_RPATH,
			      "RPATH"))
	    fd = open_path (name, namelen, preloaded, &main_map->l_rpath_dirs,
			    &realname, &fb, loader ?: main_map, LA_SER_RUNPATH,
			    &found_other_class);
	}

      /* Try the LD_LIBRARY_PATH environment variable.  */
      if (fd == -1 && env_path_list.dirs != (void *) -1)
	fd = open_path (name, namelen, preloaded, &env_path_list,
			&realname, &fb,
			loader ?: GL(dl_ns)[LM_ID_BASE]._ns_loaded,
			LA_SER_LIBPATH, &found_other_class);

      /* Look at the RUNPATH information for this binary.  */
      if (fd == -1 && loader != NULL
	  && cache_rpath (loader, &loader->l_runpath_dirs,
			  DT_RUNPATH, "RUNPATH"))
	fd = open_path (name, namelen, preloaded,
			&loader->l_runpath_dirs, &realname, &fb, loader,
			LA_SER_RUNPATH, &found_other_class);

      if (fd == -1
	  && (__builtin_expect (! preloaded, 1)
	      || ! INTUSE(__libc_enable_secure)))
	{
	  /* Check the list of libraries in the file /etc/ld.so.cache,
	     for compatibility with Linux's ldconfig program.  */
	  const char *cached = _dl_load_cache_lookup (name);

	  if (cached != NULL)
	    {
#ifdef SHARED
	      // XXX Correct to unconditionally default to namespace 0?
	      l = loader ?: GL(dl_ns)[LM_ID_BASE]._ns_loaded;
#else
	      l = loader;
#endif

	      /* If the loader has the DF_1_NODEFLIB flag set we must not
		 use a cache entry from any of these directories.  */
	      if (
#ifndef SHARED
		  /* 'l' is always != NULL for dynamically linked objects.  */
		  l != NULL &&
#endif
		  __builtin_expect (l->l_flags_1 & DF_1_NODEFLIB, 0))
		{
		  const char *dirp = system_dirs;
		  unsigned int cnt = 0;

		  do
		    {
		      if (memcmp (cached, dirp, system_dirs_len[cnt]) == 0)
			{
			  /* The prefix matches.  Don't use the entry.  */
			  cached = NULL;
			  break;
			}

		      dirp += system_dirs_len[cnt] + 1;
		      ++cnt;
		    }
		  while (cnt < nsystem_dirs_len);
		}

	      if (cached != NULL)
		{
		  fd = open_verify (cached,
				    &fb, loader ?: GL(dl_ns)[nsid]._ns_loaded,
				    LA_SER_CONFIG, &found_other_class, false);
		  if (__builtin_expect (fd != -1, 1))
		    {
		      realname = local_strdup (cached);
		      if (realname == NULL)
			{
			  __close (fd);
			  fd = -1;
			}
		    }
		}
	    }
	}

      /* Finally, try the default path.  */
      if (fd == -1
	  && ((l = loader ?: GL(dl_ns)[nsid]._ns_loaded) == NULL
	      || __builtin_expect (!(l->l_flags_1 & DF_1_NODEFLIB), 1))
	  && rtld_search_dirs.dirs != (void *) -1)
	fd = open_path (name, namelen, preloaded, &rtld_search_dirs,
			&realname, &fb, l, LA_SER_DEFAULT, &found_other_class);

      /* Add another newline when we are tracing the library loading.  */
      if (__builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_LIBS, 0))
        _dl_debug_printf ("\n");
    }
  else
    {
      /* The path may contain dynamic string tokens.  */
      realname = (loader
		  ? expand_dynamic_string_token (loader, name)
		  : local_strdup (name));
      if (realname == NULL)
	fd = -1;
      else
	{
	  fd = open_verify (realname, &fb,
			    loader ?: GL(dl_ns)[nsid]._ns_loaded, 0,
			    &found_other_class, true);
	  if (__builtin_expect (fd, 0) == -1)
	    free (realname);
	}
    }

#ifdef SHARED
 no_file:
#endif
  /* In case the LOADER information has only been provided to get to
     the appropriate RUNPATH/RPATH information we do not need it
     anymore.  */
  if (mode & __RTLD_CALLMAP)
    loader = NULL;

  if (__builtin_expect (fd, 0) == -1)
    {
      if (trace_mode
	  && __builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_PRELINK, 0) == 0)
	{
	  /* We haven't found an appropriate library.  But since we
	     are only interested in the list of libraries this isn't
	     so severe.  Fake an entry with all the information we
	     have.  */
	  static const Elf_Symndx dummy_bucket = STN_UNDEF;

	  /* Enter the new object in the list of loaded objects.  */
	  if ((name_copy = local_strdup (name)) == NULL
	      || (l = _dl_new_object (name_copy, name, type, loader,
				      mode, nsid)) == NULL)
	    {
	      free (name_copy);
	      _dl_signal_error (ENOMEM, name, NULL,
				N_("cannot create shared object descriptor"));
	    }
	  /* Signal that this is a faked entry.  */
	  l->l_faked = 1;
	  /* Since the descriptor is initialized with zero we do not
	     have do this here.
	  l->l_reserved = 0; */
	  l->l_buckets = &dummy_bucket;
	  l->l_nbuckets = 1;
	  l->l_relocated = 1;

	  return l;
	}
      else if (found_other_class)
	_dl_signal_error (0, name, NULL,
			  ELFW(CLASS) == ELFCLASS32
			  ? N_("wrong ELF class: ELFCLASS64")
			  : N_("wrong ELF class: ELFCLASS32"));
      else
	_dl_signal_error (errno, name, NULL,
			  N_("cannot open shared object file"));
    }

  void *stack_end = __libc_stack_end;
  return _dl_map_object_from_fd (name, fd, &fb, realname, loader, type, mode,
				 &stack_end, nsid);
}


void
internal_function
_dl_rtld_di_serinfo (struct link_map *loader, Dl_serinfo *si, bool counting)
{
  if (counting)
    {
      si->dls_cnt = 0;
      si->dls_size = 0;
    }

  unsigned int idx = 0;
  char *allocptr = (char *) &si->dls_serpath[si->dls_cnt];
  void add_path (const struct r_search_path_struct *sps, unsigned int flags)
# define add_path(sps, flags) add_path(sps, 0) /* XXX */
    {
      if (sps->dirs != (void *) -1)
	{
	  struct r_search_path_elem **dirs = sps->dirs;
	  do
	    {
	      const struct r_search_path_elem *const r = *dirs++;
	      if (counting)
		{
		  si->dls_cnt++;
		  si->dls_size += MAX (2, r->dirnamelen);
		}
	      else
		{
		  Dl_serpath *const sp = &si->dls_serpath[idx++];
		  sp->dls_name = allocptr;
		  if (r->dirnamelen < 2)
		    *allocptr++ = r->dirnamelen ? '/' : '.';
		  else
		    allocptr = __mempcpy (allocptr,
					  r->dirname, r->dirnamelen - 1);
		  *allocptr++ = '\0';
		  sp->dls_flags = flags;
		}
	    }
	  while (*dirs != NULL);
	}
    }

  /* When the object has the RUNPATH information we don't use any RPATHs.  */
  if (loader->l_info[DT_RUNPATH] == NULL)
    {
      /* First try the DT_RPATH of the dependent object that caused NAME
	 to be loaded.  Then that object's dependent, and on up.  */

      struct link_map *l = loader;
      do
	{
	  if (cache_rpath (l, &l->l_rpath_dirs, DT_RPATH, "RPATH"))
	    add_path (&l->l_rpath_dirs, XXX_RPATH);
	  l = l->l_loader;
	}
      while (l != NULL);

      /* If dynamically linked, try the DT_RPATH of the executable itself.  */
      if (loader->l_ns == LM_ID_BASE)
	{
	  l = GL(dl_ns)[LM_ID_BASE]._ns_loaded;
	  if (l != NULL && l->l_type != lt_loaded && l != loader)
	    if (cache_rpath (l, &l->l_rpath_dirs, DT_RPATH, "RPATH"))
	      add_path (&l->l_rpath_dirs, XXX_RPATH);
	}
    }

  /* Try the LD_LIBRARY_PATH environment variable.  */
  add_path (&env_path_list, XXX_ENV);

  /* Look at the RUNPATH information for this binary.  */
  if (cache_rpath (loader, &loader->l_runpath_dirs, DT_RUNPATH, "RUNPATH"))
    add_path (&loader->l_runpath_dirs, XXX_RUNPATH);

  /* XXX
     Here is where ld.so.cache gets checked, but we don't have
     a way to indicate that in the results for Dl_serinfo.  */

  /* Finally, try the default path.  */
  if (!(loader->l_flags_1 & DF_1_NODEFLIB))
    add_path (&rtld_search_dirs, XXX_default);

  if (counting)
    /* Count the struct size before the string area, which we didn't
       know before we completed dls_cnt.  */
    si->dls_size += (char *) &si->dls_serpath[si->dls_cnt] - (char *) si;
}
