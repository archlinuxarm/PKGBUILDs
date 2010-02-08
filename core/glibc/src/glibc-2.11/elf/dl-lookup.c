/* Look up a symbol in the loaded objects.
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

#include <alloca.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ldsodefs.h>
#include <dl-hash.h>
#include <dl-machine.h>
#include <sysdep-cancel.h>
#include <bits/libc-lock.h>
#include <tls.h>

#include <assert.h>

#define VERSTAG(tag)	(DT_NUM + DT_THISPROCNUM + DT_VERSIONTAGIDX (tag))

/* We need this string more than once.  */
static const char undefined_msg[] = "undefined symbol: ";


struct sym_val
  {
    const ElfW(Sym) *s;
    struct link_map *m;
  };


#define make_string(string, rest...) \
  ({									      \
    const char *all[] = { string, ## rest };				      \
    size_t len, cnt;							      \
    char *result, *cp;							      \
									      \
    len = 1;								      \
    for (cnt = 0; cnt < sizeof (all) / sizeof (all[0]); ++cnt)		      \
      len += strlen (all[cnt]);						      \
									      \
    cp = result = alloca (len);						      \
    for (cnt = 0; cnt < sizeof (all) / sizeof (all[0]); ++cnt)		      \
      cp = __stpcpy (cp, all[cnt]);					      \
									      \
    result;								      \
  })

/* Statistics function.  */
#ifdef SHARED
# define bump_num_relocations() ++GL(dl_num_relocations)
#else
# define bump_num_relocations() ((void) 0)
#endif


/* Inner part of the lookup functions.  We return a value > 0 if we
   found the symbol, the value 0 if nothing is found and < 0 if
   something bad happened.  */
static int
__attribute_noinline__
do_lookup_x (const char *undef_name, uint_fast32_t new_hash,
	     unsigned long int *old_hash, const ElfW(Sym) *ref,
	     struct sym_val *result, struct r_scope_elem *scope, size_t i,
	     const struct r_found_version *const version, int flags,
	     struct link_map *skip, int type_class, struct link_map *undef_map)
{
  size_t n = scope->r_nlist;
  /* Make sure we read the value before proceeding.  Otherwise we
     might use r_list pointing to the initial scope and r_nlist being
     the value after a resize.  That is the only path in dl-open.c not
     protected by GSCOPE.  A read barrier here might be to expensive.  */
  __asm volatile ("" : "+r" (n), "+m" (scope->r_list));
  struct link_map **list = scope->r_list;

  do
    {
      /* These variables are used in the nested function.  */
      Elf_Symndx symidx;
      int num_versions = 0;
      const ElfW(Sym) *versioned_sym = NULL;

      const struct link_map *map = list[i]->l_real;

      /* Here come the extra test needed for `_dl_lookup_symbol_skip'.  */
      if (map == skip)
	continue;

      /* Don't search the executable when resolving a copy reloc.  */
      if ((type_class & ELF_RTYPE_CLASS_COPY) && map->l_type == lt_executable)
	continue;

      /* Do not look into objects which are going to be removed.  */
      if (map->l_removed)
	continue;

      /* Print some debugging info if wanted.  */
      if (__builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_SYMBOLS, 0))
	_dl_debug_printf ("symbol=%s;  lookup in file=%s [%lu]\n",
			  undef_name,
			  map->l_name[0] ? map->l_name : rtld_progname,
			  map->l_ns);

      /* If the hash table is empty there is nothing to do here.  */
      if (map->l_nbuckets == 0)
	continue;

      /* The tables for this map.  */
      const ElfW(Sym) *symtab = (const void *) D_PTR (map, l_info[DT_SYMTAB]);
      const char *strtab = (const void *) D_PTR (map, l_info[DT_STRTAB]);


      /* Nested routine to check whether the symbol matches.  */
      const ElfW(Sym) *
      __attribute_noinline__
      check_match (const ElfW(Sym) *sym)
      {
	unsigned int stt = ELFW(ST_TYPE) (sym->st_info);
	assert (ELF_RTYPE_CLASS_PLT == 1);
	if (__builtin_expect ((sym->st_value == 0 /* No value.  */
			       && stt != STT_TLS)
			      || (type_class & (sym->st_shndx == SHN_UNDEF)),
			      0))
	  return NULL;

	/* Ignore all but STT_NOTYPE, STT_OBJECT, STT_FUNC,
	   STT_COMMON, STT_TLS, and STT_GNU_IFUNC since these are no
	   code/data definitions.  */
#define ALLOWED_STT \
	((1 << STT_NOTYPE) | (1 << STT_OBJECT) | (1 << STT_FUNC) \
	 | (1 << STT_COMMON) | (1 << STT_TLS) | (1 << STT_GNU_IFUNC))
	if (__builtin_expect (((1 << stt) & ALLOWED_STT) == 0, 0))
	  return NULL;

	if (sym != ref && strcmp (strtab + sym->st_name, undef_name))
	  /* Not the symbol we are looking for.  */
	  return NULL;

	const ElfW(Half) *verstab = map->l_versyms;
	if (version != NULL)
	  {
	    if (__builtin_expect (verstab == NULL, 0))
	      {
		/* We need a versioned symbol but haven't found any.  If
		   this is the object which is referenced in the verneed
		   entry it is a bug in the library since a symbol must
		   not simply disappear.

		   It would also be a bug in the object since it means that
		   the list of required versions is incomplete and so the
		   tests in dl-version.c haven't found a problem.*/
		assert (version->filename == NULL
			|| ! _dl_name_match_p (version->filename, map));

		/* Otherwise we accept the symbol.  */
	      }
	    else
	      {
		/* We can match the version information or use the
		   default one if it is not hidden.  */
		ElfW(Half) ndx = verstab[symidx] & 0x7fff;
		if ((map->l_versions[ndx].hash != version->hash
		     || strcmp (map->l_versions[ndx].name, version->name))
		    && (version->hidden || map->l_versions[ndx].hash
			|| (verstab[symidx] & 0x8000)))
		  /* It's not the version we want.  */
		  return NULL;
	      }
	  }
	else
	  {
	    /* No specific version is selected.  There are two ways we
	       can got here:

	       - a binary which does not include versioning information
	       is loaded

	       - dlsym() instead of dlvsym() is used to get a symbol which
	       might exist in more than one form

	       If the library does not provide symbol version information
	       there is no problem at at: we simply use the symbol if it
	       is defined.

	       These two lookups need to be handled differently if the
	       library defines versions.  In the case of the old
	       unversioned application the oldest (default) version
	       should be used.  In case of a dlsym() call the latest and
	       public interface should be returned.  */
	    if (verstab != NULL)
	      {
		if ((verstab[symidx] & 0x7fff)
		    >= ((flags & DL_LOOKUP_RETURN_NEWEST) ? 2 : 3))
		  {
		    /* Don't accept hidden symbols.  */
		    if ((verstab[symidx] & 0x8000) == 0
			&& num_versions++ == 0)
		      /* No version so far.  */
		      versioned_sym = sym;

		    return NULL;
		  }
	      }
	  }

	/* There cannot be another entry for this symbol so stop here.  */
	return sym;
      }

      const ElfW(Sym) *sym;
      const ElfW(Addr) *bitmask = map->l_gnu_bitmask;
      if (__builtin_expect (bitmask != NULL, 1))
	{
	  ElfW(Addr) bitmask_word
	    = bitmask[(new_hash / __ELF_NATIVE_CLASS)
		      & map->l_gnu_bitmask_idxbits];

	  unsigned int hashbit1 = new_hash & (__ELF_NATIVE_CLASS - 1);
	  unsigned int hashbit2 = ((new_hash >> map->l_gnu_shift)
				   & (__ELF_NATIVE_CLASS - 1));

	  if (__builtin_expect ((bitmask_word >> hashbit1)
				& (bitmask_word >> hashbit2) & 1, 0))
	    {
	      Elf32_Word bucket = map->l_gnu_buckets[new_hash
						     % map->l_nbuckets];
	      if (bucket != 0)
		{
		  const Elf32_Word *hasharr = &map->l_gnu_chain_zero[bucket];

		  do
		    if (((*hasharr ^ new_hash) >> 1) == 0)
		      {
			symidx = hasharr - map->l_gnu_chain_zero;
			sym = check_match (&symtab[symidx]);
			if (sym != NULL)
			  goto found_it;
		      }
		  while ((*hasharr++ & 1u) == 0);
		}
	    }
	  /* No symbol found.  */
	  symidx = SHN_UNDEF;
	}
      else
	{
	  if (*old_hash == 0xffffffff)
	    *old_hash = _dl_elf_hash (undef_name);

	  /* Use the old SysV-style hash table.  Search the appropriate
	     hash bucket in this object's symbol table for a definition
	     for the same symbol name.  */
	  for (symidx = map->l_buckets[*old_hash % map->l_nbuckets];
	       symidx != STN_UNDEF;
	       symidx = map->l_chain[symidx])
	    {
	      sym = check_match (&symtab[symidx]);
	      if (sym != NULL)
		goto found_it;
	    }
	}

      /* If we have seen exactly one versioned symbol while we are
	 looking for an unversioned symbol and the version is not the
	 default version we still accept this symbol since there are
	 no possible ambiguities.  */
      sym = num_versions == 1 ? versioned_sym : NULL;

      if (sym != NULL)
	{
	found_it:
	  switch (__builtin_expect (ELFW(ST_BIND) (sym->st_info), STB_GLOBAL))
	    {
	    case STB_WEAK:
	      /* Weak definition.  Use this value if we don't find another.  */
	      if (__builtin_expect (GLRO(dl_dynamic_weak), 0))
		{
		  if (! result->s)
		    {
		      result->s = sym;
		      result->m = (struct link_map *) map;
		    }
		  break;
		}
	      /* FALLTHROUGH */
	    case STB_GLOBAL:
	    success:
	      /* Global definition.  Just what we need.  */
	      result->s = sym;
	      result->m = (struct link_map *) map;
	      return 1;

	    case STB_GNU_UNIQUE:;
	      /* We have to determine whether we already found a
		 symbol with this name before.  If not then we have to
		 add it to the search table.  If we already found a
		 definition we have to use it.  */
	      void enter (struct unique_sym *table, size_t size,
			  unsigned int hash, const char *name,
			  const ElfW(Sym) *sym, struct link_map *map)
	      {
		size_t idx = hash % size;
		size_t hash2 = 1 + hash % (size - 2);
		while (1)
		  {
		    if (table[idx].name == NULL)
		      {
			table[idx].hashval = hash;
			table[idx].name = name;
			if ((type_class & ELF_RTYPE_CLASS_COPY) != 0)
			  {
			    table[idx].sym = ref;
			    table[idx].map = undef_map;
			  }
			else
			  {
			    table[idx].sym = sym;
			    table[idx].map = map;

			    if (map->l_type == lt_loaded)
			      /* Make sure we don't unload this object by
				 setting the appropriate flag.  */
			      map->l_flags_1 |= DF_1_NODELETE;
			  }

			return;
		      }

		    idx += hash2;
		    if (idx >= size)
		      idx -= size;
		  }
	      }

	      struct unique_sym_table *tab
		= &GL(dl_ns)[map->l_ns]._ns_unique_sym_table;

	      __rtld_lock_lock_recursive (tab->lock);

	      struct unique_sym *entries = tab->entries;
	      size_t size = tab->size;
	      if (entries != NULL)
		{
		  size_t idx = new_hash % size;
		  size_t hash2 = 1 + new_hash % (size - 2);
		  while (1)
		    {
		      if (entries[idx].hashval == new_hash
			  && strcmp (entries[idx].name, undef_name) == 0)
			{
			  result->s = entries[idx].sym;
			  result->m = (struct link_map *) entries[idx].map;
			  __rtld_lock_unlock_recursive (tab->lock);
			  return 1;
			}

		      if (entries[idx].name == NULL)
			break;

		      idx += hash2;
		      if (idx >= size)
			idx -= size;
		    }

		  if (size * 3 <= tab->n_elements * 4)
		    {
		      /* Expand the table.  */
#ifdef RTLD_CHECK_FOREIGN_CALL
		      /* This must not happen during runtime relocations.  */
		      assert (!RTLD_CHECK_FOREIGN_CALL);
#endif
		      size_t newsize = _dl_higher_prime_number (size + 1);
		      struct unique_sym *newentries
			= calloc (sizeof (struct unique_sym), newsize);
		      if (newentries == NULL)
			{
			nomem:
			  __rtld_lock_unlock_recursive (tab->lock);
			  _dl_fatal_printf ("out of memory\n");
			}

		      for (idx = 0; idx < size; ++idx)
			if (entries[idx].name != NULL)
			  enter (newentries, newsize, entries[idx].hashval,
				 entries[idx].name, entries[idx].sym,
				 entries[idx].map);

		      tab->free (entries);
		      tab->size = newsize;
		      size = newsize;
		      entries = tab->entries = newentries;
		      tab->free = free;
		    }
		}
	      else
		{
#ifdef RTLD_CHECK_FOREIGN_CALL
		  /* This must not happen during runtime relocations.  */
		  assert (!RTLD_CHECK_FOREIGN_CALL);
#endif

#define INITIAL_NUNIQUE_SYM_TABLE 31
		  size = INITIAL_NUNIQUE_SYM_TABLE;
		  entries = calloc (sizeof (struct unique_sym), size);
		  if (entries == NULL)
		    goto nomem;

		  tab->entries = entries;
		  tab->size = size;
		  tab->free = free;
		}

	      enter (entries, size, new_hash, strtab + sym->st_name, sym,
		     (struct link_map *) map);
	      ++tab->n_elements;

	      __rtld_lock_unlock_recursive (tab->lock);

	      goto success;

	    default:
	      /* Local symbols are ignored.  */
	      break;
	    }
	}

      /* If this current map is the one mentioned in the verneed entry
	 and we have not found a weak entry, it is a bug.  */
      if (symidx == STN_UNDEF && version != NULL && version->filename != NULL
	  && __builtin_expect (_dl_name_match_p (version->filename, map), 0))
	return -1;
    }
  while (++i < n);

  /* We have not found anything until now.  */
  return 0;
}


