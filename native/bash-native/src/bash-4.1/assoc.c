/*
 * assoc.c - functions to manipulate associative arrays
 *
 * Associative arrays are standard shell hash tables.
 *
 * Chet Ramey
 * chet@ins.cwru.edu
 */

/* Copyright (C) 2008,2009 Free Software Foundation, Inc.

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

#include "config.h"

#if defined (ARRAY_VARS)

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include <stdio.h>
#include "bashansi.h"

#include "shell.h"
#include "array.h"
#include "assoc.h"
#include "builtins/common.h"

static WORD_LIST *assoc_to_word_list_internal __P((HASH_TABLE *, int));

/* assoc_create == hash_create */

void
assoc_dispose (hash)
     HASH_TABLE *hash;
{
  if (hash)
    {
      hash_flush (hash, 0);
      hash_dispose (hash);
    }
}

void
assoc_flush (hash)
     HASH_TABLE *hash;
{
  hash_flush (hash, 0);
}
     
int
assoc_insert (hash, key, value)
     HASH_TABLE *hash;
     char *key;
     char *value;
{
  BUCKET_CONTENTS *b;

  b = hash_search (key, hash, HASH_CREATE);
  if (b == 0)
    return -1;
  FREE (b->data);
  b->data = value ? savestring (value) : (char *)0;
  return (0);
}

void
assoc_remove (hash, string)
     HASH_TABLE *hash;
     char *string;
{
  BUCKET_CONTENTS *b;

  b = hash_remove (string, hash, 0);
  if (b)
    {
      free ((char *)b->data);
      free (b->key);
      free (b);
    }
}

char *
assoc_reference (hash, string)
     HASH_TABLE *hash;
     char *string;
{
  BUCKET_CONTENTS *b;

  if (hash == 0)
    return (char *)0;

  b = hash_search (string, hash, 0);
  return (b ? (char *)b->data : 0);
}

/* Quote the data associated with each element of the hash table ASSOC,
   using quote_string */
HASH_TABLE *
assoc_quote (h)
     HASH_TABLE *h;
{
  int i;
  BUCKET_CONTENTS *tlist;
  char *t;

  if (h == 0 || assoc_empty (h))
    return ((HASH_TABLE *)NULL);
  
  for (i = 0; i < h->nbuckets; i++)
    for (tlist = hash_items (i, h); tlist; tlist = tlist->next)
      {
	t = quote_string ((char *)tlist->data);
	FREE (tlist->data);
	tlist->data = t;
      }

  return h;
}

/* Quote escape characters in the data associated with each element
   of the hash table ASSOC, using quote_escapes */
HASH_TABLE *
assoc_quote_escapes (h)
     HASH_TABLE *h;
{
  int i;
  BUCKET_CONTENTS *tlist;
  char *t;

  if (h == 0 || assoc_empty (h))
    return ((HASH_TABLE *)NULL);
  
  for (i = 0; i < h->nbuckets; i++)
    for (tlist = hash_items (i, h); tlist; tlist = tlist->next)
      {
	t = quote_escapes ((char *)tlist->data);
	FREE (tlist->data);
	tlist->data = t;
      }

  return h;
}

HASH_TABLE *
assoc_dequote (h)
     HASH_TABLE *h;
{
  int i;
  BUCKET_CONTENTS *tlist;
  char *t;

  if (h == 0 || assoc_empty (h))
    return ((HASH_TABLE *)NULL);
  
  for (i = 0; i < h->nbuckets; i++)
    for (tlist = hash_items (i, h); tlist; tlist = tlist->next)
      {
	t = dequote_string ((char *)tlist->data);
	FREE (tlist->data);
	tlist->data = t;
      }

  return h;
}

HASH_TABLE *
assoc_dequote_escapes (h)
     HASH_TABLE *h;
{
  int i;
  BUCKET_CONTENTS *tlist;
  char *t;

  if (h == 0 || assoc_empty (h))
    return ((HASH_TABLE *)NULL);
  
  for (i = 0; i < h->nbuckets; i++)
    for (tlist = hash_items (i, h); tlist; tlist = tlist->next)
      {
	t = dequote_escapes ((char *)tlist->data);
	FREE (tlist->data);
	tlist->data = t;
      }

  return h;
}

