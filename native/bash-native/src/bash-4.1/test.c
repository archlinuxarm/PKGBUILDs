/* test.c - GNU test program (ksb and mjb) */

/* Modified to run with the GNU shell Apr 25, 1988 by bfox. */

/* Copyright (C) 1987-2009 Free Software Foundation, Inc.

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

/* Define PATTERN_MATCHING to get the csh-like =~ and !~ pattern-matching
   binary operators. */
/* #define PATTERN_MATCHING */

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>

#include "bashtypes.h"

#if !defined (HAVE_LIMITS_H)
#  include <sys/param.h>
#endif

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <errno.h>
#if !defined (errno)
extern int errno;
#endif /* !errno */

#if !defined (_POSIX_VERSION) && defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif /* !_POSIX_VERSION */
#include "posixstat.h"
#include "filecntl.h"

#include "bashintl.h"

#include "shell.h"
#include "pathexp.h"
#include "test.h"
#include "builtins/common.h"

#include <glob/strmatch.h>

#if !defined (STRLEN)
#  define STRLEN(s) ((s)[0] ? ((s)[1] ? ((s)[2] ? strlen(s) : 2) : 1) : 0)
#endif

#if !defined (STREQ)
#  define STREQ(a, b) ((a)[0] == (b)[0] && strcmp ((a), (b)) == 0)
#endif /* !STREQ */
#define STRCOLLEQ(a, b) ((a)[0] == (b)[0] && strcoll ((a), (b)) == 0)

#if !defined (R_OK)
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0
#endif /* R_OK */

#define EQ	0
#define NE	1
#define LT	2
#define GT	3
#define LE	4
#define GE	5

#define NT	0
#define OT	1
#define EF	2

/* The following few defines control the truth and false output of each stage.
   TRUE and FALSE are what we use to compute the final output value.
   SHELL_BOOLEAN is the form which returns truth or falseness in shell terms.
   Default is TRUE = 1, FALSE = 0, SHELL_BOOLEAN = (!value). */
#define TRUE 1
#define FALSE 0
#define SHELL_BOOLEAN(value) (!(value))

#define TEST_ERREXIT_STATUS	2

static procenv_t test_exit_buf;
static int test_error_return;
#define test_exit(val) \
	do { test_error_return = val; longjmp (test_exit_buf, 1); } while (0)

extern int sh_stat __P((const char *, struct stat *));

static int pos;		/* The offset of the current argument in ARGV. */
static int argc;	/* The number of arguments present in ARGV. */
static char **argv;	/* The argument list. */
static int noeval;

static void test_syntax_error __P((char *, char *)) __attribute__((__noreturn__));
static void beyond __P((void)) __attribute__((__noreturn__));
static void integer_expected_error __P((char *)) __attribute__((__noreturn__));

static int unary_operator __P((void));
static int binary_operator __P((void));
static int two_arguments __P((void));
static int three_arguments __P((void));
static int posixtest __P((void));

static int expr __P((void));
static int term __P((void));
static int and __P((void));
static int or __P((void));

static int filecomp __P((char *, char *, int));
static int arithcomp __P((char *, char *, int, int));
static int patcomp __P((char *, char *, int));

static void
test_syntax_error (format, arg)
     char *format, *arg;
{
  builtin_error (format, arg);
  test_exit (TEST_ERREXIT_STATUS);
}

/*
 * beyond - call when we're beyond the end of the argument list (an
 *	error condition)
 */
static void
beyond ()
{
  test_syntax_error (_("argument expected"), (char *)NULL);
}

/* Syntax error for when an integer argument was expected, but
   something else was found. */
static void
integer_expected_error (pch)
     char *pch;
{
  test_syntax_error (_("%s: integer expression expected"), pch);
}

/* Increment our position in the argument list.  Check that we're not
   past the end of the argument list.  This check is supressed if the
   argument is FALSE.  Made a macro for efficiency. */
#define advance(f) do { ++pos; if (f && pos >= argc) beyond (); } while (0)
#define unary_advance() do { advance (1); ++pos; } while (0)

/*
 * expr:
 *	or
 */
static int
expr ()
{
  if (pos >= argc)
    beyond ();

  return (FALSE ^ or ());		/* Same with this. */
}

/*
 * or:
 *	and
 *	and '-o' or
 */
static int
or ()
{
  int value, v2;

  value = and ();
  if (pos < argc && argv[pos][0] == '-' && argv[pos][1] == 'o' && !argv[pos][2])
    {
      advance (0);
      v2 = or ();
      return (value || v2);
    }

  return (value);
}

