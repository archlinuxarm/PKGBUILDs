/*
 * array.c - functions to create, destroy, access, and manipulate arrays
 *	     of strings.
 *
 * Arrays are sparse doubly-linked lists.  An element's index is stored
 * with it.
 *
 * Chet Ramey
 * chet@ins.cwru.edu
 */

/* Copyright (C) 1997-2009 Free Software Foundation, Inc.

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
#include "builtins/common.h"

#define ADD_BEFORE(ae, new) \
	do { \
		ae->prev->next = new; \
		new->prev = ae->prev; \
		ae->prev = new; \
		new->next = ae; \
	} while(0)

static char *array_to_string_internal __P((ARRAY_ELEMENT *, ARRAY_ELEMENT *, char *, int));

static ARRAY *lastarray = 0;
static ARRAY_ELEMENT *lastref = 0;

#define IS_LASTREF(a)	((a) == lastarray)

#define INVALIDATE_LASTREF(a) \
do { \
	if ((a) == lastarray) { \
		lastarray = 0; \
		lastref = 0; \
	} \
} while (0)

#define SET_LASTREF(a, e) \
do { \
	lastarray = (a); \
	lastref = (e); \
} while (0)

#define UNSET_LASTREF() \
do { \
	lastarray = 0; \
	lastref = 0; \
} while (0)

ARRAY *
array_create()
{
	ARRAY	*r;
	ARRAY_ELEMENT	*head;

	r =(ARRAY *)xmalloc(sizeof(ARRAY));
	r->type = array_indexed;
	r->max_index = -1;
	r->num_elements = 0;
	head = array_create_element(-1, (char *)NULL);	/* dummy head */
	head->prev = head->next = head;
	r->head = head;
	return(r);
}

void
array_flush (a)
ARRAY	*a;
{
	register ARRAY_ELEMENT *r, *r1;

	if (a == 0)
		return;
	for (r = element_forw(a->head); r != a->head; ) {
		r1 = element_forw(r);
		array_dispose_element(r);
		r = r1;
	}
	a->head->next = a->head->prev = a->head;
	a->max_index = -1;
	a->num_elements = 0;
	INVALIDATE_LASTREF(a);
}

void
array_dispose(a)
ARRAY	*a;
{
	if (a == 0)
		return;
	array_flush (a);
	array_dispose_element(a->head);
	free(a);
}

ARRAY *
array_copy(a)
ARRAY	*a;
{
	ARRAY	*a1;
	ARRAY_ELEMENT	*ae, *new;

	if (a == 0)
		return((ARRAY *) NULL);
	a1 = array_create();
	a1->type = a->type;
	a1->max_index = a->max_index;
	a1->num_elements = a->num_elements;
	for (ae = element_forw(a->head); ae != a->head; ae = element_forw(ae)) {
		new = array_create_element(element_index(ae), element_value(ae));
		ADD_BEFORE(a1->head, new);
	}
	return(a1);
}

/*
 * Make and return a new array composed of the elements in array A from
 * S to E, inclusive.
 */
ARRAY *
array_slice(array, s, e)
ARRAY		*array;
ARRAY_ELEMENT	*s, *e;
{
	ARRAY	*a;
	ARRAY_ELEMENT *p, *n;
	int	i;
	arrayind_t mi;

	a = array_create ();
	a->type = array->type;

	for (mi = 0, p = s, i = 0; p != e; p = element_forw(p), i++) {
		n = array_create_element (element_index(p), element_value(p));
		ADD_BEFORE(a->head, n);
		mi = element_index(n);
	}
	a->num_elements = i;
	a->max_index = mi;
	return a;
}

/*
 * Walk the array, calling FUNC once for each element, with the array
 * element as the argument.
 */
void
array_walk(a, func, udata)
ARRAY	*a;
sh_ae_map_func_t *func;
void	*udata;
{
	register ARRAY_ELEMENT *ae;

	if (a == 0 || array_empty(a))
		return;
	for (ae = element_forw(a->head); ae != a->head; ae = element_forw(ae))
		if ((*func)(ae, udata) < 0)
			return;
}

/*
 * Shift the array A N elements to the left.  Delete the first N elements
 * and subtract N from the indices of the remaining elements.  If FLAGS
 * does not include AS_DISPOSE, this returns a singly-linked null-terminated
 * list of elements so the caller can dispose of the chain.  If FLAGS
 * includes AS_DISPOSE, this function disposes of the shifted-out elements
 * and returns NULL.
 */
