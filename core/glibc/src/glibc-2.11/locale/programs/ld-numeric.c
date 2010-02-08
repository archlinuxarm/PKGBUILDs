/* Copyright (C) 1995-2002,2005,2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.org>, 1995.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation; version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <langinfo.h>
#include <string.h>
#include <sys/uio.h>

#include <assert.h>

#include "localedef.h"
#include "linereader.h"
#include "localeinfo.h"
#include "locfile.h"


/* The real definition of the struct for the LC_NUMERIC locale.  */
struct locale_numeric_t
{
  const char *decimal_point;
  const char *thousands_sep;
  char *grouping;
  size_t grouping_len;
  uint32_t decimal_point_wc;
  uint32_t thousands_sep_wc;
};


static void
numeric_startup (struct linereader *lr, struct localedef_t *locale,
		 int ignore_content)
{
  if (!ignore_content)
    {
      locale->categories[LC_NUMERIC].numeric =
	(struct locale_numeric_t *) xcalloc (1,
					     sizeof (struct locale_numeric_t));
    }

  if (lr != NULL)
    {
      lr->translate_strings = 1;
      lr->return_widestr = 0;
    }
}


void
numeric_finish (struct localedef_t *locale, const struct charmap_t *charmap)
{
  struct locale_numeric_t *numeric = locale->categories[LC_NUMERIC].numeric;
  int nothing = 0;

  /* Now resolve copying and also handle completely missing definitions.  */
  if (numeric == NULL)
    {
      /* First see whether we were supposed to copy.  If yes, find the
	 actual definition.  */
      if (locale->copy_name[LC_NUMERIC] != NULL)
	{
	  /* Find the copying locale.  This has to happen transitively since
	     the locale we are copying from might also copying another one.  */
	  struct localedef_t *from = locale;

	  do
	    from = find_locale (LC_NUMERIC, from->copy_name[LC_NUMERIC],
				from->repertoire_name, charmap);
	  while (from->categories[LC_NUMERIC].numeric == NULL
		 && from->copy_name[LC_NUMERIC] != NULL);

	  numeric = locale->categories[LC_NUMERIC].numeric
	    = from->categories[LC_NUMERIC].numeric;
	}

      /* If there is still no definition issue an warning and create an
	 empty one.  */
      if (numeric == NULL)
	{
	  if (! be_quiet)
	    WITH_CUR_LOCALE (error (0, 0, _("\
No definition for %s category found"), "LC_NUMERIC"));
	  numeric_startup (NULL, locale, 0);
	  numeric = locale->categories[LC_NUMERIC].numeric;
	  nothing = 1;
	}
    }

  /* The decimal point must not be empty.  This is not said explicitly
     in POSIX but ANSI C (ISO/IEC 9899) says in 4.4.2.1 it has to be
     != "".  */
  if (numeric->decimal_point == NULL)
    {
      if (! be_quiet && ! nothing)
	WITH_CUR_LOCALE (error (0, 0, _("%s: field `%s' not defined"),
				"LC_NUMERIC", "decimal_point"));
      numeric->decimal_point = ".";
    }
  else if (numeric->decimal_point[0] == '\0' && ! be_quiet && ! nothing)
    {
      WITH_CUR_LOCALE (error (0, 0, _("\
%s: value for field `%s' must not be an empty string"),
			      "LC_NUMERIC", "decimal_point"));
    }
  if (numeric->decimal_point_wc == L'\0')
    numeric->decimal_point_wc = L'.';

  if (numeric->grouping_len == 0 && ! be_quiet && ! nothing)
    WITH_CUR_LOCALE (error (0, 0, _("%s: field `%s' not defined"),
			    "LC_NUMERIC", "grouping"));
}