HASH_TABLE *
assoc_remove_quoted_nulls (h)
     HASH_TABLE *h;
{
  int i;
  BUCKET_CONTENTS *tlist;
  char *t;

  if (h == 0 || assoc_empty (h))
    return ((HASH_TABLE *)NULL);
  
  for (i = 0; i < h->nbuckets; i++)
    for (tlist = hash_items (i, h); tlist; tlist = tlist->next)
      {
	t = remove_quoted_nulls ((char *)tlist->data);
	tlist->data = t;
      }

  return h;
}

/*
 * Return a string whose elements are the members of array H beginning at
 * the STARTth element and spanning NELEM members.  Null elements are counted.
 */
char *
assoc_subrange (hash, start, nelem, starsub, quoted)
HASH_TABLE *hash;
arrayind_t start, nelem;
int starsub, quoted;
{
  WORD_LIST *l, *save, *h, *t;
  int i, j;
  char *ret;

  if (assoc_empty (hash))
    return ((char *)NULL);

  save = l = assoc_to_word_list (hash);
  if (save == 0)
    return ((char *)NULL);

  for (i = 1; l && i < start; i++)
    l = l->next;
  if (l == 0)
    return ((char *)NULL);
  for (j = 0,h = t = l; l && j < nelem; j++)
    {
      t = l;
      l = l->next;
    }

  t->next = (WORD_LIST *)NULL;

  ret = string_list_pos_params (starsub ? '*' : '@', h, quoted);

  if (t != l)
    t->next = l;

  dispose_words (save);
  return (ret);

}

char *
assoc_patsub (h, pat, rep, mflags)
     HASH_TABLE *h;
     char *pat, *rep;
     int mflags;
{
  BUCKET_CONTENTS *tlist;
  int i, slen;
  HASH_TABLE *h2;
  char	*t, *sifs, *ifs;

  if (h == 0 || assoc_empty (h))
    return ((char *)NULL);

  h2 = assoc_copy (h);
  for (i = 0; i < h2->nbuckets; i++)
    for (tlist = hash_items (i, h2); tlist; tlist = tlist->next)
      {
	t = pat_subst ((char *)tlist->data, pat, rep, mflags);
	FREE (tlist->data);
	tlist->data = t;
      }

  if (mflags & MATCH_QUOTED)
    assoc_quote (h2);
  else
    assoc_quote_escapes (h2);

  if (mflags & MATCH_STARSUB)
    {
      assoc_remove_quoted_nulls (h2);
      sifs = ifs_firstchar ((int *)NULL);
      t = assoc_to_string (h2, sifs, 0);
      free (sifs);
    }
  else if (mflags & MATCH_QUOTED)
    {
      /* ${array[@]} */
      sifs = ifs_firstchar (&slen);
      ifs = getifs ();
      if (ifs == 0 || *ifs == 0)
	{
	  if (slen < 2)
	    sifs = xrealloc (sifs, 2);
	  sifs[0] = ' ';
	  sifs[1] = '\0';
	}
      t = assoc_to_string (h2, sifs, 0);
      free(sifs);
    }
  else
    t = assoc_to_string (h2, " ", 0);

  assoc_dispose (h2);

  return t;
}

char *
assoc_modcase (h, pat, modop, mflags)
     HASH_TABLE *h;
     char *pat;
     int modop;
     int mflags;
{
  BUCKET_CONTENTS *tlist;
  int i, slen;
  HASH_TABLE *h2;
  char	*t, *sifs, *ifs;

  if (h == 0 || assoc_empty (h))
    return ((char *)NULL);

  h2 = assoc_copy (h);
  for (i = 0; i < h2->nbuckets; i++)
    for (tlist = hash_items (i, h2); tlist; tlist = tlist->next)
      {
	t = sh_modcase ((char *)tlist->data, pat, modop);
	FREE (tlist->data);
	tlist->data = t;
      }

  if (mflags & MATCH_QUOTED)
    assoc_quote (h2);
  else
    assoc_quote_escapes (h2);

  if (mflags & MATCH_STARSUB)
    {
      assoc_remove_quoted_nulls (h2);
      sifs = ifs_firstchar ((int *)NULL);
      t = assoc_to_string (h2, sifs, 0);
      free (sifs);
    }
  else if (mflags & MATCH_QUOTED)
    {
      /* ${array[@]} */
      sifs = ifs_firstchar (&slen);
      ifs = getifs ();
      if (ifs == 0 || *ifs == 0)
	{
	  if (slen < 2)
	    sifs = xrealloc (sifs, 2);
	  sifs[0] = ' ';
	  sifs[1] = '\0';
	}
      t = assoc_to_string (h2, sifs, 0);
      free(sifs);
    }
  else
    t = assoc_to_string (h2, " ", 0);

  assoc_dispose (h2);

  return t;
}