ARRAY_ELEMENT *
array_shift(a, n, flags)
ARRAY	*a;
int	n, flags;
{
	register ARRAY_ELEMENT *ae, *ret;
	register int i;

	if (a == 0 || array_empty(a) || n <= 0)
		return ((ARRAY_ELEMENT *)NULL);

	INVALIDATE_LASTREF(a);
	for (i = 0, ret = ae = element_forw(a->head); ae != a->head && i < n; ae = element_forw(ae), i++)
		;
	if (ae == a->head) {
		/* Easy case; shifting out all of the elements */
		if (flags & AS_DISPOSE) {
			array_flush (a);
			return ((ARRAY_ELEMENT *)NULL);
		}
		for (ae = ret; element_forw(ae) != a->head; ae = element_forw(ae))
			;
		element_forw(ae) = (ARRAY_ELEMENT *)NULL;
		a->head->next = a->head->prev = a->head;
		a->max_index = -1;
		a->num_elements = 0;
		return ret;
	}
	/*
	 * ae now points to the list of elements we want to retain.
	 * ret points to the list we want to either destroy or return.
	 */
	ae->prev->next = (ARRAY_ELEMENT *)NULL;		/* null-terminate RET */

	a->head->next = ae;		/* slice RET out of the array */
	ae->prev = a->head;

	for ( ; ae != a->head; ae = element_forw(ae))
		element_index(ae) -= n;	/* renumber retained indices */

	a->num_elements -= n;		/* modify bookkeeping information */
	a->max_index = element_index(a->head->prev);

	if (flags & AS_DISPOSE) {
		for (ae = ret; ae; ) {
			ret = element_forw(ae);
			array_dispose_element(ae);
			ae = ret;
		}
		return ((ARRAY_ELEMENT *)NULL);
	}

	return ret;
}

/*
 * Shift array A right N indices.  If S is non-null, it becomes the value of
 * the new element 0.  Returns the number of elements in the array after the
 * shift.
 */
int
array_rshift (a, n, s)
ARRAY	*a;
int	n;
char	*s;
{
	register ARRAY_ELEMENT	*ae, *new;

	if (a == 0 || (array_empty(a) && s == 0))
		return 0;
	else if (n <= 0)
		return (a->num_elements);

	ae = element_forw(a->head);
	if (s) {
		new = array_create_element(0, s);
		ADD_BEFORE(ae, new);
		a->num_elements++;
		if (array_num_elements(a) == 1)	{	/* array was empty */
			a->max_index = 0;
			return 1;
		}
	}

	/*
	 * Renumber all elements in the array except the one we just added.
	 */
	for ( ; ae != a->head; ae = element_forw(ae))
		element_index(ae) += n;

	a->max_index = element_index(a->head->prev);

	INVALIDATE_LASTREF(a);
	return (a->num_elements);
}

ARRAY_ELEMENT *
array_unshift_element(a)
ARRAY	*a;
{
	return (array_shift (a, 1, 0));
}

int
array_shift_element(a, v)
ARRAY	*a;
char	*v;
{
	return (array_rshift (a, 1, v));
}

ARRAY *
array_quote(array)
ARRAY	*array;
{
	ARRAY_ELEMENT	*a;
	char	*t;

	if (array == 0 || array_head(array) == 0 || array_empty(array))
		return (ARRAY *)NULL;
	for (a = element_forw(array->head); a != array->head; a = element_forw(a)) {
		t = quote_string (a->value);
		FREE(a->value);
		a->value = t;
	}
	return array;
}

ARRAY *
array_quote_escapes(array)
ARRAY	*array;
{
	ARRAY_ELEMENT	*a;
	char	*t;

	if (array == 0 || array_head(array) == 0 || array_empty(array))
		return (ARRAY *)NULL;
	for (a = element_forw(array->head); a != array->head; a = element_forw(a)) {
		t = quote_escapes (a->value);
		FREE(a->value);
		a->value = t;
	}
	return array;
}

ARRAY *
array_dequote(array)
ARRAY	*array;
{
	ARRAY_ELEMENT	*a;
	char	*t;

	if (array == 0 || array_head(array) == 0 || array_empty(array))
		return (ARRAY *)NULL;
	for (a = element_forw(array->head); a != array->head; a = element_forw(a)) {
		t = dequote_string (a->value);
		FREE(a->value);
		a->value = t;
	}
	return array;
}

