/* Copyright (C) 1991-2006 Free Software Foundation, Inc.

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

int FCT __P((CHAR *, CHAR *, int));

static int GMATCH __P((CHAR *, CHAR *, CHAR *, CHAR *, int));
static CHAR *PARSE_COLLSYM __P((CHAR *, INT *));
static CHAR *BRACKMATCH __P((CHAR *, U_CHAR, int));
static int EXTMATCH __P((INT, CHAR *, CHAR *, CHAR *, CHAR *, int));
static CHAR *PATSCAN __P((CHAR *, CHAR *, INT));

int
FCT (pattern, string, flags)
     CHAR *pattern;
     CHAR *string;
     int flags;
{
  CHAR *se, *pe;

  if (string == 0 || pattern == 0)
    return FNM_NOMATCH;

  se = string + STRLEN ((XCHAR *)string);
  pe = pattern + STRLEN ((XCHAR *)pattern);

  return (GMATCH (string, se, pattern, pe, flags));
}

/* Match STRING against the filename pattern PATTERN, returning zero if
   it matches, FNM_NOMATCH if not.  */
static int
GMATCH (string, se, pattern, pe, flags)
     CHAR *string, *se;
     CHAR *pattern, *pe;
     int flags;
{
  CHAR *p, *n;		/* pattern, string */
  INT c;		/* current pattern character - XXX U_CHAR? */
  INT sc;		/* current string character - XXX U_CHAR? */

  p = pattern;
  n = string;

  if (string == 0 || pattern == 0)
    return FNM_NOMATCH;

#if DEBUG_MATCHING
fprintf(stderr, "gmatch: string = %s; se = %s\n", string, se);
fprintf(stderr, "gmatch: pattern = %s; pe = %s\n", pattern, pe);
#endif

  while (p < pe)
    {
      c = *p++;
      c = FOLD (c);

      sc = n < se ? *n : '\0';

#ifdef EXTENDED_GLOB
      /* EXTMATCH () will handle recursively calling GMATCH, so we can
	 just return what EXTMATCH() returns. */
      if ((flags & FNM_EXTMATCH) && *p == L('(') &&
	  (c == L('+') || c == L('*') || c == L('?') || c == L('@') || c == L('!'))) /* ) */
	{
	  int lflags;
	  /* If we're not matching the start of the string, we're not
	     concerned about the special cases for matching `.' */
	  lflags = (n == string) ? flags : (flags & ~FNM_PERIOD);
	  return (EXTMATCH (c, n, se, p, pe, lflags));
	}
#endif /* EXTENDED_GLOB */

      switch (c)
	{
	case L('?'):		/* Match single character */
	  if (sc == '\0')
	    return FNM_NOMATCH;
	  else if ((flags & FNM_PATHNAME) && sc == L('/'))
	    /* If we are matching a pathname, `?' can never match a `/'. */
	    return FNM_NOMATCH;
	  else if ((flags & FNM_PERIOD) && sc == L('.') &&
		   (n == string || ((flags & FNM_PATHNAME) && n[-1] == L('/'))))
	    /* `?' cannot match a `.' if it is the first character of the
	       string or if it is the first character following a slash and
	       we are matching a pathname. */
	    return FNM_NOMATCH;
	  break;

	case L('\\'):		/* backslash escape removes special meaning */
	  if (p == pe)
	    return FNM_NOMATCH;

	  if ((flags & FNM_NOESCAPE) == 0)
	    {
	      c = *p++;
	      /* A trailing `\' cannot match. */
	      if (p > pe)
		return FNM_NOMATCH;
	      c = FOLD (c);
	    }
	  if (FOLD (sc) != (U_CHAR)c)
	    return FNM_NOMATCH;
	  break;

	case '*':		/* Match zero or more characters */
	  if (p == pe)
	    return 0;
	  
	  if ((flags & FNM_PERIOD) && sc == L('.') &&
	      (n == string || ((flags & FNM_PATHNAME) && n[-1] == L('/'))))
	    /* `*' cannot match a `.' if it is the first character of the
	       string or if it is the first character following a slash and
	       we are matching a pathname. */
	    return FNM_NOMATCH;

	  /* Collapse multiple consecutive `*' and `?', but make sure that
	     one character of the string is consumed for each `?'. */
	  for (c = *p++; (c == L('?') || c == L('*')); c = *p++)
	    {
	      if ((flags & FNM_PATHNAME) && sc == L('/'))
		/* A slash does not match a wildcard under FNM_PATHNAME. */
		return FNM_NOMATCH;
#ifdef EXTENDED_GLOB
	      else if ((flags & FNM_EXTMATCH) && c == L('?') && *p == L('(')) /* ) */
		{
		  CHAR *newn;
		  for (newn = n; newn < se; ++newn)
		    {
		      if (EXTMATCH (c, newn, se, p, pe, flags) == 0)
			return (0);
		    }
		  /* We didn't match.  If we have a `?(...)', that's failure. */
		  return FNM_NOMATCH;
		}
#endif
	      else if (c == L('?'))
		{
		  if (sc == L('\0'))
		    return FNM_NOMATCH;
		  /* One character of the string is consumed in matching
		     this ? wildcard, so *??? won't match if there are
		     fewer than three characters. */
		  n++;
		  sc = n < se ? *n : '\0';
		}

#ifdef EXTENDED_GLOB
	      /* Handle ******(patlist) */
	      if ((flags & FNM_EXTMATCH) && c == L('*') && *p == L('('))  /*)*/
		{
		  CHAR *newn;
		  /* We need to check whether or not the extended glob
		     pattern matches the remainder of the string.
		     If it does, we match the entire pattern. */
		  for (newn = n; newn < se; ++newn)
		    {
		      if (EXTMATCH (c, newn, se, p, pe, flags) == 0)
			return (0);
		    }
		  /* We didn't match the extended glob pattern, but
		     that's OK, since we can match 0 or more occurrences.
		     We need to skip the glob pattern and see if we
		     match the rest of the string. */
		  newn = PATSCAN (p + 1, pe, 0);
		  /* If NEWN is 0, we have an ill-formed pattern. */
		  p = newn ? newn : pe;
		}
#endif
	      if (p == pe)
		break;
	    }

	  /* If we've hit the end of the pattern and the last character of
	     the pattern was handled by the loop above, we've succeeded.
	     Otherwise, we need to match that last character. */
	  if (p == pe && (c == L('?') || c == L('*')))
	    return (0);

	  /* General case, use recursion. */
	  {
	    U_CHAR c1;

	    c1 = ((flags & FNM_NOESCAPE) == 0 && c == L('\\')) ? *p : c;
	    c1 = FOLD (c1);
	    for (--p; n < se; ++n)
	      {
		/* Only call strmatch if the first character indicates a
		   possible match.  We can check the first character if
		   we're not doing an extended glob match. */
		if ((flags & FNM_EXTMATCH) == 0 && c != L('[') && FOLD (*n) != c1) /*]*/
		  continue;

		/* If we're doing an extended glob match and the pattern is not
		   one of the extended glob patterns, we can check the first
		   character. */
		if ((flags & FNM_EXTMATCH) && p[1] != L('(') && /*)*/
		    STRCHR (L("?*+@!"), *p) == 0 && c != L('[') && FOLD (*n) != c1) /*]*/
		  continue;

		/* Otherwise, we just recurse. */
		if (GMATCH (n, se, p, pe, flags & ~FNM_PERIOD) == 0)
		  return (0);
	      }
	    return FNM_NOMATCH;
	  }

	case L('['):
	  {
	    if (sc == L('\0') || n == se)
	      return FNM_NOMATCH;

	    /* A character class cannot match a `.' if it is the first
	       character of the string or if it is the first character
	       following a slash and we are matching a pathname. */
	    if ((flags & FNM_PERIOD) && sc == L('.') &&
		(n == string || ((flags & FNM_PATHNAME) && n[-1] == L('/'))))
	      return (FNM_NOMATCH);

	    p = BRACKMATCH (p, sc, flags);
	    if (p == 0)
	      return FNM_NOMATCH;
	  }
	  break;

	default:
	  if ((U_CHAR)c != FOLD (sc))
	    return (FNM_NOMATCH);
	}

      ++n;
    }

  if (n == se)
    return (0);

  if ((flags & FNM_LEADING_DIR) && *n == L('/'))
    /* The FNM_LEADING_DIR flag says that "foo*" matches "foobar/frobozz".  */
    return 0;
	  
  return (FNM_NOMATCH);
}