static uint_fast32_t
dl_new_hash (const char *s)
{
  uint_fast32_t h = 5381;
  for (unsigned char c = *s; c != '\0'; c = *++s)
    h = h * 33 + c;
  return h & 0xffffffff;
}


/* Add extra dependency on MAP to UNDEF_MAP.  */
static int
internal_function
add_dependency (struct link_map *undef_map, struct link_map *map, int flags)
{
  struct link_map *runp;
  unsigned int i;
  int result = 0;

  /* Avoid self-references and references to objects which cannot be
     unloaded anyway.  */
  if (undef_map == map)
    return 0;

  /* Avoid references to objects which cannot be unloaded anyway.  */
  assert (map->l_type == lt_loaded);
  if ((map->l_flags_1 & DF_1_NODELETE) != 0)
    return 0;

  struct link_map_reldeps *l_reldeps
    = atomic_forced_read (undef_map->l_reldeps);

  /* Make sure l_reldeps is read before l_initfini.  */
  atomic_read_barrier ();

  /* Determine whether UNDEF_MAP already has a reference to MAP.  First
     look in the normal dependencies.  */
  struct link_map **l_initfini = atomic_forced_read (undef_map->l_initfini);
  if (l_initfini != NULL)
    {
      for (i = 0; l_initfini[i] != NULL; ++i)
	if (l_initfini[i] == map)
	  return 0;
    }

  /* No normal dependency.  See whether we already had to add it
     to the special list of dynamic dependencies.  */
  unsigned int l_reldepsact = 0;
  if (l_reldeps != NULL)
    {
      struct link_map **list = &l_reldeps->list[0];
      l_reldepsact = l_reldeps->act;
      for (i = 0; i < l_reldepsact; ++i)
	if (list[i] == map)
	  return 0;
    }

  /* Save serial number of the target MAP.  */
  unsigned long long serial = map->l_serial;

  /* Make sure nobody can unload the object while we are at it.  */
  if (__builtin_expect (flags & DL_LOOKUP_GSCOPE_LOCK, 0))
    {
      /* We can't just call __rtld_lock_lock_recursive (GL(dl_load_lock))
	 here, that can result in ABBA deadlock.  */
      THREAD_GSCOPE_RESET_FLAG ();
      __rtld_lock_lock_recursive (GL(dl_load_lock));
      /* While MAP value won't change, after THREAD_GSCOPE_RESET_FLAG ()
	 it can e.g. point to unallocated memory.  So avoid the optimizer
	 treating the above read from MAP->l_serial as ensurance it
	 can safely dereference it.  */
      map = atomic_forced_read (map);

      /* From this point on it is unsafe to dereference MAP, until it
	 has been found in one of the lists.  */

      /* Redo the l_initfini check in case undef_map's l_initfini
	 changed in the mean time.  */
      if (undef_map->l_initfini != l_initfini
	  && undef_map->l_initfini != NULL)
	{
	  l_initfini = undef_map->l_initfini;
	  for (i = 0; l_initfini[i] != NULL; ++i)
	    if (l_initfini[i] == map)
	      goto out_check;
	}

      /* Redo the l_reldeps check if undef_map's l_reldeps changed in
	 the mean time.  */
      if (undef_map->l_reldeps != NULL)
	{
	  if (undef_map->l_reldeps != l_reldeps)
	    {
	      struct link_map **list = &undef_map->l_reldeps->list[0];
	      l_reldepsact = undef_map->l_reldeps->act;
	      for (i = 0; i < l_reldepsact; ++i)
		if (list[i] == map)
		  goto out_check;
	    }
	  else if (undef_map->l_reldeps->act > l_reldepsact)
	    {
	      struct link_map **list
		= &undef_map->l_reldeps->list[0];
	      i = l_reldepsact;
	      l_reldepsact = undef_map->l_reldeps->act;
	      for (; i < l_reldepsact; ++i)
		if (list[i] == map)
		  goto out_check;
	    }
	}
    }
  else
    __rtld_lock_lock_recursive (GL(dl_load_lock));

  /* The object is not yet in the dependency list.  Before we add
     it make sure just one more time the object we are about to
     reference is still available.  There is a brief period in
     which the object could have been removed since we found the
     definition.  */
  runp = GL(dl_ns)[undef_map->l_ns]._ns_loaded;
  while (runp != NULL && runp != map)
    runp = runp->l_next;

  if (runp != NULL)
    {
      /* The object is still available.  */

      /* MAP could have been dlclosed, freed and then some other dlopened
	 library could have the same link_map pointer.  */
      if (map->l_serial != serial)
	goto out_check;

      /* Redo the NODELETE check, as when dl_load_lock wasn't held
	 yet this could have changed.  */
      if ((map->l_flags_1 & DF_1_NODELETE) != 0)
	goto out;

      /* If the object with the undefined reference cannot be removed ever
	 just make sure the same is true for the object which contains the
	 definition.  */
      if (undef_map->l_type != lt_loaded
	  || (undef_map->l_flags_1 & DF_1_NODELETE) != 0)
	{
	  map->l_flags_1 |= DF_1_NODELETE;
	  goto out;
	}

      /* Add the reference now.  */
      if (__builtin_expect (l_reldepsact >= undef_map->l_reldepsmax, 0))
	{
	  /* Allocate more memory for the dependency list.  Since this
	     can never happen during the startup phase we can use
	     `realloc'.  */
	  struct link_map_reldeps *newp;
	  unsigned int max
	    = undef_map->l_reldepsmax ? undef_map->l_reldepsmax * 2 : 10;

#ifdef RTLD_PREPARE_FOREIGN_CALL
	  RTLD_PREPARE_FOREIGN_CALL;
#endif

	  newp = malloc (sizeof (*newp) + max * sizeof (struct link_map *));
	  if (newp == NULL)
	    {
	      /* If we didn't manage to allocate memory for the list this is
		 no fatal problem.  We simply make sure the referenced object
		 cannot be unloaded.  This is semantically the correct
		 behavior.  */
	      map->l_flags_1 |= DF_1_NODELETE;
	      goto out;
	    }
	  else
	    {
	      if (l_reldepsact)
		memcpy (&newp->list[0], &undef_map->l_reldeps->list[0],
			l_reldepsact * sizeof (struct link_map *));
	      newp->list[l_reldepsact] = map;
	      newp->act = l_reldepsact + 1;
	      atomic_write_barrier ();
	      void *old = undef_map->l_reldeps;
	      undef_map->l_reldeps = newp;
	      undef_map->l_reldepsmax = max;
	      if (old)
		_dl_scope_free (old);
	    }
	}
      else
	{
	  undef_map->l_reldeps->list[l_reldepsact] = map;
	  atomic_write_barrier ();
	  undef_map->l_reldeps->act = l_reldepsact + 1;
	}

      /* Display information if we are debugging.  */
      if (__builtin_expect (GLRO(dl_debug_mask) & DL_DEBUG_FILES, 0))
	_dl_debug_printf ("\
\nfile=%s [%lu];  needed by %s [%lu] (relocation dependency)\n\n",
			  map->l_name[0] ? map->l_name : rtld_progname,
			  map->l_ns,
			  undef_map->l_name[0]
			  ? undef_map->l_name : rtld_progname,
			  undef_map->l_ns);
    }
  else
    /* Whoa, that was bad luck.  We have to search again.  */
    result = -1;

 out:
  /* Release the lock.  */
  __rtld_lock_unlock_recursive (GL(dl_load_lock));

  if (__builtin_expect (flags & DL_LOOKUP_GSCOPE_LOCK, 0))
    THREAD_GSCOPE_SET_FLAG ();

  return result;

 out_check:
  if (map->l_serial != serial)
    result = -1;
  goto out;
}