ARRAY *
array_dequote_escapes(array)
ARRAY	*array;
{
	ARRAY_ELEMENT	*a;
	char	*t;

	if (array == 0 || array_head(array) == 0 || array_empty(array))
		return (ARRAY *)NULL;
	for (a = element_forw(array->head); a != array->head; a = element_forw(a)) {
		t = dequote_escapes (a->value);
		FREE(a->value);
		a->value = t;
	}
	return array;
}

ARRAY *
array_remove_quoted_nulls(array)
ARRAY	*array;
{
	ARRAY_ELEMENT	*a;
	char	*t;

	if (array == 0 || array_head(array) == 0 || array_empty(array))
		return (ARRAY *)NULL;
	for (a = element_forw(array->head); a != array->head; a = element_forw(a))
		a->value = remove_quoted_nulls (a->value);
	return array;
}

/*
 * Return a string whose elements are the members of array A beginning at
 * index START and spanning NELEM members.  Null elements are counted.
 * Since arrays are sparse, unset array elements are not counted.
 */
char *
array_subrange (a, start, nelem, starsub, quoted)
ARRAY	*a;
arrayind_t	start, nelem;
int	starsub, quoted;
{
	ARRAY		*a2;
	ARRAY_ELEMENT	*h, *p;
	arrayind_t	i;
	char		*ifs, *sifs, *t;
	int		slen;

	p = a ? array_head (a) : 0;
	if (p == 0 || array_empty (a) || start > array_max_index(a))
		return ((char *)NULL);

	/*
	 * Find element with index START.  If START corresponds to an unset
	 * element (arrays can be sparse), use the first element whose index
	 * is >= START.  If START is < 0, we count START indices back from
	 * the end of A (not elements, even with sparse arrays -- START is an
	 * index).
	 */
	for (p = element_forw(p); p != array_head(a) && start > element_index(p); p = element_forw(p))
		;

	if (p == a->head)
		return ((char *)NULL);

	/* Starting at P, take NELEM elements, inclusive. */
	for (i = 0, h = p; p != a->head && i < nelem; i++, p = element_forw(p))
		;

	a2 = array_slice(a, h, p);

	if (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT))
		array_quote(a2);
	else
		array_quote_escapes(a2);

	if (starsub && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT))) {
		/* ${array[*]} */
		array_remove_quoted_nulls (a2);
		sifs = ifs_firstchar ((int *)NULL);
		t = array_to_string (a2, sifs, 0);
		free (sifs);
	} else if (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) {
		/* ${array[@]} */
		sifs = ifs_firstchar (&slen);
		ifs = getifs ();
		if (ifs == 0 || *ifs == 0) {
			if (slen < 2)
				sifs = xrealloc(sifs, 2);
			sifs[0] = ' ';
			sifs[1] = '\0';
		}
		t = array_to_string (a2, sifs, 0);
		free (sifs);
	} else
		t = array_to_string (a2, " ", 0);
	array_dispose(a2);

	return t;
}

char *
array_patsub (a, pat, rep, mflags)
ARRAY	*a;
char	*pat, *rep;
int	mflags;
{
	ARRAY		*a2;
	ARRAY_ELEMENT	*e;
	char	*t, *sifs, *ifs;
	int	slen;

	if (a == 0 || array_head(a) == 0 || array_empty(a))
		return ((char *)NULL);

	a2 = array_copy(a);
	for (e = element_forw(a2->head); e != a2->head; e = element_forw(e)) {
		t = pat_subst(element_value(e), pat, rep, mflags);
		FREE(element_value(e));
		e->value = t;
	}

	if (mflags & MATCH_QUOTED)
		array_quote(a2);
	else
		array_quote_escapes(a2);

	if (mflags & MATCH_STARSUB) {
		array_remove_quoted_nulls (a2);
		sifs = ifs_firstchar((int *)NULL);
		t = array_to_string (a2, sifs, 0);
		free(sifs);
	} else if (mflags & MATCH_QUOTED) {
		/* ${array[@]} */
		sifs = ifs_firstchar (&slen);
		ifs = getifs ();
		if (ifs == 0 || *ifs == 0) {
			if (slen < 2)
				sifs = xrealloc (sifs, 2);
			sifs[0] = ' ';
			sifs[1] = '\0';
		}
		t = array_to_string (a2, sifs, 0);
		free(sifs);
	} else
		t = array_to_string (a2, " ", 0);
	array_dispose (a2);

	return t;
}