/* Parse a bracket expression collating symbol ([.sym.]) starting at P, find
   the value of the symbol, and move P past the collating symbol expression.
   The value is returned in *VP, if VP is not null. */
static CHAR *
PARSE_COLLSYM (p, vp)
     CHAR *p;
     INT *vp;
{
  register int pc;
  INT val;

  p++;				/* move past the `.' */
	  
  for (pc = 0; p[pc]; pc++)
    if (p[pc] == L('.') && p[pc+1] == L(']'))
      break;
   val = COLLSYM (p, pc);
   if (vp)
     *vp = val;
   return (p + pc + 2);
}

/* Use prototype definition here because of type promotion. */
static CHAR *
#if defined (PROTOTYPES)
BRACKMATCH (CHAR *p, U_CHAR test, int flags)
#else
BRACKMATCH (p, test, flags)
     CHAR *p;
     U_CHAR test;
     int flags;
#endif
{
  register CHAR cstart, cend, c;
  register int not;    /* Nonzero if the sense of the character class is inverted.  */
  int brcnt;
  INT pc;
  CHAR *savep;

  test = FOLD (test);

  savep = p;

  /* POSIX.2 3.13.1 says that an exclamation mark (`!') shall replace the
     circumflex (`^') in its role in a `nonmatching list'.  A bracket
     expression starting with an unquoted circumflex character produces
     unspecified results.  This implementation treats the two identically. */
  if (not = (*p == L('!') || *p == L('^')))
    ++p;

  c = *p++;
  for (;;)
    {
      /* Initialize cstart and cend in case `-' is the last
	 character of the pattern. */
      cstart = cend = c;

      /* POSIX.2 equivalence class:  [=c=].  See POSIX.2 2.8.3.2.  Find
	 the end of the equivalence class, move the pattern pointer past
	 it, and check for equivalence.  XXX - this handles only
	 single-character equivalence classes, which is wrong, or at
	 least incomplete. */
      if (c == L('[') && *p == L('=') && p[2] == L('=') && p[3] == L(']'))
	{
	  pc = FOLD (p[1]);
	  p += 4;
	  if (COLLEQUIV (test, pc))
	    {
/*[*/	      /* Move past the closing `]', since the first thing we do at
		 the `matched:' label is back p up one. */
	      p++;
	      goto matched;
	    }
	  else
	    {
	      c = *p++;
	      if (c == L('\0'))
		return ((test == L('[')) ? savep : (CHAR *)0); /*]*/
	      c = FOLD (c);
	      continue;
	    }
	}

      /* POSIX.2 character class expression.  See POSIX.2 2.8.3.2. */
      if (c == L('[') && *p == L(':'))
	{
	  CHAR *close, *ccname;

	  pc = 0;	/* make sure invalid char classes don't match. */
	  /* Find end of character class name */
	  for (close = p + 1; *close != '\0'; close++)
	    if (*close == L(':') && *(close+1) == L(']'))
	      break;

	  if (*close != L('\0'))
	    {
	      ccname = (CHAR *)malloc ((close - p) * sizeof (CHAR));
	      if (ccname == 0)
		pc = 0;
	      else
		{
		  bcopy (p + 1, ccname, (close - p - 1) * sizeof (CHAR));
		  *(ccname + (close - p - 1)) = L('\0');
		  pc = IS_CCLASS (test, (XCHAR *)ccname);
		}
	      if (pc == -1)
		pc = 0;
	      else
		p = close + 2;

	      free (ccname);
	    }
	    
	  if (pc)
	    {
/*[*/	      /* Move past the closing `]', since the first thing we do at
		 the `matched:' label is back p up one. */
	      p++;
	      goto matched;
	    }
	  else
	    {
	      /* continue the loop here, since this expression can't be
		 the first part of a range expression. */
	      c = *p++;
	      if (c == L('\0'))
		return ((test == L('[')) ? savep : (CHAR *)0);
	      else if (c == L(']'))
		break;
	      c = FOLD (c);
	      continue;
	    }
	}
 
      /* POSIX.2 collating symbols.  See POSIX.2 2.8.3.2.  Find the end of
	 the symbol name, make sure it is terminated by `.]', translate
	 the name to a character using the external table, and do the
	 comparison. */
      if (c == L('[') && *p == L('.'))
	{
	  p = PARSE_COLLSYM (p, &pc);
	  /* An invalid collating symbol cannot be the first point of a
	     range.  If it is, we set cstart to one greater than `test',
	     so any comparisons later will fail. */
	  cstart = (pc == INVALID) ? test + 1 : pc;
	}

      if (!(flags & FNM_NOESCAPE) && c == L('\\'))
	{
	  if (*p == '\0')
	    return (CHAR *)0;
	  cstart = cend = *p++;
	}

      cstart = cend = FOLD (cstart);

      /* POSIX.2 2.8.3.1.2 says: `An expression containing a `[' that
	 is not preceded by a backslash and is not part of a bracket
	 expression produces undefined results.'  This implementation
	 treats the `[' as just a character to be matched if there is
	 not a closing `]'. */
      if (c == L('\0'))
	return ((test == L('[')) ? savep : (CHAR *)0);

      c = *p++;
      c = FOLD (c);

      if ((flags & FNM_PATHNAME) && c == L('/'))
	/* [/] can never match when matching a pathname.  */
	return (CHAR *)0;

      /* This introduces a range, unless the `-' is the last
	 character of the class.  Find the end of the range
	 and move past it. */
      if (c == L('-') && *p != L(']'))
	{
	  cend = *p++;
	  if (!(flags & FNM_NOESCAPE) && cend == L('\\'))
	    cend = *p++;
	  if (cend == L('\0'))
	    return (CHAR *)0;
	  if (cend == L('[') && *p == L('.'))
	    {
	      p = PARSE_COLLSYM (p, &pc);
	      /* An invalid collating symbol cannot be the second part of a
		 range expression.  If we get one, we set cend to one fewer
		 than the test character to make sure the range test fails. */
	      cend = (pc == INVALID) ? test - 1 : pc;
	    }
	  cend = FOLD (cend);

	  c = *p++;

	  /* POSIX.2 2.8.3.2:  ``The ending range point shall collate
	     equal to or higher than the starting range point; otherwise
	     the expression shall be treated as invalid.''  Note that this
	     applies to only the range expression; the rest of the bracket
	     expression is still checked for matches. */
	  if (RANGECMP (cstart, cend) > 0)
	    {
	      if (c == L(']'))
		break;
	      c = FOLD (c);
	      continue;
	    }
	}

      if (RANGECMP (test, cstart) >= 0 && RANGECMP (test, cend) <= 0)
	goto matched;

      if (c == L(']'))
	break;
    }
  /* No match. */
  return (!not ? (CHAR *)0 : p);

matched:
  /* Skip the rest of the [...] that already matched.  */
  c = *--p;
  brcnt = 1;
  while (brcnt > 0)
    {
      /* A `[' without a matching `]' is just another character to match. */
      if (c == L('\0'))
	return ((test == L('[')) ? savep : (CHAR *)0);

      c = *p++;
      if (c == L('[') && (*p == L('=') || *p == L(':') || *p == L('.')))
	brcnt++;
      else if (c == L(']'))
	brcnt--;
      else if (!(flags & FNM_NOESCAPE) && c == L('\\'))
	{
	  if (*p == '\0')
	    return (CHAR *)0;
	  /* XXX 1003.2d11 is unclear if this is right. */
	  ++p;
	}
    }
  return (not ? (CHAR *)0 : p);
}

