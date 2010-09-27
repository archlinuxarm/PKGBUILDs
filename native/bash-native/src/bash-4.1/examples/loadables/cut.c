/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam S. Moskowitz of Menlo Consulting and Marciano Pitargue.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static const char sccsid[] = "@(#)cut.c	8.3 (Berkeley) 5/4/95";
#endif /* not lint */

#include <config.h>

#include <ctype.h>
#include <stdio.h>
#include <errno.h>

#include "bashansi.h"

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "builtins.h"
#include "shell.h"
#include "bashgetopt.h"
#include "common.h"

#if !defined (errno)
extern int	errno;
#endif

#if !defined (_POSIX2_LINE_MAX)
#  define _POSIX2_LINE_MAX 2048
#endif

static int	cflag;
static char	dchar;
static int	dflag;
static int	fflag;
static int	sflag;

static int autostart, autostop, maxval;
static char positions[_POSIX2_LINE_MAX + 1];

static int	c_cut __P((FILE *, char *));
static int	f_cut __P((FILE *, char *));
static int	get_list __P((char *));
static char	*_cut_strsep __P((char **, const char *));

int
cut_builtin(list)
	WORD_LIST *list;
{
	FILE *fp;
	int (*fcn) __P((FILE *, char *)) = NULL;
	int ch;

	fcn = NULL;
	dchar = '\t';			/* default delimiter is \t */

	/* Since we don't support multi-byte characters, the -c and -b 
	   options are equivalent, and the -n option is meaningless. */
	reset_internal_getopt ();
	while ((ch = internal_getopt (list, "b:c:d:f:sn")) != -1)
		switch(ch) {
		case 'b':
		case 'c':
			fcn = c_cut;
			if (get_list(list_optarg) < 0)
				return (EXECUTION_FAILURE);
			cflag = 1;
			break;
		case 'd':
			dchar = *list_optarg;
			dflag = 1;
			break;
		case 'f':
			fcn = f_cut;
			if (get_list(list_optarg) < 0)
				return (EXECUTION_FAILURE);
			fflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'n':
			break;
		case '?':
		default:
			builtin_usage();
			return (EX_USAGE);
		}

	list = loptend;

	if (fflag) {
		if (cflag) {
			builtin_usage();
			return (EX_USAGE);
		}
	} else if (!cflag || dflag || sflag) {
		builtin_usage();
		return (EX_USAGE);
	}

	if (list) {
		while (list) {
			fp = fopen(list->word->word, "r");
			if (fp == 0) {
				builtin_error("%s", list->word->word);
				return (EXECUTION_FAILURE);
			}
			ch = (*fcn)(fp, list->word->word);
			(void)fclose(fp);
			if (ch < 0)
				return (EXECUTION_FAILURE);
			list = list->next;
		}
	} else {
		ch = (*fcn)(stdin, "stdin");
		if (ch < 0)
			return (EXECUTION_FAILURE);
	}

	return (EXECUTION_SUCCESS);
}

static int
get_list(list)
	char *list;
{
	int setautostart, start, stop;
	char *pos;
	char *p;

	/*
	 * set a byte in the positions array to indicate if a field or
	 * column is to be selected; use +1, it's 1-based, not 0-based.
	 * This parser is less restrictive than the Draft 9 POSIX spec.
	 * POSIX doesn't allow lists that aren't in increasing order or
	 * overlapping lists.  We also handle "-3-5" although there's no
	 * real reason too.
	 */
	for (; (p = _cut_strsep(&list, ", \t")) != NULL;) {
		setautostart = start = stop = 0;
		if (*p == '-') {
			++p;
			setautostart = 1;
		}
		if (isdigit((unsigned char)*p)) {
			start = stop = strtol(p, &p, 10);
			if (setautostart && start > autostart)
				autostart = start;
		}
		if (*p == '-') {
			if (isdigit((unsigned char)p[1]))
				stop = strtol(p + 1, &p, 10);
			if (*p == '-') {
				++p;
				if (!autostop || autostop > stop)
					autostop = stop;
			}
		}
		if (*p) {
			builtin_error("[-cf] list: illegal list value");
			return -1;
		}
		if (!stop || !start) {
			builtin_error("[-cf] list: values may not include zero");
			return -1;
		}
		if (stop > _POSIX2_LINE_MAX) {
			builtin_error("[-cf] list: %d too large (max %d)",
				       stop, _POSIX2_LINE_MAX);
			return -1;
		}
		if (maxval < stop)
			maxval = stop;
		for (pos = positions + start; start++ <= stop; *pos++ = 1);
	}

	/* overlapping ranges */
	if (autostop && maxval > autostop)
		maxval = autostop;

	/* set autostart */
	if (autostart)
		memset(positions + 1, '1', autostart);

	return 0;
}