/*
 * and:
 *	term
 *	term '-a' and
 */
static int
and ()
{
  int value, v2;

  value = term ();
  if (pos < argc && argv[pos][0] == '-' && argv[pos][1] == 'a' && !argv[pos][2])
    {
      advance (0);
      v2 = and ();
      return (value && v2);
    }
  return (value);
}

/*
 * term - parse a term and return 1 or 0 depending on whether the term
 *	evaluates to true or false, respectively.
 *
 * term ::=
 *	'-'('a'|'b'|'c'|'d'|'e'|'f'|'g'|'h'|'k'|'p'|'r'|'s'|'u'|'w'|'x') filename
 *	'-'('G'|'L'|'O'|'S'|'N') filename
 * 	'-t' [int]
 *	'-'('z'|'n') string
 *	'-o' option
 *	string
 *	string ('!='|'='|'==') string
 *	<int> '-'(eq|ne|le|lt|ge|gt) <int>
 *	file '-'(nt|ot|ef) file
 *	'(' <expr> ')'
 * int ::=
 *	positive and negative integers
 */
static int
term ()
{
  int value;

  if (pos >= argc)
    beyond ();

  /* Deal with leading `not's. */
  if (argv[pos][0] == '!' && argv[pos][1] == '\0')
    {
      value = 0;
      while (pos < argc && argv[pos][0] == '!' && argv[pos][1] == '\0')
	{
	  advance (1);
	  value = 1 - value;
	}

      return (value ? !term() : term());
    }

  /* A paren-bracketed argument. */
  if (argv[pos][0] == '(' && argv[pos][1] == '\0') /* ) */
    {
      advance (1);
      value = expr ();
      if (argv[pos] == 0) /* ( */
	test_syntax_error (_("`)' expected"), (char *)NULL);
      else if (argv[pos][0] != ')' || argv[pos][1]) /* ( */
	test_syntax_error (_("`)' expected, found %s"), argv[pos]);
      advance (0);
      return (value);
    }

  /* are there enough arguments left that this could be dyadic? */
  if ((pos + 3 <= argc) && test_binop (argv[pos + 1]))
    value = binary_operator ();

  /* Might be a switch type argument */
  else if (argv[pos][0] == '-' && argv[pos][2] == '\0')
    {
      if (test_unop (argv[pos]))
	value = unary_operator ();
      else
	test_syntax_error (_("%s: unary operator expected"), argv[pos]);
    }
  else
    {
      value = argv[pos][0] != '\0';
      advance (0);
    }

  return (value);
}

static int
filecomp (s, t, op)
     char *s, *t;
     int op;
{
  struct stat st1, st2;
  int r1, r2;

  if ((r1 = sh_stat (s, &st1)) < 0)
    {
      if (op == EF)
	return (FALSE);
    }
  if ((r2 = sh_stat (t, &st2)) < 0)
    {
      if (op == EF)
	return (FALSE);
    }
  
  switch (op)
    {
    case OT: return (r1 < r2 || (r2 == 0 && st1.st_mtime < st2.st_mtime));
    case NT: return (r1 > r2 || (r1 == 0 && st1.st_mtime > st2.st_mtime));
    case EF: return (same_file (s, t, &st1, &st2));
    }
  return (FALSE);
}

static int
arithcomp (s, t, op, flags)
     char *s, *t;
     int op, flags;
{
  intmax_t l, r;
  int expok;

  if (flags & TEST_ARITHEXP)
    {
      l = evalexp (s, &expok);
      if (expok == 0)
	return (FALSE);		/* should probably longjmp here */
      r = evalexp (t, &expok);
      if (expok == 0)
	return (FALSE);		/* ditto */
    }
  else
    {
      if (legal_number (s, &l) == 0)
	integer_expected_error (s);
      if (legal_number (t, &r) == 0)
	integer_expected_error (t);
    }

  switch (op)
    {
    case EQ: return (l == r);
    case NE: return (l != r);
    case LT: return (l < r);
    case GT: return (l > r);
    case LE: return (l <= r);
    case GE: return (l >= r);
    }

  return (FALSE);
}

static int
patcomp (string, pat, op)
     char *string, *pat;
     int op;
{
  int m;

  m = strmatch (pat, string, FNMATCH_EXTFLAG|FNMATCH_IGNCASE);
  return ((op == EQ) ? (m == 0) : (m != 0));
}