#if defined (EXTENDED_GLOB)
/* ksh-like extended pattern matching:

	[?*+@!](pat-list)

   where pat-list is a list of one or patterns separated by `|'.  Operation
   is as follows:

	?(patlist)	match zero or one of the given patterns
	*(patlist)	match zero or more of the given patterns
	+(patlist)	match one or more of the given patterns
	@(patlist)	match exactly one of the given patterns
	!(patlist)	match anything except one of the given patterns
*/

/* Scan a pattern starting at STRING and ending at END, keeping track of
   embedded () and [].  If DELIM is 0, we scan until a matching `)'
   because we're scanning a `patlist'.  Otherwise, we scan until we see
   DELIM.  In all cases, we never scan past END.  The return value is the
   first character after the matching DELIM. */
static CHAR *
PATSCAN (string, end, delim)
     CHAR *string, *end;
     INT delim;
{
  int pnest, bnest, skip;
  INT cchar;
  CHAR *s, c, *bfirst;

  pnest = bnest = skip = 0;
  cchar = 0;
  bfirst = NULL;

  for (s = string; c = *s; s++)
    {
      if (s >= end)
	return (s);
      if (skip)
	{
	  skip = 0;
	  continue;
	}
      switch (c)
	{
	case L('\\'):
	  skip = 1;
	  break;

	case L('\0'):
	  return ((CHAR *)NULL);

	/* `[' is not special inside a bracket expression, but it may
	   introduce one of the special POSIX bracket expressions
	   ([.SYM.], [=c=], [: ... :]) that needs special handling. */
	case L('['):
	  if (bnest == 0)
	    {
	      bfirst = s + 1;
	      if (*bfirst == L('!') || *bfirst == L('^'))
		bfirst++;
	      bnest++;
	    }
	  else if (s[1] == L(':') || s[1] == L('.') || s[1] == L('='))
	    cchar = s[1];
	  break;

	/* `]' is not special if it's the first char (after a leading `!'
	   or `^') in a bracket expression or if it's part of one of the
	   special POSIX bracket expressions ([.SYM.], [=c=], [: ... :]) */
	case L(']'):
	  if (bnest)
	    {
	      if (cchar && s[-1] == cchar)
		cchar = 0;
	      else if (s != bfirst)
		{
		  bnest--;
		  bfirst = 0;
		}
	    }
	  break;

	case L('('):
	  if (bnest == 0)
	    pnest++;
	  break;

	case L(')'):
	  if (bnest == 0 && pnest-- <= 0)
	    return ++s;
	  break;

	case L('|'):
	  if (bnest == 0 && pnest == 0 && delim == L('|'))
	    return ++s;
	  break;
	}
    }

  return (NULL);
}

