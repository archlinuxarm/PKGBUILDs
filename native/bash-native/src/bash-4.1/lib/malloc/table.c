/* table.c - bookkeeping functions for allocated memory */

/* Copyright (C) 2001-2003 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "imalloc.h"
#include "table.h"

extern int malloc_register;

#ifdef MALLOC_REGISTER

#define FIND_ALLOC	0x01	/* allocate new entry or find existing */
#define FIND_EXIST	0x02	/* find existing entry */

static int table_count = 0;
static int table_allocated = 0;
static mr_table_t mem_table[REG_TABLE_SIZE];
static mr_table_t mem_overflow;

/*
 * NOTE: taken from dmalloc (http://dmalloc.com) and modified.
 */
static unsigned int
mt_hash (key)
     const PTR_T key;
{
  unsigned int a, b, c;
  unsigned long x;

  /* set up the internal state */
  a = 0x9e3779b9;	/* the golden ratio; an arbitrary value */
  x = (unsigned long)key;		/* truncation is OK */
  b = x >> 8;
  c = x >> 3;				/* XXX - was >> 4 */

  HASH_MIX(a, b, c);
  return c;
}

#if 0
static unsigned int
which_bucket (mem)
     PTR_T mem;
{
  return (mt_hash ((unsigned char *)mem) & (REG_TABLE_SIZE-1));
}
#else
#define which_bucket(mem) (mt_hash ((unsigned char *)(mem)) & (REG_TABLE_SIZE-1));
#endif

static mr_table_t *
find_entry (mem, flags)
     PTR_T mem;
     int flags;
{
  unsigned int bucket;
  register mr_table_t *tp;
  mr_table_t *endp, *lastp;

  if (mem_overflow.mem == mem)
    return (&mem_overflow);

  bucket = which_bucket (mem);	/* get initial hash */
  tp = endp = mem_table + bucket;
  lastp = mem_table + REG_TABLE_SIZE;

  while (1)
    {
      if (tp->mem == mem)
	return (tp);
      if (tp->mem == 0 && (flags & FIND_ALLOC))
	{
	  table_count++;
	  return (tp);
	}

      tp++;

      if (tp == lastp)		/* wrap around */
        tp = mem_table;

      if (tp == endp && (flags & FIND_EXIST))
        return ((mr_table_t *)NULL);

      if (tp == endp && (flags & FIND_ALLOC))
        break;
    }

  /* oops.  table is full.  replace an existing free entry. */
  do
    {
      /* If there are no free entries, punt right away without searching. */
      if (table_allocated == REG_TABLE_SIZE)
	break;

      if (tp->flags & MT_FREE)
	{
	  memset(tp, 0, sizeof (mr_table_t));
	  return (tp);
	}
      tp++;

      if (tp == lastp)
	tp = mem_table;
    }
  while (tp != endp);

  /* wow. entirely full.  return mem_overflow dummy entry. */
  tp = &mem_overflow;
  memset (tp, 0, sizeof (mr_table_t));
  return tp;
}

mr_table_t *
mr_table_entry (mem)
     PTR_T mem;
{
  return (find_entry (mem, FIND_EXIST));
}

void
mregister_describe_mem (mem, fp)
     PTR_T mem;
     FILE *fp;
{
  mr_table_t *entry;

  entry = find_entry (mem, FIND_EXIST);
  if (entry == 0)
    return;
  fprintf (fp, "malloc: %p: %s: last %s from %s:%d\n",
  		mem,
		(entry->flags & MT_ALLOC) ? "allocated" : "free",
		(entry->flags & MT_ALLOC) ? "allocated" : "freed",
		entry->file ? entry->file : "unknown",
		entry->line);
}

void
mregister_alloc (tag, mem, size, file, line)
     const char *tag;
     PTR_T mem;
     size_t size;
     const char *file;
     int line;
{
  mr_table_t *tentry;

  tentry = find_entry (mem, FIND_ALLOC);

  if (tentry == 0)
    {
      /* oops.  table is full.  punt. */
      fprintf (stderr, _("register_alloc: alloc table is full with FIND_ALLOC?\n"));
      return;
    }
  
  if (tentry->flags & MT_ALLOC)
    {
      /* oops.  bad bookkeeping. ignore for now */
      fprintf (stderr, _("register_alloc: %p already in table as allocated?\n"), mem);
    }

  tentry->mem = mem;
  tentry->size = size;
  tentry->func = tag;
  tentry->flags = MT_ALLOC;
  tentry->file = file;
  tentry->line = line;
  tentry->nalloc++;

  if (tentry != &mem_overflow)
    table_allocated++;
}

void
mregister_free (mem, size, file, line)
     PTR_T mem;
     int size;
     const char *file;
     int line;
{
  mr_table_t *tentry;

  tentry = find_entry (mem, FIND_EXIST);
  if (tentry == 0)
    {
      /* oops.  not found. */
#if 0
      fprintf (stderr, "register_free: %p not in allocation table?\n", mem);
#endif
      return;
    }
  if (tentry->flags & MT_FREE)
    {
      /* oops.  bad bookkeeping. ignore for now */
      fprintf (stderr, _("register_free: %p already in table as free?\n"), mem);
    }
    	
  tentry->flags = MT_FREE;
  tentry->func = "free";
  tentry->file = file;
  tentry->line = line;
  tentry->nfree++;

  if (tentry != &mem_overflow)
    table_allocated--;
}

/* If we ever add more flags, this will require changes. */
static char *
_entry_flags(x)
     int x;
{
  if (x & MT_FREE)
    return "free";
  else if (x & MT_ALLOC)
    return "allocated";
  else
    return "undetermined?";
}

static void
_register_dump_table(fp)
     FILE *fp;
{
  register int i;
  mr_table_t entry;

  for (i = 0; i < REG_TABLE_SIZE; i++)
    {
      entry = mem_table[i];
      if (entry.mem)
	fprintf (fp, "[%d] %p:%d:%s:%s:%s:%d:%d:%d\n", i,
						entry.mem, entry.size,
						_entry_flags(entry.flags),
						entry.func ? entry.func : "unknown",
						entry.file ? entry.file : "unknown",
						entry.line,
						entry.nalloc, entry.nfree);
    }
}
 
void
mregister_dump_table()
{
  _register_dump_table (stderr);
}

void
mregister_table_init ()
{
  memset (mem_table, 0, sizeof(mr_table_t) * REG_TABLE_SIZE);
  memset (&mem_overflow, 0, sizeof (mr_table_t));
  table_count = 0;
}

#endif /* MALLOC_REGISTER */

int
malloc_set_register(n)
     int n;
{
  int old;

  old = malloc_register;
  malloc_register = n;
  return old;
}