static void
internal_function
_dl_debug_bindings (const char *undef_name, struct link_map *undef_map,
		    const ElfW(Sym) **ref, struct sym_val *value,
		    const struct r_found_version *version, int type_class,
		    int protected);


/* Search loaded objects' symbol tables for a definition of the symbol
   UNDEF_NAME, perhaps with a requested version for the symbol.

   We must never have calls to the audit functions inside this function
   or in any function which gets called.  If this would happen the audit
   code might create a thread which can throw off all the scope locking.  */
lookup_t
internal_function
_dl_lookup_symbol_x (const char *undef_name, struct link_map *undef_map,
		     const ElfW(Sym) **ref,
		     struct r_scope_elem *symbol_scope[],
		     const struct r_found_version *version,
		     int type_class, int flags, struct link_map *skip_map)
{
  const uint_fast32_t new_hash = dl_new_hash (undef_name);
  unsigned long int old_hash = 0xffffffff;
  struct sym_val current_value = { NULL, NULL };
  struct r_scope_elem **scope = symbol_scope;

  bump_num_relocations ();

  /* No other flag than DL_LOOKUP_ADD_DEPENDENCY or DL_LOOKUP_GSCOPE_LOCK
     is allowed if we look up a versioned symbol.  */
  assert (version == NULL
	  || (flags & ~(DL_LOOKUP_ADD_DEPENDENCY | DL_LOOKUP_GSCOPE_LOCK))
	     == 0);

  size_t i = 0;
  if (__builtin_expect (skip_map != NULL, 0))
    /* Search the relevant loaded objects for a definition.  */
    while ((*scope)->r_list[i] != skip_map)
      ++i;

  /* Search the relevant loaded objects for a definition.  */
  for (size_t start = i; *scope != NULL; start = 0, ++scope)
    {
      int res = do_lookup_x (undef_name, new_hash, &old_hash, *ref,
			     &current_value, *scope, start, version, flags,
			     skip_map, type_class, undef_map);
      if (res > 0)
	break;

      if (__builtin_expect (res, 0) < 0 && skip_map == NULL)
	{
	  /* Oh, oh.  The file named in the relocation entry does not
	     contain the needed symbol.  This code is never reached
	     for unversioned lookups.  */
	  assert (version != NULL);
	  const char *reference_name = undef_map ? undef_map->l_name : NULL;

	  /* XXX We cannot translate the message.  */
	  _dl_signal_cerror (0, (reference_name[0]
				 ? reference_name
				 : (rtld_progname ?: "<main program>")),
			     N_("relocation error"),
			     make_string ("symbol ", undef_name, ", version ",
					  version->name,
					  " not defined in file ",
					  version->filename,
					  " with link time reference",
					  res == -2
					  ? " (no version symbols)" : ""));
	  *ref = NULL;
	  return 0;
	}
    }

  if (__builtin_expect (current_value.s == NULL, 0))
    {
      if ((*ref == NULL || ELFW(ST_BIND) ((*ref)->st_info) != STB_WEAK)
	  && skip_map == NULL)
	{
	  /* We could find no value for a strong reference.  */
	  const char *reference_name = undef_map ? undef_map->l_name : "";
	  const char *versionstr = version ? ", version " : "";
	  const char *versionname = (version && version->name
				     ? version->name : "");

	  /* XXX We cannot translate the message.  */
	  _dl_signal_cerror (0, (reference_name[0]
				 ? reference_name
				 : (rtld_progname ?: "<main program>")),
			     N_("symbol lookup error"),
			     make_string (undefined_msg, undef_name,
					  versionstr, versionname));
	}
      *ref = NULL;
      return 0;
    }

  int protected = (*ref
		   && ELFW(ST_VISIBILITY) ((*ref)->st_other) == STV_PROTECTED);
  if (__builtin_expect (protected != 0, 0))
    {
      /* It is very tricky.  We need to figure out what value to
         return for the protected symbol.  */
      if (type_class == ELF_RTYPE_CLASS_PLT)
	{
	  if (current_value.s != NULL && current_value.m != undef_map)
	    {
	      current_value.s = *ref;
	      current_value.m = undef_map;
	    }
	}
      else
	{
	  struct sym_val protected_value = { NULL, NULL };

	  for (scope = symbol_scope; *scope != NULL; i = 0, ++scope)
	    if (do_lookup_x (undef_name, new_hash, &old_hash, *ref,
			     &protected_value, *scope, i, version, flags,
			     skip_map, ELF_RTYPE_CLASS_PLT, NULL) != 0)
	      break;

	  if (protected_value.s != NULL && protected_value.m != undef_map)
	    {
	      current_value.s = *ref;
	      current_value.m = undef_map;
	    }
	}
    }

  /* We have to check whether this would bind UNDEF_MAP to an object
     in the global scope which was dynamically loaded.  In this case
     we have to prevent the latter from being unloaded unless the
     UNDEF_MAP object is also unloaded.  */
  if (__builtin_expect (current_value.m->l_type == lt_loaded, 0)
      /* Don't do this for explicit lookups as opposed to implicit
	 runtime lookups.  */
      && (flags & DL_LOOKUP_ADD_DEPENDENCY) != 0
      /* Add UNDEF_MAP to the dependencies.  */
      && add_dependency (undef_map, current_value.m, flags) < 0)
      /* Something went wrong.  Perhaps the object we tried to reference
	 was just removed.  Try finding another definition.  */
      return _dl_lookup_symbol_x (undef_name, undef_map, ref,
				  (flags & DL_LOOKUP_GSCOPE_LOCK)
				  ? undef_map->l_scope : symbol_scope,
				  version, type_class, flags, skip_map);

  /* The object is used.  */
  current_value.m->l_used = 1;

  if (__builtin_expect (GLRO(dl_debug_mask)
			& (DL_DEBUG_BINDINGS|DL_DEBUG_PRELINK), 0))
    _dl_debug_bindings (undef_name, undef_map, ref,
			&current_value, version, type_class, protected);

  *ref = current_value.s;
  return LOOKUP_VALUE (current_value.m);
}