char *
array_modcase (a, pat, modop, mflags)
ARRAY	*a;
char	*pat;
int	modop;
int	mflags;
{
	ARRAY		*a2;
	ARRAY_ELEMENT	*e;
	char	*t, *sifs, *ifs;
	int	slen;

	if (a == 0 || array_head(a) == 0 || array_empty(a))
		return ((char *)NULL);

	a2 = array_copy(a);
	for (e = element_forw(a2->head); e != a2->head; e = element_forw(e)) {
		t = sh_modcase(element_value(e), pat, modop);
		FREE(element_value(e));
		e->value = t;
	}

	if (mflags & MATCH_QUOTED)
		array_quote(a2);
	else
		array_quote_escapes(a2);

	if (mflags & MATCH_STARSUB) {
		array_remove_quoted_nulls (a2);
		sifs = ifs_firstchar((int *)NULL);
		t = array_to_string (a2, sifs, 0);
		free(sifs);
	} else if (mflags & MATCH_QUOTED) {
		/* ${array[@]} */
		sifs = ifs_firstchar (&slen);
		ifs = getifs ();
		if (ifs == 0 || *ifs == 0) {
			if (slen < 2)
				sifs = xrealloc (sifs, 2);
			sifs[0] = ' ';
			sifs[1] = '\0';
		}
		t = array_to_string (a2, sifs, 0);
		free(sifs);
	} else
		t = array_to_string (a2, " ", 0);
	array_dispose (a2);

	return t;
}
/*
 * Allocate and return a new array element with index INDEX and value
 * VALUE.
 */
ARRAY_ELEMENT *
array_create_element(indx, value)
arrayind_t	indx;
char	*value;
{
	ARRAY_ELEMENT *r;

	r = (ARRAY_ELEMENT *)xmalloc(sizeof(ARRAY_ELEMENT));
	r->ind = indx;
	r->value = value ? savestring(value) : (char *)NULL;
	r->next = r->prev = (ARRAY_ELEMENT *) NULL;
	return(r);
}

#ifdef INCLUDE_UNUSED
ARRAY_ELEMENT *
array_copy_element(ae)
ARRAY_ELEMENT	*ae;
{
	return(ae ? array_create_element(element_index(ae), element_value(ae))
		  : (ARRAY_ELEMENT *) NULL);
}
#endif

void
array_dispose_element(ae)
ARRAY_ELEMENT	*ae;
{
	if (ae) {
		FREE(ae->value);
		free(ae);
	}
}

/*
 * Add a new element with index I and value V to array A (a[i] = v).
 */
int
array_insert(a, i, v)
ARRAY	*a;
arrayind_t	i;
char	*v;
{
	register ARRAY_ELEMENT *new, *ae;

	if (a == 0)
		return(-1);
	new = array_create_element(i, v);
	if (i > array_max_index(a)) {
		/*
		 * Hook onto the end.  This also works for an empty array.
		 * Fast path for the common case of allocating arrays
		 * sequentially.
		 */
		ADD_BEFORE(a->head, new);
		a->max_index = i;
		a->num_elements++;
		SET_LASTREF(a, new);
		return(0);
	}
	/*
	 * Otherwise we search for the spot to insert it.
	 */
	for (ae = element_forw(a->head); ae != a->head; ae = element_forw(ae)) {
		if (element_index(ae) == i) {
			/*
			 * Replacing an existing element.
			 */
			array_dispose_element(new);
			free(element_value(ae));
			ae->value = v ? savestring(v) : (char *)NULL;
			SET_LASTREF(a, ae);
			return(0);
		} else if (element_index(ae) > i) {
			ADD_BEFORE(ae, new);
			a->num_elements++;
			SET_LASTREF(a, new);
			return(0);
		}
	}
	INVALIDATE_LASTREF(a);
	return (-1);		/* problem */
}

/*
 * Delete the element with index I from array A and return it so the
 * caller can dispose of it.
 */