void
numeric_output (struct localedef_t *locale, const struct charmap_t *charmap,
		const char *output_path)
{
  struct locale_numeric_t *numeric = locale->categories[LC_NUMERIC].numeric;
  struct iovec iov[3 + _NL_ITEM_INDEX (_NL_NUM_LC_NUMERIC)];
  struct locale_file data;
  uint32_t idx[_NL_ITEM_INDEX (_NL_NUM_LC_NUMERIC)];
  size_t cnt = 0;

  data.magic = LIMAGIC (LC_NUMERIC);
  data.n = _NL_ITEM_INDEX (_NL_NUM_LC_NUMERIC);
  iov[cnt].iov_base = (void *) &data;
  iov[cnt].iov_len = sizeof (data);
  ++cnt;

  iov[cnt].iov_base = (void *) idx;
  iov[cnt].iov_len = sizeof (idx);
  ++cnt;

  idx[cnt - 2] = iov[0].iov_len + iov[1].iov_len;
  iov[cnt].iov_base = (void *) (numeric->decimal_point ?: "");
  iov[cnt].iov_len = strlen (iov[cnt].iov_base) + 1;
  ++cnt;

  idx[cnt - 2] = idx[cnt - 3] + iov[cnt - 1].iov_len;
  iov[cnt].iov_base = (void *) (numeric->thousands_sep ?: "");
  iov[cnt].iov_len = strlen (iov[cnt].iov_base) + 1;
  ++cnt;

  idx[cnt - 2] = idx[cnt - 3] + iov[cnt - 1].iov_len;
  iov[cnt].iov_base = numeric->grouping;
  iov[cnt].iov_len = numeric->grouping_len;
  ++cnt;

  idx[cnt - 2] = idx[cnt - 3] + iov[cnt - 1].iov_len;

  /* Align following data */
  iov[cnt].iov_base = (void *) "\0\0";
  iov[cnt].iov_len = ((idx[cnt - 2] + 3) & ~3) - idx[cnt - 2];
  idx[cnt - 2] = (idx[cnt - 2] + 3) & ~3;
  ++cnt;

  iov[cnt].iov_base = (void *) &numeric->decimal_point_wc;
  iov[cnt].iov_len = sizeof (uint32_t);
  ++cnt;

  idx[cnt - 3] = idx[cnt - 4] + iov[cnt - 1].iov_len;
  iov[cnt].iov_base = (void *) &numeric->thousands_sep_wc;
  iov[cnt].iov_len = sizeof (uint32_t);
  ++cnt;

  idx[cnt - 3] = idx[cnt - 4] + iov[cnt - 1].iov_len;
  iov[cnt].iov_base = (void *) charmap->code_set_name;
  iov[cnt].iov_len = strlen (iov[cnt].iov_base) + 1;

  assert (cnt + 1 == 3 + _NL_ITEM_INDEX (_NL_NUM_LC_NUMERIC));

  write_locale_data (output_path, LC_NUMERIC, "LC_NUMERIC",
		     3 + _NL_ITEM_INDEX (_NL_NUM_LC_NUMERIC), iov);
}