int
binary_test (op, arg1, arg2, flags)
     char *op, *arg1, *arg2;
     int flags;
{
  int patmatch;

  patmatch = (flags & TEST_PATMATCH);

  if (op[0] == '=' && (op[1] == '\0' || (op[1] == '=' && op[2] == '\0')))
    return (patmatch ? patcomp (arg1, arg2, EQ) : STREQ (arg1, arg2));
  else if ((op[0] == '>' || op[0] == '<') && op[1] == '\0')
    {
      if (shell_compatibility_level > 40 && flags & TEST_LOCALE)
	return ((op[0] == '>') ? (strcoll (arg1, arg2) > 0) : (strcoll (arg1, arg2) < 0));
      else
	return ((op[0] == '>') ? (strcmp (arg1, arg2) > 0) : (strcmp (arg1, arg2) < 0));
    }
  else if (op[0] == '!' && op[1] == '=' && op[2] == '\0')
    return (patmatch ? patcomp (arg1, arg2, NE) : (STREQ (arg1, arg2) == 0));
    

  else if (op[2] == 't')
    {
      switch (op[1])
	{
	case 'n': return (filecomp (arg1, arg2, NT));		/* -nt */
	case 'o': return (filecomp (arg1, arg2, OT));		/* -ot */
	case 'l': return (arithcomp (arg1, arg2, LT, flags));	/* -lt */
	case 'g': return (arithcomp (arg1, arg2, GT, flags));	/* -gt */
	}
    }
  else if (op[1] == 'e')
    {
      switch (op[2])
	{
	case 'f': return (filecomp (arg1, arg2, EF));		/* -ef */
	case 'q': return (arithcomp (arg1, arg2, EQ, flags));	/* -eq */
	}
    }
  else if (op[2] == 'e')
    {
      switch (op[1])
	{
	case 'n': return (arithcomp (arg1, arg2, NE, flags));	/* -ne */
	case 'g': return (arithcomp (arg1, arg2, GE, flags));	/* -ge */
	case 'l': return (arithcomp (arg1, arg2, LE, flags));	/* -le */
	}
    }

  return (FALSE);	/* should never get here */
}


static int
binary_operator ()
{
  int value;
  char *w;

  w = argv[pos + 1];
  if ((w[0] == '=' && (w[1] == '\0' || (w[1] == '=' && w[2] == '\0'))) || /* =, == */
      ((w[0] == '>' || w[0] == '<') && w[1] == '\0') ||		/* <, > */
      (w[0] == '!' && w[1] == '=' && w[2] == '\0'))		/* != */
    {
      value = binary_test (w, argv[pos], argv[pos + 2], 0);
      pos += 3;
      return (value);
    }

#if defined (PATTERN_MATCHING)
  if ((w[0] == '=' || w[0] == '!') && w[1] == '~' && w[2] == '\0')
    {
      value = patcomp (argv[pos], argv[pos + 2], w[0] == '=' ? EQ : NE);
      pos += 3;
      return (value);
    }
#endif

  if ((w[0] != '-' || w[3] != '\0') || test_binop (w) == 0)
    {
      test_syntax_error (_("%s: binary operator expected"), w);
      /* NOTREACHED */
      return (FALSE);
    }

  value = binary_test (w, argv[pos], argv[pos + 2], 0);
  pos += 3;
  return value;
}

static int
unary_operator ()
{
  char *op;
  intmax_t r;

  op = argv[pos];
  if (test_unop (op) == 0)
    return (FALSE);

  /* the only tricky case is `-t', which may or may not take an argument. */
  if (op[1] == 't')
    {
      advance (0);
      if (pos < argc)
	{
	  if (legal_number (argv[pos], &r))
	    {
	      advance (0);
	      return (unary_test (op, argv[pos - 1]));
	    }
	  else
	    return (FALSE);
	}
      else
	return (unary_test (op, "1"));
    }

  /* All of the unary operators take an argument, so we first call
     unary_advance (), which checks to make sure that there is an
     argument, and then advances pos right past it.  This means that
     pos - 1 is the location of the argument. */
  unary_advance ();
  return (unary_test (op, argv[pos - 1]));
}