char *
assoc_to_assign (hash, quoted)
     HASH_TABLE *hash;
     int quoted;
{
  char *ret;
  char *istr, *vstr;
  int i, rsize, rlen, elen;
  BUCKET_CONTENTS *tlist;

  if (hash == 0 || assoc_empty (hash))
    return (char *)0;

  ret = xmalloc (rsize = 128);
  ret[0] = '(';
  rlen = 1;

  for (i = 0; i < hash->nbuckets; i++)
    for (tlist = hash_items (i, hash); tlist; tlist = tlist->next)
      {
#if 1
	if (sh_contains_shell_metas (tlist->key))
	  istr = sh_double_quote (tlist->key);
	else
	  istr = tlist->key;	
#else
	istr = tlist->key;
#endif
	vstr = tlist->data ? sh_double_quote ((char *)tlist->data) : (char *)0;

	elen = STRLEN (istr) + 8 + STRLEN (vstr);
	RESIZE_MALLOCED_BUFFER (ret, rlen, (elen+1), rsize, rsize);

	ret[rlen++] = '[';
	strcpy (ret+rlen, istr);
	rlen += STRLEN (istr);
	ret[rlen++] = ']';
	ret[rlen++] = '=';
	if (vstr)
	  {
	    strcpy (ret + rlen, vstr);
	    rlen += STRLEN (vstr);
	  }
	ret[rlen++] = ' ';


	if (istr != tlist->key)
	  FREE (istr);

	FREE (vstr);
    }

  RESIZE_MALLOCED_BUFFER (ret, rlen, 1, rsize, 8);
  ret[rlen++] = ')';
  ret[rlen] = '\0';

  if (quoted)
    {
      vstr = sh_single_quote (ret);
      free (ret);
      ret = vstr;
    }

  return ret;
}

static WORD_LIST *
assoc_to_word_list_internal (h, t)
     HASH_TABLE *h;
     int t;
{
  WORD_LIST *list;
  int i;
  BUCKET_CONTENTS *tlist;
  char *w;

  if (h == 0 || assoc_empty (h))
    return((WORD_LIST *)NULL);
  list = (WORD_LIST *)NULL;
  
  for (i = 0; i < h->nbuckets; i++)
    for (tlist = hash_items (i, h); tlist; tlist = tlist->next)
      {
	w = (t == 0) ? (char *)tlist->data : (char *)tlist->key;
	list = make_word_list (make_bare_word(w), list);
      }
  return (REVERSE_LIST(list, WORD_LIST *));
}

WORD_LIST *
assoc_to_word_list (h)
     HASH_TABLE *h;
{
  return (assoc_to_word_list_internal (h, 0));
}

WORD_LIST *
assoc_keys_to_word_list (h)
     HASH_TABLE *h;
{
  return (assoc_to_word_list_internal (h, 1));
}

char *
assoc_to_string (h, sep, quoted)
     HASH_TABLE *h;
     char *sep;
     int quoted;
{
  BUCKET_CONTENTS *tlist;
  int i;
  char *result, *t, *w;
  WORD_LIST *list, *l;

  if (h == 0)
    return ((char *)NULL);
  if (assoc_empty (h))
    return (savestring (""));

  result = NULL;
  list = NULL;
  /* This might be better implemented directly, but it's simple to implement
     by converting to a word list first, possibly quoting the data, then
     using list_string */
  for (i = 0; i < h->nbuckets; i++)
    for (tlist = hash_items (i, h); tlist; tlist = tlist->next)
      {
	w = (char *)tlist->data;
	if (w == 0)
	  continue;
	t = quoted ? quote_string (w) : savestring (w);
	list = make_word_list (make_bare_word(t), list);
	FREE (t);
      }

  l = REVERSE_LIST(list, WORD_LIST *);

  result = l ? string_list_internal (l, sep) : savestring ("");
  return result;
}

#endif /* ARRAY_VARS */