/* The parser for the LC_NUMERIC section of the locale definition.  */
void
numeric_read (struct linereader *ldfile, struct localedef_t *result,
	      const struct charmap_t *charmap, const char *repertoire_name,
	      int ignore_content)
{
  struct repertoire_t *repertoire = NULL;
  struct locale_numeric_t *numeric;
  struct token *now;
  enum token_t nowtok;

  /* Get the repertoire we have to use.  */
  if (repertoire_name != NULL)
    repertoire = repertoire_read (repertoire_name);

  /* The rest of the line containing `LC_NUMERIC' must be free.  */
  lr_ignore_rest (ldfile, 1);


  do
    {
      now = lr_token (ldfile, charmap, result, NULL, verbose);
      nowtok = now->tok;
    }
  while (nowtok == tok_eol);

  /* If we see `copy' now we are almost done.  */
  if (nowtok == tok_copy)
    {
      handle_copy (ldfile, charmap, repertoire_name, result, tok_lc_numeric,
		   LC_NUMERIC, "LC_NUMERIC", ignore_content);
      return;
    }

  /* Prepare the data structures.  */
  numeric_startup (ldfile, result, ignore_content);
  numeric = result->categories[LC_NUMERIC].numeric;

  while (1)
    {
      /* Of course we don't proceed beyond the end of file.  */
      if (nowtok == tok_eof)
	break;

      /* Ingore empty lines.  */
      if (nowtok == tok_eol)
	{
	  now = lr_token (ldfile, charmap, result, NULL, verbose);
	  nowtok = now->tok;
	  continue;
	}

      switch (nowtok)
	{
#define STR_ELEM(cat) \
	case tok_##cat:							      \
	  /* Ignore the rest of the line if we don't need the input of	      \
	     this line.  */						      \
	  if (ignore_content)						      \
	    {								      \
	      lr_ignore_rest (ldfile, 0);				      \
	      break;							      \
	    }								      \
									      \
	  ldfile->return_widestr = 1;					      \
	  now = lr_token (ldfile, charmap, result, repertoire, verbose);	      \
	  if (now->tok != tok_string)					      \
	    goto err_label;						      \
	  if (numeric->cat != NULL)					      \
	    lr_error (ldfile, _("\
%s: field `%s' declared more than once"), "LC_NUMERIC", #cat);		      \
	  else if (!ignore_content && now->val.str.startmb == NULL)	      \
	    {								      \
	      lr_error (ldfile, _("\
%s: unknown character in field `%s'"), "LC_NUMERIC", #cat);		      \
	      numeric->cat = "";					      \
	      numeric->cat##_wc = L'\0';				      \
	    }								      \
	  else if (now->val.str.startwc != NULL && now->val.str.lenwc > 2)    \
	    {								      \
	      lr_error (ldfile, _("\
%s: value for field `%s' must be a single character"), "LC_NUMERIC", #cat);   \
	    }								      \
	  else if (!ignore_content)					      \
	    {								      \
	      numeric->cat = now->val.str.startmb;			      \
									      \
	      if (now->val.str.startwc != NULL)				      \
		numeric->cat##_wc = *now->val.str.startwc;		      \
	    }								      \
	  ldfile->return_widestr = 0;					      \
	  break

	  STR_ELEM (decimal_point);
	  STR_ELEM (thousands_sep);

	case tok_grouping:
	  /* Ignore the rest of the line if we don't need the input of
	     this line.  */
	  if (ignore_content)
	    {
	      lr_ignore_rest (ldfile, 0);
	      break;
	    }

	  now = lr_token (ldfile, charmap, result, NULL, verbose);
	  if (now->tok != tok_minus1 && now->tok != tok_number)
	    goto err_label;
	  else
	    {
	      size_t act = 0;
	      size_t max = 10;
	      char *grouping = xmalloc (max);

	      do
		{
		  if (act + 1 >= max)
		    {
		      max *= 2;
		      grouping = xrealloc (grouping, max);
		    }

		  if (act > 0 && grouping[act - 1] == '\177')
		    {
		      lr_error (ldfile, _("\
%s: `-1' must be last entry in `%s' field"), "LC_NUMERIC", "grouping");
		      lr_ignore_rest (ldfile, 0);
		      break;
		    }

		  if (now->tok == tok_minus1)
		    grouping[act++] = '\177';
		  else if (now->val.num == 0)
		    {
		      /* A value of 0 disables grouping from here on but
			 we must not store a NUL character since this
			 terminates the string.  Use something different
			 which must not be used otherwise.  */
		      grouping[act++] = '\377';
		    }
		  else if (now->val.num > 126)
		    lr_error (ldfile, _("\
%s: values for field `%s' must be smaller than 127"),
			      "LC_NUMERIC", "grouping");
		  else
		    grouping[act++] = now->val.num;

		  /* Next must be semicolon.  */
		  now = lr_token (ldfile, charmap, result, NULL, verbose);
		  if (now->tok != tok_semicolon)
		    break;

		  now = lr_token (ldfile, charmap, result, NULL, verbose);
		}
	      while (now->tok == tok_minus1 || now->tok == tok_number);

	      if (now->tok != tok_eol)
		goto err_label;

	      grouping[act++] = '\0';

	      numeric->grouping = xrealloc (grouping, act);
	      numeric->grouping_len = act;
	    }
	  break;

	case tok_end:
	  /* Next we assume `LC_NUMERIC'.  */
	  now = lr_token (ldfile, charmap, result, NULL, verbose);
	  if (now->tok == tok_eof)
	    break;
	  if (now->tok == tok_eol)
	    lr_error (ldfile, _("%s: incomplete `END' line"), "LC_NUMERIC");
	  else if (now->tok != tok_lc_numeric)
	    lr_error (ldfile, _("\
%1$s: definition does not end with `END %1$s'"), "LC_NUMERIC");
	  lr_ignore_rest (ldfile, now->tok == tok_lc_numeric);
	  return;

	default:
	err_label:
	  SYNTAX_ERROR (_("%s: syntax error"), "LC_NUMERIC");
	}

      /* Prepare for the next round.  */
      now = lr_token (ldfile, charmap, result, NULL, verbose);
      nowtok = now->tok;
    }

  /* When we come here we reached the end of the file.  */
  lr_error (ldfile, _("%s: premature end of file"), "LC_NUMERIC");
}