ARRAY_ELEMENT *
array_remove(a, i)
ARRAY	*a;
arrayind_t	i;
{
	register ARRAY_ELEMENT *ae;

	if (a == 0 || array_empty(a))
		return((ARRAY_ELEMENT *) NULL);
	for (ae = element_forw(a->head); ae != a->head; ae = element_forw(ae))
		if (element_index(ae) == i) {
			ae->next->prev = ae->prev;
			ae->prev->next = ae->next;
			a->num_elements--;
			if (i == array_max_index(a))
				a->max_index = element_index(ae->prev);
			INVALIDATE_LASTREF(a);
			return(ae);
		}
	return((ARRAY_ELEMENT *) NULL);
}

/*
 * Return the value of a[i].
 */
char *
array_reference(a, i)
ARRAY	*a;
arrayind_t	i;
{
	register ARRAY_ELEMENT *ae;

	if (a == 0 || array_empty(a))
		return((char *) NULL);
	if (i > array_max_index(a))
		return((char *)NULL);
	/* Keep roving pointer into array to optimize sequential access */
	if (lastref && IS_LASTREF(a))
		ae = (i >= element_index(lastref)) ? lastref : element_forw(a->head);
	else
		ae = element_forw(a->head);
	for ( ; ae != a->head; ae = element_forw(ae))
		if (element_index(ae) == i) {
			SET_LASTREF(a, ae);
			return(element_value(ae));
		}
	UNSET_LASTREF();
	return((char *) NULL);
}

/* Convenience routines for the shell to translate to and from the form used
   by the rest of the code. */

WORD_LIST *
array_to_word_list(a)
ARRAY	*a;
{
	WORD_LIST	*list;
	ARRAY_ELEMENT	*ae;

	if (a == 0 || array_empty(a))
		return((WORD_LIST *)NULL);
	list = (WORD_LIST *)NULL;
	for (ae = element_forw(a->head); ae != a->head; ae = element_forw(ae))
		list = make_word_list (make_bare_word(element_value(ae)), list);
	return (REVERSE_LIST(list, WORD_LIST *));
}

ARRAY *
array_from_word_list (list)
WORD_LIST	*list;
{
	ARRAY	*a;

	if (list == 0)
		return((ARRAY *)NULL);
	a = array_create();
	return (array_assign_list (a, list));
}

WORD_LIST *
array_keys_to_word_list(a)
ARRAY	*a;
{
	WORD_LIST	*list;
	ARRAY_ELEMENT	*ae;
	char		*t;

	if (a == 0 || array_empty(a))
		return((WORD_LIST *)NULL);
	list = (WORD_LIST *)NULL;
	for (ae = element_forw(a->head); ae != a->head; ae = element_forw(ae)) {
		t = itos(element_index(ae));
		list = make_word_list (make_bare_word(t), list);
		free(t);
	}
	return (REVERSE_LIST(list, WORD_LIST *));
}

ARRAY *
array_assign_list (array, list)
ARRAY	*array;
WORD_LIST	*list;
{
	register WORD_LIST *l;
	register arrayind_t i;

	for (l = list, i = 0; l; l = l->next, i++)
		array_insert(array, i, l->word->word);
	return array;
}

char **
array_to_argv (a)
ARRAY	*a;
{
	char		**ret, *t;
	int		i;
	ARRAY_ELEMENT	*ae;

	if (a == 0 || array_empty(a))
		return ((char **)NULL);
	ret = strvec_create (array_num_elements (a) + 1);
	i = 0;
	for (ae = element_forw(a->head); ae != a->head; ae = element_forw(ae)) {
		t = element_value (ae);
		ret[i++] = t ? savestring (t) : (char *)NULL;
	}
	ret[i] = (char *)NULL;
	return (ret);
}
	
/*
 * Return a string that is the concatenation of the elements in A from START
 * to END, separated by SEP.
 */
static char *
array_to_string_internal (start, end, sep, quoted)
ARRAY_ELEMENT	*start, *end;
char	*sep;
int	quoted;
{
	char	*result, *t;
	ARRAY_ELEMENT *ae;
	int	slen, rsize, rlen, reg;

	if (start == end)	/* XXX - should not happen */
		return ((char *)NULL);

	slen = strlen(sep);
	result = NULL;
	for (rsize = rlen = 0, ae = start; ae != end; ae = element_forw(ae)) {
		if (rsize == 0)
			result = (char *)xmalloc (rsize = 64);
		if (element_value(ae)) {
			t = quoted ? quote_string(element_value(ae)) : element_value(ae);
			reg = strlen(t);
			RESIZE_MALLOCED_BUFFER (result, rlen, (reg + slen + 2),
						rsize, rsize);
			strcpy(result + rlen, t);
			rlen += reg;
			if (quoted && t)
				free(t);
			/*
			 * Add a separator only after non-null elements.
			 */
			if (element_forw(ae) != end) {
				strcpy(result + rlen, sep);
				rlen += slen;
			}
		}
	}
	if (result)
	  result[rlen] = '\0';	/* XXX */
	return(result);
}