/* Return 0 if dequoted pattern matches S in the current locale. */
static int
STRCOMPARE (p, pe, s, se)
     CHAR *p, *pe, *s, *se;
{
  int ret;
  CHAR c1, c2;

  c1 = *pe;
  c2 = *se;

  *pe = *se = '\0';
#if HAVE_MULTIBYTE || defined (HAVE_STRCOLL)
  ret = STRCOLL ((XCHAR *)p, (XCHAR *)s);
#else
  ret = STRCMP ((XCHAR *)p, (XCHAR *)s);
#endif

  *pe = c1;
  *se = c2;

  return (ret == 0 ? ret : FNM_NOMATCH);
}

/* Match a ksh extended pattern specifier.  Return FNM_NOMATCH on failure or
   0 on success.  This is handed the entire rest of the pattern and string
   the first time an extended pattern specifier is encountered, so it calls
   gmatch recursively. */
static int
EXTMATCH (xc, s, se, p, pe, flags)
     INT xc;		/* select which operation */
     CHAR *s, *se;
     CHAR *p, *pe;
     int flags;
{
  CHAR *prest;			/* pointer to rest of pattern */
  CHAR *psub;			/* pointer to sub-pattern */
  CHAR *pnext;			/* pointer to next sub-pattern */
  CHAR *srest;			/* pointer to rest of string */
  int m1, m2, xflags;		/* xflags = flags passed to recursive matches */

#if DEBUG_MATCHING
fprintf(stderr, "extmatch: xc = %c\n", xc);
fprintf(stderr, "extmatch: s = %s; se = %s\n", s, se);
fprintf(stderr, "extmatch: p = %s; pe = %s\n", p, pe);
fprintf(stderr, "extmatch: flags = %d\n", flags);
#endif

  prest = PATSCAN (p + (*p == L('(')), pe, 0); /* ) */
  if (prest == 0)
    /* If PREST is 0, we failed to scan a valid pattern.  In this
       case, we just want to compare the two as strings. */
    return (STRCOMPARE (p - 1, pe, s, se));

  switch (xc)
    {
    case L('+'):		/* match one or more occurrences */
    case L('*'):		/* match zero or more occurrences */
      /* If we can get away with no matches, don't even bother.  Just
	 call GMATCH on the rest of the pattern and return success if
	 it succeeds. */
      if (xc == L('*') && (GMATCH (s, se, prest, pe, flags) == 0))
	return 0;

      /* OK, we have to do this the hard way.  First, we make sure one of
	 the subpatterns matches, then we try to match the rest of the
	 string. */
      for (psub = p + 1; ; psub = pnext)
	{
	  pnext = PATSCAN (psub, pe, L('|'));
	  for (srest = s; srest <= se; srest++)
	    {
	      /* Match this substring (S -> SREST) against this
		 subpattern (psub -> pnext - 1) */
	      m1 = GMATCH (s, srest, psub, pnext - 1, flags) == 0;
	      /* OK, we matched a subpattern, so make sure the rest of the
		 string matches the rest of the pattern.  Also handle
		 multiple matches of the pattern. */
	      if (m1)
		{
		  /* if srest > s, we are not at start of string */
		  xflags = (srest > s) ? (flags & ~FNM_PERIOD) : flags;
		  m2 = (GMATCH (srest, se, prest, pe, xflags) == 0) ||
			(s != srest && GMATCH (srest, se, p - 1, pe, xflags) == 0);
		}
	      if (m1 && m2)
		return (0);
	    }
	  if (pnext == prest)
	    break;
	}
      return (FNM_NOMATCH);

    case L('?'):		/* match zero or one of the patterns */
    case L('@'):		/* match one (or more) of the patterns */
      /* If we can get away with no matches, don't even bother.  Just
	 call gmatch on the rest of the pattern and return success if
	 it succeeds. */
      if (xc == L('?') && (GMATCH (s, se, prest, pe, flags) == 0))
	return 0;

      /* OK, we have to do this the hard way.  First, we see if one of
	 the subpatterns matches, then, if it does, we try to match the
	 rest of the string. */
      for (psub = p + 1; ; psub = pnext)
	{
	  pnext = PATSCAN (psub, pe, L('|'));
	  srest = (prest == pe) ? se : s;
	  for ( ; srest <= se; srest++)
	    {
	      /* if srest > s, we are not at start of string */
	      xflags = (srest > s) ? (flags & ~FNM_PERIOD) : flags;
	      if (GMATCH (s, srest, psub, pnext - 1, flags) == 0 &&
		  GMATCH (srest, se, prest, pe, xflags) == 0)
		return (0);
	    }
	  if (pnext == prest)
	    break;
	}
      return (FNM_NOMATCH);

    case '!':		/* match anything *except* one of the patterns */
      for (srest = s; srest <= se; srest++)
	{
	  m1 = 0;
	  for (psub = p + 1; ; psub = pnext)
	    {
	      pnext = PATSCAN (psub, pe, L('|'));
	      /* If one of the patterns matches, just bail immediately. */
	      if (m1 = (GMATCH (s, srest, psub, pnext - 1, flags) == 0))
		break;
	      if (pnext == prest)
		break;
	    }
	  /* if srest > s, we are not at start of string */
	  xflags = (srest > s) ? (flags & ~FNM_PERIOD) : flags;
	  if (m1 == 0 && GMATCH (srest, se, prest, pe, xflags) == 0)
	    return (0);
	}
      return (FNM_NOMATCH);
    }

  return (FNM_NOMATCH);
}
#endif /* EXTENDED_GLOB */

#undef IS_CCLASS
#undef FOLD
#undef CHAR
#undef U_CHAR
#undef XCHAR
#undef INT
#undef INVALID
#undef FCT
#undef GMATCH
#undef COLLSYM
#undef PARSE_COLLSYM
#undef PATSCAN
#undef STRCOMPARE
#undef EXTMATCH
#undef BRACKMATCH
#undef STRCHR
#undef STRCOLL
#undef STRLEN
#undef STRCMP
#undef COLLEQUIV
#undef RANGECMP
#undef L