int
unary_test (op, arg)
     char *op, *arg;
{
  intmax_t r;
  struct stat stat_buf;
     
  switch (op[1])
    {
    case 'a':			/* file exists in the file system? */
    case 'e':
      return (sh_stat (arg, &stat_buf) == 0);

    case 'r':			/* file is readable? */
      return (sh_eaccess (arg, R_OK) == 0);

    case 'w':			/* File is writeable? */
      return (sh_eaccess (arg, W_OK) == 0);

    case 'x':			/* File is executable? */
      return (sh_eaccess (arg, X_OK) == 0);

    case 'O':			/* File is owned by you? */
      return (sh_stat (arg, &stat_buf) == 0 &&
	      (uid_t) current_user.euid == (uid_t) stat_buf.st_uid);

    case 'G':			/* File is owned by your group? */
      return (sh_stat (arg, &stat_buf) == 0 &&
	      (gid_t) current_user.egid == (gid_t) stat_buf.st_gid);

    case 'N':
      return (sh_stat (arg, &stat_buf) == 0 &&
	      stat_buf.st_atime <= stat_buf.st_mtime);

    case 'f':			/* File is a file? */
      if (sh_stat (arg, &stat_buf) < 0)
	return (FALSE);

      /* -f is true if the given file exists and is a regular file. */
#if defined (S_IFMT)
      return (S_ISREG (stat_buf.st_mode) || (stat_buf.st_mode & S_IFMT) == 0);
#else
      return (S_ISREG (stat_buf.st_mode));
#endif /* !S_IFMT */

    case 'd':			/* File is a directory? */
      return (sh_stat (arg, &stat_buf) == 0 && (S_ISDIR (stat_buf.st_mode)));

    case 's':			/* File has something in it? */
      return (sh_stat (arg, &stat_buf) == 0 && stat_buf.st_size > (off_t) 0);

    case 'S':			/* File is a socket? */
#if !defined (S_ISSOCK)
      return (FALSE);
#else
      return (sh_stat (arg, &stat_buf) == 0 && S_ISSOCK (stat_buf.st_mode));
#endif /* S_ISSOCK */

    case 'c':			/* File is character special? */
      return (sh_stat (arg, &stat_buf) == 0 && S_ISCHR (stat_buf.st_mode));

    case 'b':			/* File is block special? */
      return (sh_stat (arg, &stat_buf) == 0 && S_ISBLK (stat_buf.st_mode));

    case 'p':			/* File is a named pipe? */
#ifndef S_ISFIFO
      return (FALSE);
#else
      return (sh_stat (arg, &stat_buf) == 0 && S_ISFIFO (stat_buf.st_mode));
#endif /* S_ISFIFO */

    case 'L':			/* Same as -h  */
    case 'h':			/* File is a symbolic link? */
#if !defined (S_ISLNK) || !defined (HAVE_LSTAT)
      return (FALSE);
#else
      return ((arg[0] != '\0') &&
	      (lstat (arg, &stat_buf) == 0) && S_ISLNK (stat_buf.st_mode));
#endif /* S_IFLNK && HAVE_LSTAT */

    case 'u':			/* File is setuid? */
      return (sh_stat (arg, &stat_buf) == 0 && (stat_buf.st_mode & S_ISUID) != 0);

    case 'g':			/* File is setgid? */
      return (sh_stat (arg, &stat_buf) == 0 && (stat_buf.st_mode & S_ISGID) != 0);

    case 'k':			/* File has sticky bit set? */
#if !defined (S_ISVTX)
      /* This is not Posix, and is not defined on some Posix systems. */
      return (FALSE);
#else
      return (sh_stat (arg, &stat_buf) == 0 && (stat_buf.st_mode & S_ISVTX) != 0);
#endif

    case 't':	/* File fd is a terminal? */
      if (legal_number (arg, &r) == 0)
	return (FALSE);
      return ((r == (int)r) && isatty ((int)r));

    case 'n':			/* True if arg has some length. */
      return (arg[0] != '\0');

    case 'z':			/* True if arg has no length. */
      return (arg[0] == '\0');

    case 'o':			/* True if option `arg' is set. */
      return (minus_o_option_value (arg) == 1);
    }

  /* We can't actually get here, but this shuts up gcc. */
  return (FALSE);
}

/* Return TRUE if OP is one of the test command's binary operators. */
int
test_binop (op)
     char *op;
{
  if (op[0] == '=' && op[1] == '\0')
    return (1);		/* '=' */
  else if ((op[0] == '<' || op[0] == '>') && op[1] == '\0')  /* string <, > */
    return (1);
  else if ((op[0] == '=' || op[0] == '!') && op[1] == '=' && op[2] == '\0')
    return (1);		/* `==' and `!=' */
#if defined (PATTERN_MATCHING)
  else if (op[2] == '\0' && op[1] == '~' && (op[0] == '=' || op[0] == '!'))
    return (1);
#endif
  else if (op[0] != '-' || op[2] == '\0' || op[3] != '\0')
    return (0);
  else
    {
      if (op[2] == 't')
	switch (op[1])
	  {
	  case 'n':		/* -nt */
	  case 'o':		/* -ot */
	  case 'l':		/* -lt */
	  case 'g':		/* -gt */
	    return (1);
	  default:
	    return (0);
	  }
      else if (op[1] == 'e')
	switch (op[2])
	  {
	  case 'q':		/* -eq */
	  case 'f':		/* -ef */
	    return (1);
	  default:
	    return (0);
	  }
      else if (op[2] == 'e')
	switch (op[1])
	  {
	  case 'n':		/* -ne */
	  case 'g':		/* -ge */
	  case 'l':		/* -le */
	    return (1);
	  default:
	    return (0);
	  }
      else
	return (0);
    }
}