char *
array_to_assign (a, quoted)
ARRAY	*a;
int	quoted;
{
	char	*result, *valstr, *is;
	char	indstr[INT_STRLEN_BOUND(intmax_t) + 1];
	ARRAY_ELEMENT *ae;
	int	rsize, rlen, elen;

	if (a == 0 || array_empty (a))
		return((char *)NULL);

	result = (char *)xmalloc (rsize = 128);
	result[0] = '(';
	rlen = 1;

	for (ae = element_forw(a->head); ae != a->head; ae = element_forw(ae)) {
		is = inttostr (element_index(ae), indstr, sizeof(indstr));
		valstr = element_value (ae) ? sh_double_quote (element_value(ae))
					    : (char *)NULL;
		elen = STRLEN (is) + 8 + STRLEN (valstr);
		RESIZE_MALLOCED_BUFFER (result, rlen, (elen + 1), rsize, rsize);

		result[rlen++] = '[';
		strcpy (result + rlen, is);
		rlen += STRLEN (is);
		result[rlen++] = ']';
		result[rlen++] = '=';
		if (valstr) {
			strcpy (result + rlen, valstr);
			rlen += STRLEN (valstr);
		}

		if (element_forw(ae) != a->head)
		  result[rlen++] = ' ';

		FREE (valstr);
	}
	RESIZE_MALLOCED_BUFFER (result, rlen, 1, rsize, 8);
	result[rlen++] = ')';
	result[rlen] = '\0';
	if (quoted) {
		/* This is not as efficient as it could be... */
		valstr = sh_single_quote (result);
		free (result);
		result = valstr;
	}
	return(result);
}

char *
array_to_string (a, sep, quoted)
ARRAY	*a;
char	*sep;
int	quoted;
{
	if (a == 0)
		return((char *)NULL);
	if (array_empty(a))
		return(savestring(""));
	return (array_to_string_internal (element_forw(a->head), a->head, sep, quoted));
}

#if defined (INCLUDE_UNUSED) || defined (TEST_ARRAY)
/*
 * Return an array consisting of elements in S, separated by SEP
 */
ARRAY *
array_from_string(s, sep)
char	*s, *sep;
{
	ARRAY	*a;
	WORD_LIST *w;

	if (s == 0)
		return((ARRAY *)NULL);
	w = list_string (s, sep, 0);
	if (w == 0)
		return((ARRAY *)NULL);
	a = array_from_word_list (w);
	return (a);
}
#endif

#if defined (TEST_ARRAY)
/*
 * To make a running version, compile -DTEST_ARRAY and link with:
 * 	xmalloc.o syntax.o lib/malloc/libmalloc.a lib/sh/libsh.a
 */
int interrupt_immediately = 0;

int
signal_is_trapped(s)
int	s;
{
	return 0;
}

void
fatal_error(const char *s, ...)
{
	fprintf(stderr, "array_test: fatal memory error\n");
	abort();
}

void
programming_error(const char *s, ...)
{
	fprintf(stderr, "array_test: fatal programming error\n");
	abort();
}

WORD_DESC *
make_bare_word (s)
const char	*s;
{
	WORD_DESC *w;

	w = (WORD_DESC *)xmalloc(sizeof(WORD_DESC));
	w->word = s ? savestring(s) : savestring ("");
	w->flags = 0;
	return w;
}

WORD_LIST *
make_word_list(x, l)
WORD_DESC	*x;
WORD_LIST	*l;
{
	WORD_LIST *w;

	w = (WORD_LIST *)xmalloc(sizeof(WORD_LIST));
	w->word = x;
	w->next = l;
	return w;
}

