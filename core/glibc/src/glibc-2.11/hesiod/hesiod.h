/*
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * This file is primarily maintained by <tytso@mit.edu> and <ghudson@mit.edu>.
 */

/*
 * $BINDId: hesiod.h,v 1.7 1999/01/08 19:22:45 vixie Exp $
 */

#ifndef _HESIOD_H_INCLUDED
#define _HESIOD_H_INCLUDED

int		hesiod_init (void **context);
void		hesiod_end (void *context);
char *		hesiod_to_bind (void *context, const char *name,
				const char *type);
char **		hesiod_resolve (void *context, const char *name,
				const char *type);
void		hesiod_free_list (void *context, char **list);
struct __res_state * __hesiod_res_get (void *context);
void		__hesiod_res_set (void *context, struct __res_state *,
				  void (*)(void *));

#endif /*_HESIOD_H_INCLUDED*/