/* ARGSUSED */
static int
c_cut(fp, fname)
	FILE *fp;
	char *fname;
{
	int ch, col;
	char *pos;

	ch = 0;
	for (;;) {
		pos = positions + 1;
		for (col = maxval; col; --col) {
			if ((ch = getc(fp)) == EOF)
				return;
			if (ch == '\n')
				break;
			if (*pos++)
				(void)putchar(ch);
		}
		if (ch != '\n') {
			if (autostop)
				while ((ch = getc(fp)) != EOF && ch != '\n')
					(void)putchar(ch);
			else
				while ((ch = getc(fp)) != EOF && ch != '\n');
		}
		(void)putchar('\n');
	}
	return (0);
}

static int
f_cut(fp, fname)
	FILE *fp;
	char *fname;
{
	int ch, field, isdelim;
	char *pos, *p, sep;
	int output;
	char lbuf[_POSIX2_LINE_MAX + 1];

	for (sep = dchar; fgets(lbuf, sizeof(lbuf), fp);) {
		output = 0;
		for (isdelim = 0, p = lbuf;; ++p) {
			if (!(ch = *p)) {
				builtin_error("%s: line too long.", fname);
				return -1;
			}
			/* this should work if newline is delimiter */
			if (ch == sep)
				isdelim = 1;
			if (ch == '\n') {
				if (!isdelim && !sflag)
					(void)printf("%s", lbuf);
				break;
			}
		}
		if (!isdelim)
			continue;

		pos = positions + 1;
		for (field = maxval, p = lbuf; field; --field, ++pos) {
			if (*pos) {
				if (output++)
					(void)putchar(sep);
				while ((ch = *p++) != '\n' && ch != sep)
					(void)putchar(ch);
			} else {
				while ((ch = *p++) != '\n' && ch != sep)
					continue;
			}
			if (ch == '\n')
				break;
		}
		if (ch != '\n') {
			if (autostop) {
				if (output)
					(void)putchar(sep);
				for (; (ch = *p) != '\n'; ++p)
					(void)putchar(ch);
			} else
				for (; (ch = *p) != '\n'; ++p);
		}
		(void)putchar('\n');
	}
	return (0);
}

/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
static char *
_cut_strsep(stringp, delim)
	register char **stringp;
	register const char *delim;
{
	register char *s;
	register const char *spanp;
	register int c, sc;
	char *tok;

	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

static char *cut_doc[] = {
	"Select portions of lines.",
	"",
	"Select portions of each line (as specified by LIST) from each FILE",
	"(by default, the standard input), and write them to the standard output.",
	"Items specified by LIST are either column positions or fields delimited",
	"by a special character.  Column numbering starts at 1.",
	(char *)0
};

struct builtin cut_struct = {
	"cut",
	cut_builtin,
	BUILTIN_ENABLED,
	cut_doc,
	"cut -b list [-n] [file ...] OR cut -c list [file ...] OR cut -f list [-s] [-d delim] [file ...]",
	0
};