/* Cache the location of MAP's hash table.  */

void
internal_function
_dl_setup_hash (struct link_map *map)
{
  Elf_Symndx *hash;
  Elf_Symndx nchain;

  if (__builtin_expect (map->l_info[DT_ADDRTAGIDX (DT_GNU_HASH) + DT_NUM
  				    + DT_THISPROCNUM + DT_VERSIONTAGNUM
				    + DT_EXTRANUM + DT_VALNUM] != NULL, 1))
    {
      Elf32_Word *hash32
	= (void *) D_PTR (map, l_info[DT_ADDRTAGIDX (DT_GNU_HASH) + DT_NUM
				      + DT_THISPROCNUM + DT_VERSIONTAGNUM
				      + DT_EXTRANUM + DT_VALNUM]);
      map->l_nbuckets = *hash32++;
      Elf32_Word symbias = *hash32++;
      Elf32_Word bitmask_nwords = *hash32++;
      /* Must be a power of two.  */
      assert ((bitmask_nwords & (bitmask_nwords - 1)) == 0);
      map->l_gnu_bitmask_idxbits = bitmask_nwords - 1;
      map->l_gnu_shift = *hash32++;

      map->l_gnu_bitmask = (ElfW(Addr) *) hash32;
      hash32 += __ELF_NATIVE_CLASS / 32 * bitmask_nwords;

      map->l_gnu_buckets = hash32;
      hash32 += map->l_nbuckets;
      map->l_gnu_chain_zero = hash32 - symbias;
      return;
    }

  if (!map->l_info[DT_HASH])
    return;
  hash = (void *) D_PTR (map, l_info[DT_HASH]);

  map->l_nbuckets = *hash++;
  nchain = *hash++;
  map->l_buckets = hash;
  hash += map->l_nbuckets;
  map->l_chain = hash;
}