WORD_LIST *
list_string(s, t, i)
char	*s, *t;
int	i;
{
	char	*r, *a;
	WORD_LIST	*wl;

	if (s == 0)
		return (WORD_LIST *)NULL;
	r = savestring(s);
	wl = (WORD_LIST *)NULL;
	a = strtok(r, t);
	while (a) {
		wl = make_word_list (make_bare_word(a), wl);
		a = strtok((char *)NULL, t);
	}
	return (REVERSE_LIST (wl, WORD_LIST *));
}

GENERIC_LIST *
list_reverse (list)
GENERIC_LIST	*list;
{
	register GENERIC_LIST *next, *prev;

	for (prev = 0; list; ) {
		next = list->next;
		list->next = prev;
		prev = list;
		list = next;
	}
	return prev;
}

char *
pat_subst(s, t, u, i)
char	*s, *t, *u;
int	i;
{
	return ((char *)NULL);
}

char *
quote_string(s)
char	*s;
{
	return savestring(s);
}

print_element(ae)
ARRAY_ELEMENT	*ae;
{
	char	lbuf[INT_STRLEN_BOUND (intmax_t) + 1];

	printf("array[%s] = %s\n",
		inttostr (element_index(ae), lbuf, sizeof (lbuf)),
		element_value(ae));
}

print_array(a)
ARRAY	*a;
{
	printf("\n");
	array_walk(a, print_element, (void *)NULL);
}

main()
{
	ARRAY	*a, *new_a, *copy_of_a;
	ARRAY_ELEMENT	*ae, *aew;
	char	*s;

	a = array_create();
	array_insert(a, 1, "one");
	array_insert(a, 7, "seven");
	array_insert(a, 4, "four");
	array_insert(a, 1029, "one thousand twenty-nine");
	array_insert(a, 12, "twelve");
	array_insert(a, 42, "forty-two");
	print_array(a);
	s = array_to_string (a, " ", 0);
	printf("s = %s\n", s);
	copy_of_a = array_from_string(s, " ");
	printf("copy_of_a:");
	print_array(copy_of_a);
	array_dispose(copy_of_a);
	printf("\n");
	free(s);
	ae = array_remove(a, 4);
	array_dispose_element(ae);
	ae = array_remove(a, 1029);
	array_dispose_element(ae);
	array_insert(a, 16, "sixteen");
	print_array(a);
	s = array_to_string (a, " ", 0);
	printf("s = %s\n", s);
	copy_of_a = array_from_string(s, " ");
	printf("copy_of_a:");
	print_array(copy_of_a);
	array_dispose(copy_of_a);
	printf("\n");
	free(s);
	array_insert(a, 2, "two");
	array_insert(a, 1029, "new one thousand twenty-nine");
	array_insert(a, 0, "zero");
	array_insert(a, 134, "");
	print_array(a);
	s = array_to_string (a, ":", 0);
	printf("s = %s\n", s);
	copy_of_a = array_from_string(s, ":");
	printf("copy_of_a:");
	print_array(copy_of_a);
	array_dispose(copy_of_a);
	printf("\n");
	free(s);
	new_a = array_copy(a);
	print_array(new_a);
	s = array_to_string (new_a, ":", 0);
	printf("s = %s\n", s);
	copy_of_a = array_from_string(s, ":");
	free(s);
	printf("copy_of_a:");
	print_array(copy_of_a);
	array_shift(copy_of_a, 2, AS_DISPOSE);
	printf("copy_of_a shifted by two:");
	print_array(copy_of_a);
	ae = array_shift(copy_of_a, 2, 0);
	printf("copy_of_a shifted by two:");
	print_array(copy_of_a);
	for ( ; ae; ) {
		aew = element_forw(ae);
		array_dispose_element(ae);
		ae = aew;
	}
	array_rshift(copy_of_a, 1, (char *)0);
	printf("copy_of_a rshift by 1:");
	print_array(copy_of_a);
	array_rshift(copy_of_a, 2, "new element zero");
	printf("copy_of_a rshift again by 2 with new element zero:");
	print_array(copy_of_a);
	s = array_to_assign(copy_of_a, 0);
	printf("copy_of_a=%s\n", s);
	free(s);
	ae = array_shift(copy_of_a, array_num_elements(copy_of_a), 0);
	for ( ; ae; ) {
		aew = element_forw(ae);
		array_dispose_element(ae);
		ae = aew;
	}
	array_dispose(copy_of_a);
	printf("\n");
	array_dispose(a);
	array_dispose(new_a);
}

#endif /* TEST_ARRAY */
#endif /* ARRAY_VARS */