/* Return non-zero if OP is one of the test command's unary operators. */
int
test_unop (op)
     char *op;
{
  if (op[0] != '-' || op[2] != 0)
    return (0);

  switch (op[1])
    {
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'k': case 'n':
    case 'o': case 'p': case 'r': case 's': case 't':
    case 'u': case 'w': case 'x': case 'z':
    case 'G': case 'L': case 'O': case 'S': case 'N':
      return (1);
    }

  return (0);
}

static int
two_arguments ()
{
  if (argv[pos][0] == '!' && argv[pos][1] == '\0')
    return (argv[pos + 1][0] == '\0');
  else if (argv[pos][0] == '-' && argv[pos][2] == '\0')
    {
      if (test_unop (argv[pos]))
	return (unary_operator ());
      else
	test_syntax_error (_("%s: unary operator expected"), argv[pos]);
    }
  else
    test_syntax_error (_("%s: unary operator expected"), argv[pos]);

  return (0);
}

#define ANDOR(s)  (s[0] == '-' && !s[2] && (s[1] == 'a' || s[1] == 'o'))

/* This could be augmented to handle `-t' as equivalent to `-t 1', but
   POSIX requires that `-t' be given an argument. */
#define ONE_ARG_TEST(s)		((s)[0] != '\0')

static int
three_arguments ()
{
  int value;

  if (test_binop (argv[pos+1]))
    {
      value = binary_operator ();
      pos = argc;
    }
  else if (ANDOR (argv[pos+1]))
    {
      if (argv[pos+1][1] == 'a')
	value = ONE_ARG_TEST(argv[pos]) && ONE_ARG_TEST(argv[pos+2]);
      else
	value = ONE_ARG_TEST(argv[pos]) || ONE_ARG_TEST(argv[pos+2]);
      pos = argc;
    }
  else if (argv[pos][0] == '!' && argv[pos][1] == '\0')
    {
      advance (1);
      value = !two_arguments ();
    }
  else if (argv[pos][0] == '(' && argv[pos+2][0] == ')')
    {
      value = ONE_ARG_TEST(argv[pos+1]);
      pos = argc;
    }
  else
    test_syntax_error (_("%s: binary operator expected"), argv[pos+1]);

  return (value);
}

/* This is an implementation of a Posix.2 proposal by David Korn. */
static int
posixtest ()
{
  int value;

  switch (argc - 1)	/* one extra passed in */
    {
      case 0:
	value = FALSE;
	pos = argc;
	break;

      case 1:
	value = ONE_ARG_TEST(argv[1]);
	pos = argc;
	break;

      case 2:
	value = two_arguments ();
	pos = argc;
	break;

      case 3:
	value = three_arguments ();
	break;

      case 4:
	if (argv[pos][0] == '!' && argv[pos][1] == '\0')
	  {
	    advance (1);
	    value = !three_arguments ();
	    break;
	  }
	/* FALLTHROUGH */
      default:
	value = expr ();
    }

  return (value);
}

/*
 * [:
 *	'[' expr ']'
 * test:
 *	test expr
 */
int
test_command (margc, margv)
     int margc;
     char **margv;
{
  int value;
  int code;

  USE_VAR(margc);

  code = setjmp (test_exit_buf);

  if (code)
    return (test_error_return);

  argv = margv;

  if (margv[0] && margv[0][0] == '[' && margv[0][1] == '\0')
    {
      --margc;

      if (margv[margc] && (margv[margc][0] != ']' || margv[margc][1]))
	test_syntax_error (_("missing `]'"), (char *)NULL);

      if (margc < 2)
	test_exit (SHELL_BOOLEAN (FALSE));
    }

  argc = margc;
  pos = 1;

  if (pos >= argc)
    test_exit (SHELL_BOOLEAN (FALSE));

  noeval = 0;
  value = posixtest ();

  if (pos != argc)
    test_syntax_error (_("too many arguments"), (char *)NULL);

  test_exit (SHELL_BOOLEAN (value));
}