static void
internal_function
_dl_debug_bindings (const char *undef_name, struct link_map *undef_map,
		    const ElfW(Sym) **ref, struct sym_val *value,
		    const struct r_found_version *version, int type_class,
		    int protected)
{
  const char *reference_name = undef_map->l_name;

  if (GLRO(dl_debug_mask) & DL_DEBUG_BINDINGS)
    {
      _dl_debug_printf ("binding file %s [%lu] to %s [%lu]: %s symbol `%s'",
			(reference_name[0]
			 ? reference_name
			 : (rtld_progname ?: "<main program>")),
			undef_map->l_ns,
			value->m->l_name[0] ? value->m->l_name : rtld_progname,
			value->m->l_ns,
			protected ? "protected" : "normal", undef_name);
      if (version)
	_dl_debug_printf_c (" [%s]\n", version->name);
      else
	_dl_debug_printf_c ("\n");
    }
#ifdef SHARED
  if (GLRO(dl_debug_mask) & DL_DEBUG_PRELINK)
    {
      int conflict = 0;
      struct sym_val val = { NULL, NULL };

      if ((GLRO(dl_trace_prelink_map) == NULL
	   || GLRO(dl_trace_prelink_map) == GL(dl_ns)[LM_ID_BASE]._ns_loaded)
	  && undef_map != GL(dl_ns)[LM_ID_BASE]._ns_loaded)
	{
	  const uint_fast32_t new_hash = dl_new_hash (undef_name);
	  unsigned long int old_hash = 0xffffffff;

	  do_lookup_x (undef_name, new_hash, &old_hash, *ref, &val,
		       undef_map->l_local_scope[0], 0, version, 0, NULL,
		       type_class, undef_map);

	  if (val.s != value->s || val.m != value->m)
	    conflict = 1;
	}

      if (value->s)
	{
	  if (__builtin_expect (ELFW(ST_TYPE) (value->s->st_info)
				== STT_TLS, 0))
	    type_class = 4;
	  else if (__builtin_expect (ELFW(ST_TYPE) (value->s->st_info)
				     == STT_GNU_IFUNC, 0))
	    type_class |= 8;
	}

      if (conflict
	  || GLRO(dl_trace_prelink_map) == undef_map
	  || GLRO(dl_trace_prelink_map) == NULL
	  || type_class >= 4)
	{
	  _dl_printf ("%s 0x%0*Zx 0x%0*Zx -> 0x%0*Zx 0x%0*Zx ",
		      conflict ? "conflict" : "lookup",
		      (int) sizeof (ElfW(Addr)) * 2,
		      (size_t) undef_map->l_map_start,
		      (int) sizeof (ElfW(Addr)) * 2,
		      (size_t) (((ElfW(Addr)) *ref) - undef_map->l_map_start),
		      (int) sizeof (ElfW(Addr)) * 2,
		      (size_t) (value->s ? value->m->l_map_start : 0),
		      (int) sizeof (ElfW(Addr)) * 2,
		      (size_t) (value->s ? value->s->st_value : 0));

	  if (conflict)
	    _dl_printf ("x 0x%0*Zx 0x%0*Zx ",
			(int) sizeof (ElfW(Addr)) * 2,
			(size_t) (val.s ? val.m->l_map_start : 0),
			(int) sizeof (ElfW(Addr)) * 2,
			(size_t) (val.s ? val.s->st_value : 0));

	  _dl_printf ("/%x %s\n", type_class, undef_name);
	}
    }
#endif
}
