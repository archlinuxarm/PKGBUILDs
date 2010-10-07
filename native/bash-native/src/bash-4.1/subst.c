/* subst.c -- The part of the shell that does parameter, command, arithmetic,
   and globbing substitutions. */

/* ``Have a little faith, there's magic in the night.  You ain't a
     beauty, but, hey, you're alright.'' */

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

#include "config.h"

#include "bashtypes.h"
#include <stdio.h>
#include "chartypes.h"
#if defined (HAVE_PWD_H)
#  include <pwd.h>
#endif
#include <signal.h>
#include <errno.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "bashansi.h"
#include "posixstat.h"
#include "bashintl.h"

#include "shell.h"
#include "flags.h"
#include "jobs.h"
#include "execute_cmd.h"
#include "filecntl.h"
#include "trap.h"
#include "pathexp.h"
#include "mailcheck.h"

#include "shmbutil.h"

#include "builtins/getopt.h"
#include "builtins/common.h"

#include "builtins/builtext.h"

#include <tilde/tilde.h>
#include <glob/strmatch.h>

#if !defined (errno)
extern int errno;
#endif /* !errno */

/* The size that strings change by. */
#define DEFAULT_INITIAL_ARRAY_SIZE 112
#define DEFAULT_ARRAY_SIZE 128

/* Variable types. */
#define VT_VARIABLE	0
#define VT_POSPARMS	1
#define VT_ARRAYVAR	2
#define VT_ARRAYMEMBER	3
#define VT_ASSOCVAR	4

#define VT_STARSUB	128	/* $* or ${array[*]} -- used to split */

/* Flags for quoted_strchr */
#define ST_BACKSL	0x01
#define ST_CTLESC	0x02
#define ST_SQUOTE	0x04	/* unused yet */
#define ST_DQUOTE	0x08	/* unused yet */

/* Flags for the `pflags' argument to param_expand() */
#define PF_NOCOMSUB	0x01	/* Do not perform command substitution */
#define PF_IGNUNBOUND	0x02	/* ignore unbound vars even if -u set */
#define PF_NOSPLIT2	0x04	/* same as W_NOSPLIT2 */

/* These defs make it easier to use the editor. */
#define LBRACE		'{'
#define RBRACE		'}'
#define LPAREN		'('
#define RPAREN		')'

#if defined (HANDLE_MULTIBYTE)
#define WLPAREN		L'('
#define WRPAREN		L')'
#endif

/* Evaluates to 1 if C is one of the shell's special parameters whose length
   can be taken, but is also one of the special expansion characters. */
#define VALID_SPECIAL_LENGTH_PARAM(c) \
  ((c) == '-' || (c) == '?' || (c) == '#')

/* Evaluates to 1 if C is one of the shell's special parameters for which an
   indirect variable reference may be made. */
#define VALID_INDIR_PARAM(c) \
  ((c) == '#' || (c) == '?' || (c) == '@' || (c) == '*')

/* Evaluates to 1 if C is one of the OP characters that follows the parameter
   in ${parameter[:]OPword}. */
#define VALID_PARAM_EXPAND_CHAR(c) (sh_syntaxtab[(unsigned char)c] & CSUBSTOP)

/* Evaluates to 1 if this is one of the shell's special variables. */
#define SPECIAL_VAR(name, wi) \
 ((DIGIT (*name) && all_digits (name)) || \
      (name[1] == '\0' && (sh_syntaxtab[(unsigned char)*name] & CSPECVAR)) || \
      (wi && name[2] == '\0' && VALID_INDIR_PARAM (name[1])))

/* An expansion function that takes a string and a quoted flag and returns
   a WORD_LIST *.  Used as the type of the third argument to
   expand_string_if_necessary(). */
typedef WORD_LIST *EXPFUNC __P((char *, int));

/* Process ID of the last command executed within command substitution. */
pid_t last_command_subst_pid = NO_PID;
pid_t current_command_subst_pid = NO_PID;

/* Variables used to keep track of the characters in IFS. */
SHELL_VAR *ifs_var;
char *ifs_value;
unsigned char ifs_cmap[UCHAR_MAX + 1];

#if defined (HANDLE_MULTIBYTE)
unsigned char ifs_firstc[MB_LEN_MAX];
size_t ifs_firstc_len;
#else
unsigned char ifs_firstc;
#endif

/* Sentinel to tell when we are performing variable assignments preceding a
   command name and putting them into the environment.  Used to make sure
   we use the temporary environment when looking up variable values. */
int assigning_in_environment;

/* Used to hold a list of variable assignments preceding a command.  Global
   so the SIGCHLD handler in jobs.c can unwind-protect it when it runs a
   SIGCHLD trap and so it can be saved and restored by the trap handlers. */
WORD_LIST *subst_assign_varlist = (WORD_LIST *)NULL;

/* Extern functions and variables from different files. */
extern int last_command_exit_value, last_command_exit_signal;
extern int subshell_environment, line_number;
extern int subshell_level, parse_and_execute_level, sourcelevel;
extern int eof_encountered;
extern int return_catch_flag, return_catch_value;
extern pid_t dollar_dollar_pid;
extern int posixly_correct;
extern char *this_command_name;
extern struct fd_bitmap *current_fds_to_close;
extern int wordexp_only;
extern int expanding_redir;
extern int tempenv_assign_error;

#if !defined (HAVE_WCSDUP) && defined (HANDLE_MULTIBYTE)
extern wchar_t *wcsdup __P((const wchar_t *));
#endif

/* Non-zero means to allow unmatched globbed filenames to expand to
   a null file. */
int allow_null_glob_expansion;

/* Non-zero means to throw an error when globbing fails to match anything. */
int fail_glob_expansion;

#if 0
/* Variables to keep track of which words in an expanded word list (the
   output of expand_word_list_internal) are the result of globbing
   expansions.  GLOB_ARGV_FLAGS is used by execute_cmd.c.
   (CURRENTLY UNUSED). */
char *glob_argv_flags;
static int glob_argv_flags_size;
#endif

static WORD_LIST expand_word_error, expand_word_fatal;
static WORD_DESC expand_wdesc_error, expand_wdesc_fatal;
static char expand_param_error, expand_param_fatal;
static char extract_string_error, extract_string_fatal;

/* Tell the expansion functions to not longjmp back to top_level on fatal
   errors.  Enabled when doing completion and prompt string expansion. */
static int no_longjmp_on_fatal_error = 0;

/* Set by expand_word_unsplit; used to inhibit splitting and re-joining
   $* on $IFS, primarily when doing assignment statements. */
static int expand_no_split_dollar_star = 0;

/* A WORD_LIST of words to be expanded by expand_word_list_internal,
   without any leading variable assignments. */
static WORD_LIST *garglist = (WORD_LIST *)NULL;

static char *quoted_substring __P((char *, int, int));
static int quoted_strlen __P((char *));
static char *quoted_strchr __P((char *, int, int));

static char *expand_string_if_necessary __P((char *, int, EXPFUNC *));
static inline char *expand_string_to_string_internal __P((char *, int, EXPFUNC *));
static WORD_LIST *call_expand_word_internal __P((WORD_DESC *, int, int, int *, int *));
static WORD_LIST *expand_string_internal __P((char *, int));
static WORD_LIST *expand_string_leave_quoted __P((char *, int));
static WORD_LIST *expand_string_for_rhs __P((char *, int, int *, int *));

static WORD_LIST *list_quote_escapes __P((WORD_LIST *));
static char *make_quoted_char __P((int));
static WORD_LIST *quote_list __P((WORD_LIST *));

static int unquoted_substring __P((char *, char *));
static int unquoted_member __P((int, char *));

#if defined (ARRAY_VARS)
static SHELL_VAR *do_compound_assignment __P((char *, char *, int));
#endif
static int do_assignment_internal __P((const WORD_DESC *, int));

static char *string_extract_verbatim __P((char *, size_t, int *, char *, int));
static char *string_extract __P((char *, int *, char *, int));
static char *string_extract_double_quoted __P((char *, int *, int));
static inline char *string_extract_single_quoted __P((char *, int *));
static inline int skip_single_quoted __P((const char *, size_t, int));
static int skip_double_quoted __P((char *, size_t, int));
static char *extract_delimited_string __P((char *, int *, char *, char *, char *, int));
static char *extract_dollar_brace_string __P((char *, int *, int, int));
static int skip_matched_pair __P((const char *, int, int, int, int));

static char *pos_params __P((char *, int, int, int));

static unsigned char *mb_getcharlens __P((char *, int));

static char *remove_upattern __P((char *, char *, int));
#if defined (HANDLE_MULTIBYTE) 
static wchar_t *remove_wpattern __P((wchar_t *, size_t, wchar_t *, int));
#endif
static char *remove_pattern __P((char *, char *, int));

static int match_pattern_char __P((char *, char *));
static int match_upattern __P((char *, char *, int, char **, char **));
#if defined (HANDLE_MULTIBYTE)
static int match_pattern_wchar __P((wchar_t *, wchar_t *));
static int match_wpattern __P((wchar_t *, char **, size_t, wchar_t *, int, char **, char **));
#endif
static int match_pattern __P((char *, char *, int, char **, char **));
static int getpatspec __P((int, char *));
static char *getpattern __P((char *, int, int));
static char *variable_remove_pattern __P((char *, char *, int, int));
static char *list_remove_pattern __P((WORD_LIST *, char *, int, int, int));
static char *parameter_list_remove_pattern __P((int, char *, int, int));
#ifdef ARRAY_VARS
static char *array_remove_pattern __P((SHELL_VAR *, char *, int, char *, int));
#endif
static char *parameter_brace_remove_pattern __P((char *, char *, char *, int, int));

static char *process_substitute __P((char *, int));

static char *read_comsub __P((int, int, int *));

#ifdef ARRAY_VARS
static arrayind_t array_length_reference __P((char *));
#endif

static int valid_brace_expansion_word __P((char *, int));
static int chk_atstar __P((char *, int, int *, int *));
static int chk_arithsub __P((const char *, int));

static WORD_DESC *parameter_brace_expand_word __P((char *, int, int, int));
static WORD_DESC *parameter_brace_expand_indir __P((char *, int, int, int *, int *));
static WORD_DESC *parameter_brace_expand_rhs __P((char *, char *, int, int, int *, int *));
static void parameter_brace_expand_error __P((char *, char *));

static int valid_length_expression __P((char *));
static intmax_t parameter_brace_expand_length __P((char *));

static char *skiparith __P((char *, int));
static int verify_substring_values __P((SHELL_VAR *, char *, char *, int, intmax_t *, intmax_t *));
static int get_var_and_type __P((char *, char *, int, SHELL_VAR **, char **));
static char *mb_substring __P((char *, int, int));
static char *parameter_brace_substring __P((char *, char *, char *, int));

static char *pos_params_pat_subst __P((char *, char *, char *, int));

static char *parameter_brace_patsub __P((char *, char *, char *, int));

static char *pos_params_casemod __P((char *, char *, int, int));
static char *parameter_brace_casemod __P((char *, char *, int, char *, int));

static WORD_DESC *parameter_brace_expand __P((char *, int *, int, int, int *, int *));
static WORD_DESC *param_expand __P((char *, int *, int, int *, int *, int *, int *, int));

static WORD_LIST *expand_word_internal __P((WORD_DESC *, int, int, int *, int *));

static WORD_LIST *word_list_split __P((WORD_LIST *));

static void exp_jump_to_top_level __P((int));

static WORD_LIST *separate_out_assignments __P((WORD_LIST *));
static WORD_LIST *glob_expand_word_list __P((WORD_LIST *, int));
#ifdef BRACE_EXPANSION
static WORD_LIST *brace_expand_word_list __P((WORD_LIST *, int));
#endif
#if defined (ARRAY_VARS)
static int make_internal_declare __P((char *, char *));
#endif
static WORD_LIST *shell_expand_word_list __P((WORD_LIST *, int));
static WORD_LIST *expand_word_list_internal __P((WORD_LIST *, int));

/* **************************************************************** */
/*								    */
/*			Utility Functions			    */
/*								    */
/* **************************************************************** */

#if defined (DEBUG)
void
dump_word_flags (flags)
     int flags;
{
  int f;

  f = flags;
  fprintf (stderr, "%d -> ", f);
  if (f & W_ASSIGNASSOC)
    {
      f &= ~W_ASSIGNASSOC;
      fprintf (stderr, "W_ASSIGNASSOC%s", f ? "|" : "");
    }
  if (f & W_HASCTLESC)
    {
      f &= ~W_HASCTLESC;
      fprintf (stderr, "W_HASCTLESC%s", f ? "|" : "");
    }
  if (f & W_NOPROCSUB)
    {
      f &= ~W_NOPROCSUB;
      fprintf (stderr, "W_NOPROCSUB%s", f ? "|" : "");
    }
  if (f & W_DQUOTE)
    {
      f &= ~W_DQUOTE;
      fprintf (stderr, "W_DQUOTE%s", f ? "|" : "");
    }
  if (f & W_HASQUOTEDNULL)
    {
      f &= ~W_HASQUOTEDNULL;
      fprintf (stderr, "W_HASQUOTEDNULL%s", f ? "|" : "");
    }
  if (f & W_ASSIGNARG)
    {
      f &= ~W_ASSIGNARG;
      fprintf (stderr, "W_ASSIGNARG%s", f ? "|" : "");
    }
  if (f & W_ASSNBLTIN)
    {
      f &= ~W_ASSNBLTIN;
      fprintf (stderr, "W_ASSNBLTIN%s", f ? "|" : "");
    }
  if (f & W_COMPASSIGN)
    {
      f &= ~W_COMPASSIGN;
      fprintf (stderr, "W_COMPASSIGN%s", f ? "|" : "");
    }
  if (f & W_NOEXPAND)
    {
      f &= ~W_NOEXPAND;
      fprintf (stderr, "W_NOEXPAND%s", f ? "|" : "");
    }
  if (f & W_ITILDE)
    {
      f &= ~W_ITILDE;
      fprintf (stderr, "W_ITILDE%s", f ? "|" : "");
    }
  if (f & W_NOTILDE)
    {
      f &= ~W_NOTILDE;
      fprintf (stderr, "W_NOTILDE%s", f ? "|" : "");
    }
  if (f & W_ASSIGNRHS)
    {
      f &= ~W_ASSIGNRHS;
      fprintf (stderr, "W_ASSIGNRHS%s", f ? "|" : "");
    }
  if (f & W_NOCOMSUB)
    {
      f &= ~W_NOCOMSUB;
      fprintf (stderr, "W_NOCOMSUB%s", f ? "|" : "");
    }
  if (f & W_DOLLARSTAR)
    {
      f &= ~W_DOLLARSTAR;
      fprintf (stderr, "W_DOLLARSTAR%s", f ? "|" : "");
    }
  if (f & W_DOLLARAT)
    {
      f &= ~W_DOLLARAT;
      fprintf (stderr, "W_DOLLARAT%s", f ? "|" : "");
    }
  if (f & W_TILDEEXP)
    {
      f &= ~W_TILDEEXP;
      fprintf (stderr, "W_TILDEEXP%s", f ? "|" : "");
    }
  if (f & W_NOSPLIT2)
    {
      f &= ~W_NOSPLIT2;
      fprintf (stderr, "W_NOSPLIT2%s", f ? "|" : "");
    }
  if (f & W_NOGLOB)
    {
      f &= ~W_NOGLOB;
      fprintf (stderr, "W_NOGLOB%s", f ? "|" : "");
    }
  if (f & W_NOSPLIT)
    {
      f &= ~W_NOSPLIT;
      fprintf (stderr, "W_NOSPLIT%s", f ? "|" : "");
    }
  if (f & W_GLOBEXP)
    {
      f &= ~W_GLOBEXP;
      fprintf (stderr, "W_GLOBEXP%s", f ? "|" : "");
    }
  if (f & W_ASSIGNMENT)
    {
      f &= ~W_ASSIGNMENT;
      fprintf (stderr, "W_ASSIGNMENT%s", f ? "|" : "");
    }
  if (f & W_QUOTED)
    {
      f &= ~W_QUOTED;
      fprintf (stderr, "W_QUOTED%s", f ? "|" : "");
    }
  if (f & W_HASDOLLAR)
    {
      f &= ~W_HASDOLLAR;
      fprintf (stderr, "W_HASDOLLAR%s", f ? "|" : "");
    }
  fprintf (stderr, "\n");
  fflush (stderr);
}
#endif

#ifdef INCLUDE_UNUSED
static char *
quoted_substring (string, start, end)
     char *string;
     int start, end;
{
  register int len, l;
  register char *result, *s, *r;

  len = end - start;

  /* Move to string[start], skipping quoted characters. */
  for (s = string, l = 0; *s && l < start; )
    {
      if (*s == CTLESC)
	{
	  s++;
	  continue;
	}
      l++;
      if (*s == 0)
	break;
    }

  r = result = (char *)xmalloc (2*len + 1);      /* save room for quotes */

  /* Copy LEN characters, including quote characters. */
  s = string + l;
  for (l = 0; l < len; s++)
    {
      if (*s == CTLESC)
	*r++ = *s++;
      *r++ = *s;
      l++;
      if (*s == 0)
	break;
    }
  *r = '\0';
  return result;
}
#endif

#ifdef INCLUDE_UNUSED
/* Return the length of S, skipping over quoted characters */
static int
quoted_strlen (s)
     char *s;
{
  register char *p;
  int i;

  i = 0;
  for (p = s; *p; p++)
    {
      if (*p == CTLESC)
	{
	  p++;
	  if (*p == 0)
	    return (i + 1);
	}
      i++;
    }

  return i;
}
#endif

/* Find the first occurrence of character C in string S, obeying shell
   quoting rules.  If (FLAGS & ST_BACKSL) is non-zero, backslash-escaped
   characters are skipped.  If (FLAGS & ST_CTLESC) is non-zero, characters
   escaped with CTLESC are skipped. */
static char *
quoted_strchr (s, c, flags)
     char *s;
     int c, flags;
{
  register char *p;

  for (p = s; *p; p++)
    {
      if (((flags & ST_BACKSL) && *p == '\\')
	    || ((flags & ST_CTLESC) && *p == CTLESC))
	{
	  p++;
	  if (*p == '\0')
	    return ((char *)NULL);
	  continue;
	}
      else if (*p == c)
	return p;
    }
  return ((char *)NULL);
}

/* Return 1 if CHARACTER appears in an unquoted portion of
   STRING.  Return 0 otherwise.  CHARACTER must be a single-byte character. */
static int
unquoted_member (character, string)
     int character;
     char *string;
{
  size_t slen;
  int sindex, c;
  DECLARE_MBSTATE;

  slen = strlen (string);
  sindex = 0;
  while (c = string[sindex])
    {
      if (c == character)
	return (1);

      switch (c)
	{
	default:
	  ADVANCE_CHAR (string, slen, sindex);
	  break;

	case '\\':
	  sindex++;
	  if (string[sindex])
	    ADVANCE_CHAR (string, slen, sindex);
	  break;

	case '\'':
	  sindex = skip_single_quoted (string, slen, ++sindex);
	  break;

	case '"':
	  sindex = skip_double_quoted (string, slen, ++sindex);
	  break;
	}
    }
  return (0);
}

/* Return 1 if SUBSTR appears in an unquoted portion of STRING. */
static int
unquoted_substring (substr, string)
     char *substr, *string;
{
  size_t slen;
  int sindex, c, sublen;
  DECLARE_MBSTATE;

  if (substr == 0 || *substr == '\0')
    return (0);

  slen = strlen (string);
  sublen = strlen (substr);
  for (sindex = 0; c = string[sindex]; )
    {
      if (STREQN (string + sindex, substr, sublen))
	return (1);

      switch (c)
	{
	case '\\':
	  sindex++;

	  if (string[sindex])
	    ADVANCE_CHAR (string, slen, sindex);
	  break;

	case '\'':
	  sindex = skip_single_quoted (string, slen, ++sindex);
	  break;

	case '"':
	  sindex = skip_double_quoted (string, slen, ++sindex);
	  break;

	default:
	  ADVANCE_CHAR (string, slen, sindex);
	  break;
	}
    }
  return (0);
}

/* Most of the substitutions must be done in parallel.  In order
   to avoid using tons of unclear goto's, I have some functions
   for manipulating malloc'ed strings.  They all take INDX, a
   pointer to an integer which is the offset into the string
   where manipulation is taking place.  They also take SIZE, a
   pointer to an integer which is the current length of the
   character array for this string. */

/* Append SOURCE to TARGET at INDEX.  SIZE is the current amount
   of space allocated to TARGET.  SOURCE can be NULL, in which
   case nothing happens.  Gets rid of SOURCE by freeing it.
   Returns TARGET in case the location has changed. */
INLINE char *
sub_append_string (source, target, indx, size)
     char *source, *target;
     int *indx, *size;
{
  if (source)
    {
      int srclen, n;

      srclen = STRLEN (source);
      if (srclen >= (int)(*size - *indx))
	{
	  n = srclen + *indx;
	  n = (n + DEFAULT_ARRAY_SIZE) - (n % DEFAULT_ARRAY_SIZE);
	  target = (char *)xrealloc (target, (*size = n));
	}

      FASTCOPY (source, target + *indx, srclen);
      *indx += srclen;
      target[*indx] = '\0';

      free (source);
    }
  return (target);
}

#if 0
/* UNUSED */
/* Append the textual representation of NUMBER to TARGET.
   INDX and SIZE are as in SUB_APPEND_STRING. */
char *
sub_append_number (number, target, indx, size)
     intmax_t number;
     int *indx, *size;
     char *target;
{
  char *temp;

  temp = itos (number);
  return (sub_append_string (temp, target, indx, size));
}
#endif

/* Extract a substring from STRING, starting at SINDEX and ending with
   one of the characters in CHARLIST.  Don't make the ending character
   part of the string.  Leave SINDEX pointing at the ending character.
   Understand about backslashes in the string.  If (flags & SX_VARNAME)
   is non-zero, and array variables have been compiled into the shell,
   everything between a `[' and a corresponding `]' is skipped over.
   If (flags & SX_NOALLOC) is non-zero, don't return the substring, just
   update SINDEX.  If (flags & SX_REQMATCH) is non-zero, the string must
   contain a closing character from CHARLIST. */
static char *
string_extract (string, sindex, charlist, flags)
     char *string;
     int *sindex;
     char *charlist;
     int flags;
{
  register int c, i;
  int found;
  size_t slen;
  char *temp;
  DECLARE_MBSTATE;

  slen = (MB_CUR_MAX > 1) ? strlen (string + *sindex) + *sindex : 0;
  i = *sindex;
  found = 0;
  while (c = string[i])
    {
      if (c == '\\')
	{
	  if (string[i + 1])
	    i++;
	  else
	    break;
	}
#if defined (ARRAY_VARS)
      else if ((flags & SX_VARNAME) && c == '[')
	{
	  int ni;
	  /* If this is an array subscript, skip over it and continue. */
	  ni = skipsubscript (string, i, 0);
	  if (string[ni] == ']')
	    i = ni;
	}
#endif
      else if (MEMBER (c, charlist))
	{
	  found = 1;
	  break;
	}

      ADVANCE_CHAR (string, slen, i);
    }

  /* If we had to have a matching delimiter and didn't find one, return an
     error and let the caller deal with it. */
  if ((flags & SX_REQMATCH) && found == 0)
    {
      *sindex = i;
      return (&extract_string_error);
    }
  
  temp = (flags & SX_NOALLOC) ? (char *)NULL : substring (string, *sindex, i);
  *sindex = i;
  
  return (temp);
}

/* Extract the contents of STRING as if it is enclosed in double quotes.
   SINDEX, when passed in, is the offset of the character immediately
   following the opening double quote; on exit, SINDEX is left pointing after
   the closing double quote.  If STRIPDQ is non-zero, unquoted double
   quotes are stripped and the string is terminated by a null byte.
   Backslashes between the embedded double quotes are processed.  If STRIPDQ
   is zero, an unquoted `"' terminates the string. */
static char *
string_extract_double_quoted (string, sindex, stripdq)
     char *string;
     int *sindex, stripdq;
{
  size_t slen;
  char *send;
  int j, i, t;
  unsigned char c;
  char *temp, *ret;		/* The new string we return. */
  int pass_next, backquote, si;	/* State variables for the machine. */
  int dquote;
  DECLARE_MBSTATE;

  slen = strlen (string + *sindex) + *sindex;
  send = string + slen;

  pass_next = backquote = dquote = 0;
  temp = (char *)xmalloc (1 + slen - *sindex);

  j = 0;
  i = *sindex;
  while (c = string[i])
    {
      /* Process a character that was quoted by a backslash. */
      if (pass_next)
	{
	  /* Posix.2 sez:

	     ``The backslash shall retain its special meaning as an escape
	     character only when followed by one of the characters:
		$	`	"	\	<newline>''.

	     If STRIPDQ is zero, we handle the double quotes here and let
	     expand_word_internal handle the rest.  If STRIPDQ is non-zero,
	     we have already been through one round of backslash stripping,
	     and want to strip these backslashes only if DQUOTE is non-zero,
	     indicating that we are inside an embedded double-quoted string. */

	     /* If we are in an embedded quoted string, then don't strip
		backslashes before characters for which the backslash
		retains its special meaning, but remove backslashes in
		front of other characters.  If we are not in an
		embedded quoted string, don't strip backslashes at all.
		This mess is necessary because the string was already
		surrounded by double quotes (and sh has some really weird
		quoting rules).
		The returned string will be run through expansion as if
		it were double-quoted. */
	  if ((stripdq == 0 && c != '"') ||
	      (stripdq && ((dquote && (sh_syntaxtab[c] & CBSDQUOTE)) || dquote == 0)))
	    temp[j++] = '\\';
	  pass_next = 0;

add_one_character:
	  COPY_CHAR_I (temp, j, string, send, i);
	  continue;
	}

      /* A backslash protects the next character.  The code just above
	 handles preserving the backslash in front of any character but
	 a double quote. */
      if (c == '\\')
	{
	  pass_next++;
	  i++;
	  continue;
	}

      /* Inside backquotes, ``the portion of the quoted string from the
	 initial backquote and the characters up to the next backquote
	 that is not preceded by a backslash, having escape characters
	 removed, defines that command''. */
      if (backquote)
	{
	  if (c == '`')
	    backquote = 0;
	  temp[j++] = c;
	  i++;
	  continue;
	}

      if (c == '`')
	{
	  temp[j++] = c;
	  backquote++;
	  i++;
	  continue;
	}

      /* Pass everything between `$(' and the matching `)' or a quoted
	 ${ ... } pair through according to the Posix.2 specification. */
      if (c == '$' && ((string[i + 1] == LPAREN) || (string[i + 1] == LBRACE)))
	{
	  int free_ret = 1;

	  si = i + 2;
	  if (string[i + 1] == LPAREN)
	    ret = extract_command_subst (string, &si, 0);
	  else
	    ret = extract_dollar_brace_string (string, &si, 1, 0);

	  temp[j++] = '$';
	  temp[j++] = string[i + 1];

	  /* Just paranoia; ret will not be 0 unless no_longjmp_on_fatal_error
	     is set. */
	  if (ret == 0 && no_longjmp_on_fatal_error)
	    {
	      free_ret = 0;
	      ret = string + i + 2;
	    }

	  for (t = 0; ret[t]; t++, j++)
	    temp[j] = ret[t];
	  temp[j] = string[si];

	  if (string[si])
	    {
	      j++;
	      i = si + 1;
	    }
	  else
	    i = si;

	  if (free_ret)
	    free (ret);
	  continue;
	}

      /* Add any character but a double quote to the quoted string we're
	 accumulating. */
      if (c != '"')
	goto add_one_character;

      /* c == '"' */
      if (stripdq)
	{
	  dquote ^= 1;
	  i++;
	  continue;
	}

      break;
    }
  temp[j] = '\0';

  /* Point to after the closing quote. */
  if (c)
    i++;
  *sindex = i;

  return (temp);
}

/* This should really be another option to string_extract_double_quoted. */
static int
skip_double_quoted (string, slen, sind)
     char *string;
     size_t slen;
     int sind;
{
  int c, i;
  char *ret;
  int pass_next, backquote, si;
  DECLARE_MBSTATE;

  pass_next = backquote = 0;
  i = sind;
  while (c = string[i])
    {
      if (pass_next)
	{
	  pass_next = 0;
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}
      else if (c == '\\')
	{
	  pass_next++;
	  i++;
	  continue;
	}
      else if (backquote)
	{
	  if (c == '`')
	    backquote = 0;
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}
      else if (c == '`')
	{
	  backquote++;
	  i++;
	  continue;
	}
      else if (c == '$' && ((string[i + 1] == LPAREN) || (string[i + 1] == LBRACE)))
	{
	  si = i + 2;
	  if (string[i + 1] == LPAREN)
	    ret = extract_command_subst (string, &si, SX_NOALLOC);
	  else
	    ret = extract_dollar_brace_string (string, &si, 1, SX_NOALLOC);

	  i = si + 1;
	  continue;
	}
      else if (c != '"')
	{
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}
      else
	break;
    }

  if (c)
    i++;

  return (i);
}

/* Extract the contents of STRING as if it is enclosed in single quotes.
   SINDEX, when passed in, is the offset of the character immediately
   following the opening single quote; on exit, SINDEX is left pointing after
   the closing single quote. */
static inline char *
string_extract_single_quoted (string, sindex)
     char *string;
     int *sindex;
{
  register int i;
  size_t slen;
  char *t;
  DECLARE_MBSTATE;

  /* Don't need slen for ADVANCE_CHAR unless multibyte chars possible. */
  slen = (MB_CUR_MAX > 1) ? strlen (string + *sindex) + *sindex : 0;
  i = *sindex;
  while (string[i] && string[i] != '\'')
    ADVANCE_CHAR (string, slen, i);

  t = substring (string, *sindex, i);

  if (string[i])
    i++;
  *sindex = i;

  return (t);
}

static inline int
skip_single_quoted (string, slen, sind)
     const char *string;
     size_t slen;
     int sind;
{
  register int c;
  DECLARE_MBSTATE;

  c = sind;
  while (string[c] && string[c] != '\'')
    ADVANCE_CHAR (string, slen, c);

  if (string[c])
    c++;
  return c;
}

/* Just like string_extract, but doesn't hack backslashes or any of
   that other stuff.  Obeys CTLESC quoting.  Used to do splitting on $IFS. */
static char *
string_extract_verbatim (string, slen, sindex, charlist, flags)
     char *string;
     size_t slen;
     int *sindex;
     char *charlist;
     int flags;
{
  register int i;
#if defined (HANDLE_MULTIBYTE)
  size_t clen;
  wchar_t *wcharlist;
#endif
  int c;
  char *temp;
  DECLARE_MBSTATE;

  if (charlist[0] == '\'' && charlist[1] == '\0')
    {
      temp = string_extract_single_quoted (string, sindex);
      --*sindex;	/* leave *sindex at separator character */
      return temp;
    }

  i = *sindex;
#if 0
  /* See how the MBLEN and ADVANCE_CHAR macros work to understand why we need
     this only if MB_CUR_MAX > 1. */
  slen = (MB_CUR_MAX > 1) ? strlen (string + *sindex) + *sindex : 1;
#endif
#if defined (HANDLE_MULTIBYTE)
  clen = strlen (charlist);
  wcharlist = 0;
#endif
  while (c = string[i])
    {
#if defined (HANDLE_MULTIBYTE)
      size_t mblength;
#endif
      if ((flags & SX_NOCTLESC) == 0 && c == CTLESC)
	{
	  i += 2;
	  continue;
	}
      /* Even if flags contains SX_NOCTLESC, we let CTLESC quoting CTLNUL
	 through, to protect the CTLNULs from later calls to
	 remove_quoted_nulls. */
      else if ((flags & SX_NOESCCTLNUL) == 0 && c == CTLESC && string[i+1] == CTLNUL)
	{
	  i += 2;
	  continue;
	}

#if defined (HANDLE_MULTIBYTE)
      mblength = MBLEN (string + i, slen - i);
      if (mblength > 1)
	{
	  wchar_t wc;
	  mblength = mbtowc (&wc, string + i, slen - i);
	  if (MB_INVALIDCH (mblength))
	    {
	      if (MEMBER (c, charlist))
		break;
	    }
	  else
	    {
	      if (wcharlist == 0)
		{
		  size_t len;
		  len = mbstowcs (wcharlist, charlist, 0);
		  if (len == -1)
		    len = 0;
		  wcharlist = (wchar_t *)xmalloc (sizeof (wchar_t) * (len + 1));
		  mbstowcs (wcharlist, charlist, len + 1);
		}

	      if (wcschr (wcharlist, wc))
		break;
	    }
	}
      else		
#endif
      if (MEMBER (c, charlist))
	break;

      ADVANCE_CHAR (string, slen, i);
    }

#if defined (HANDLE_MULTIBYTE)
  FREE (wcharlist);
#endif

  temp = substring (string, *sindex, i);
  *sindex = i;

  return (temp);
}

/* Extract the $( construct in STRING, and return a new string.
   Start extracting at (SINDEX) as if we had just seen "$(".
   Make (SINDEX) get the position of the matching ")". )
   XFLAGS is additional flags to pass to other extraction functions. */
char *
extract_command_subst (string, sindex, xflags)
     char *string;
     int *sindex;
     int xflags;
{
  if (string[*sindex] == LPAREN)
    return (extract_delimited_string (string, sindex, "$(", "(", ")", xflags|SX_COMMAND)); /*)*/
  else
    {
      xflags |= (no_longjmp_on_fatal_error ? SX_NOLONGJMP : 0);
      return (xparse_dolparen (string, string+*sindex, sindex, xflags));
    }
}

/* Extract the $[ construct in STRING, and return a new string. (])
   Start extracting at (SINDEX) as if we had just seen "$[".
   Make (SINDEX) get the position of the matching "]". */
char *
extract_arithmetic_subst (string, sindex)
     char *string;
     int *sindex;
{
  return (extract_delimited_string (string, sindex, "$[", "[", "]", 0)); /*]*/
}

#if defined (PROCESS_SUBSTITUTION)
/* Extract the <( or >( construct in STRING, and return a new string.
   Start extracting at (SINDEX) as if we had just seen "<(".
   Make (SINDEX) get the position of the matching ")". */ /*))*/
char *
extract_process_subst (string, starter, sindex)
     char *string;
     char *starter;
     int *sindex;
{
  return (extract_delimited_string (string, sindex, starter, "(", ")", 0));
}
#endif /* PROCESS_SUBSTITUTION */

#if defined (ARRAY_VARS)
/* This can be fooled by unquoted right parens in the passed string. If
   each caller verifies that the last character in STRING is a right paren,
   we don't even need to call extract_delimited_string. */
char *
extract_array_assignment_list (string, sindex)
     char *string;
     int *sindex;
{
  int slen;
  char *ret;

  slen = strlen (string);	/* ( */
  if (string[slen - 1] == ')')
   {
      ret = substring (string, *sindex, slen - 1);
      *sindex = slen - 1;
      return ret;
    }
  return 0;  
}
#endif

/* Extract and create a new string from the contents of STRING, a
   character string delimited with OPENER and CLOSER.  SINDEX is
   the address of an int describing the current offset in STRING;
   it should point to just after the first OPENER found.  On exit,
   SINDEX gets the position of the last character of the matching CLOSER.
   If OPENER is more than a single character, ALT_OPENER, if non-null,
   contains a character string that can also match CLOSER and thus
   needs to be skipped. */
static char *
extract_delimited_string (string, sindex, opener, alt_opener, closer, flags)
     char *string;
     int *sindex;
     char *opener, *alt_opener, *closer;
     int flags;
{
  int i, c, si;
  size_t slen;
  char *t, *result;
  int pass_character, nesting_level, in_comment;
  int len_closer, len_opener, len_alt_opener;
  DECLARE_MBSTATE;

  slen = strlen (string + *sindex) + *sindex;
  len_opener = STRLEN (opener);
  len_alt_opener = STRLEN (alt_opener);
  len_closer = STRLEN (closer);

  pass_character = in_comment = 0;

  nesting_level = 1;
  i = *sindex;

  while (nesting_level)
    {
      c = string[i];

      if (c == 0)
	break;

      if (in_comment)
	{
	  if (c == '\n')
	    in_comment = 0;
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}

      if (pass_character)	/* previous char was backslash */
	{
	  pass_character = 0;
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}

      /* Not exactly right yet; should handle shell metacharacters and
	 multibyte characters, too.  See COMMENT_BEGIN define in parse.y */
      if ((flags & SX_COMMAND) && c == '#' && (i == 0 || string[i - 1] == '\n' || shellblank (string[i - 1])))
	{
          in_comment = 1;
          ADVANCE_CHAR (string, slen, i);
          continue;
	}
        
      if (c == CTLESC || c == '\\')
	{
	  pass_character++;
	  i++;
	  continue;
	}

#if 0
      /* Process a nested command substitution, but only if we're parsing a
	 command substitution.  XXX - for bash-4.2 */
      if ((flags & SX_COMMAND) && string[i] == '$' && string[i+1] == LPAREN)
        {
          si = i + 2;
          t = extract_command_subst (string, &si, flags);
          i = si + 1;
          continue;
        }
#endif

      /* Process a nested OPENER. */
      if (STREQN (string + i, opener, len_opener))
	{
	  si = i + len_opener;
	  t = extract_delimited_string (string, &si, opener, alt_opener, closer, flags|SX_NOALLOC);
	  i = si + 1;
	  continue;
	}

      /* Process a nested ALT_OPENER */
      if (len_alt_opener && STREQN (string + i, alt_opener, len_alt_opener))
	{
	  si = i + len_alt_opener;
	  t = extract_delimited_string (string, &si, alt_opener, alt_opener, closer, flags|SX_NOALLOC);
	  i = si + 1;
	  continue;
	}

      /* If the current substring terminates the delimited string, decrement
	 the nesting level. */
      if (STREQN (string + i, closer, len_closer))
	{
	  i += len_closer - 1;	/* move to last byte of the closer */
	  nesting_level--;
	  if (nesting_level == 0)
	    break;
	}

      /* Pass old-style command substitution through verbatim. */
      if (c == '`')
	{
	  si = i + 1;
	  t = string_extract (string, &si, "`", flags|SX_NOALLOC);
	  i = si + 1;
	  continue;
	}

      /* Pass single-quoted and double-quoted strings through verbatim. */
      if (c == '\'' || c == '"')
	{
	  si = i + 1;
	  i = (c == '\'') ? skip_single_quoted (string, slen, si)
			  : skip_double_quoted (string, slen, si);
	  continue;
	}

      /* move past this character, which was not special. */
      ADVANCE_CHAR (string, slen, i);
    }

  if (c == 0 && nesting_level)
    {
      if (no_longjmp_on_fatal_error == 0)
	{
	  report_error (_("bad substitution: no closing `%s' in %s"), closer, string);
	  last_command_exit_value = EXECUTION_FAILURE;
	  exp_jump_to_top_level (DISCARD);
	}
      else
	{
	  *sindex = i;
	  return (char *)NULL;
	}
    }

  si = i - *sindex - len_closer + 1;
  if (flags & SX_NOALLOC)
    result = (char *)NULL;
  else    
    {
      result = (char *)xmalloc (1 + si);
      strncpy (result, string + *sindex, si);
      result[si] = '\0';
    }
  *sindex = i;

  return (result);
}

/* Extract a parameter expansion expression within ${ and } from STRING.
   Obey the Posix.2 rules for finding the ending `}': count braces while
   skipping over enclosed quoted strings and command substitutions.
   SINDEX is the address of an int describing the current offset in STRING;
   it should point to just after the first `{' found.  On exit, SINDEX
   gets the position of the matching `}'.  QUOTED is non-zero if this
   occurs inside double quotes. */
/* XXX -- this is very similar to extract_delimited_string -- XXX */
static char *
extract_dollar_brace_string (string, sindex, quoted, flags)
     char *string;
     int *sindex, quoted, flags;
{
  register int i, c;
  size_t slen;
  int pass_character, nesting_level, si;
  char *result, *t;
  DECLARE_MBSTATE;

  pass_character = 0;
  nesting_level = 1;
  slen = strlen (string + *sindex) + *sindex;

  i = *sindex;
  while (c = string[i])
    {
      if (pass_character)
	{
	  pass_character = 0;
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}

      /* CTLESCs and backslashes quote the next character. */
      if (c == CTLESC || c == '\\')
	{
	  pass_character++;
	  i++;
	  continue;
	}

      if (string[i] == '$' && string[i+1] == LBRACE)
	{
	  nesting_level++;
	  i += 2;
	  continue;
	}

      if (c == RBRACE)
	{
	  nesting_level--;
	  if (nesting_level == 0)
	    break;
	  i++;
	  continue;
	}

      /* Pass the contents of old-style command substitutions through
	 verbatim. */
      if (c == '`')
	{
	  si = i + 1;
	  t = string_extract (string, &si, "`", flags|SX_NOALLOC);
	  i = si + 1;
	  continue;
	}

      /* Pass the contents of new-style command substitutions and
	 arithmetic substitutions through verbatim. */
      if (string[i] == '$' && string[i+1] == LPAREN)
	{
	  si = i + 2;
	  t = extract_command_subst (string, &si, flags|SX_NOALLOC);
	  i = si + 1;
	  continue;
	}

      /* Pass the contents of single-quoted and double-quoted strings
	 through verbatim. */
      if (c == '\'' || c == '"')
	{
	  si = i + 1;
	  i = (c == '\'') ? skip_single_quoted (string, slen, si)
			  : skip_double_quoted (string, slen, si);
	  /* skip_XXX_quoted leaves index one past close quote */
	  continue;
	}

      /* move past this character, which was not special. */
      ADVANCE_CHAR (string, slen, i);
    }

  if (c == 0 && nesting_level)
    {
      if (no_longjmp_on_fatal_error == 0)
	{			/* { */
	  report_error (_("bad substitution: no closing `%s' in %s"), "}", string);
	  last_command_exit_value = EXECUTION_FAILURE;
	  exp_jump_to_top_level (DISCARD);
	}
      else
	{
	  *sindex = i;
	  return ((char *)NULL);
	}
    }

  result = (flags & SX_NOALLOC) ? (char *)NULL : substring (string, *sindex, i);
  *sindex = i;

  return (result);
}

/* Remove backslashes which are quoting backquotes from STRING.  Modifies
   STRING, and returns a pointer to it. */
char *
de_backslash (string)
     char *string;
{
  register size_t slen;
  register int i, j, prev_i;
  DECLARE_MBSTATE;

  slen = strlen (string);
  i = j = 0;

  /* Loop copying string[i] to string[j], i >= j. */
  while (i < slen)
    {
      if (string[i] == '\\' && (string[i + 1] == '`' || string[i + 1] == '\\' ||
			      string[i + 1] == '$'))
	i++;
      prev_i = i;
      ADVANCE_CHAR (string, slen, i);
      if (j < prev_i)
	do string[j++] = string[prev_i++]; while (prev_i < i);
      else
	j = i;
    }
  string[j] = '\0';

  return (string);
}

#if 0
/*UNUSED*/
/* Replace instances of \! in a string with !. */
void
unquote_bang (string)
     char *string;
{
  register int i, j;
  register char *temp;

  temp = (char *)xmalloc (1 + strlen (string));

  for (i = 0, j = 0; (temp[j] = string[i]); i++, j++)
    {
      if (string[i] == '\\' && string[i + 1] == '!')
	{
	  temp[j] = '!';
	  i++;
	}
    }
  strcpy (string, temp);
  free (temp);
}
#endif

#define CQ_RETURN(x) do { no_longjmp_on_fatal_error = 0; return (x); } while (0)

/* This function assumes s[i] == open; returns with s[ret] == close; used to
   parse array subscripts.  FLAGS & 1 means to not attempt to skip over
   matched pairs of quotes or backquotes, or skip word expansions; it is
   intended to be used after expansion has been performed and during final
   assignment parsing (see arrayfunc.c:assign_compound_array_list()). */
static int
skip_matched_pair (string, start, open, close, flags)
     const char *string;
     int start, open, close, flags;
{
  int i, pass_next, backq, si, c, count;
  size_t slen;
  char *temp, *ss;
  DECLARE_MBSTATE;

  slen = strlen (string + start) + start;
  no_longjmp_on_fatal_error = 1;

  i = start + 1;		/* skip over leading bracket */
  count = 1;
  pass_next = backq = 0;
  ss = (char *)string;
  while (c = string[i])
    {
      if (pass_next)
	{
	  pass_next = 0;
	  if (c == 0)
	    CQ_RETURN(i);
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}
      else if (c == '\\')
	{
	  pass_next = 1;
	  i++;
	  continue;
	}
      else if (backq)
	{
	  if (c == '`')
	    backq = 0;
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}
      else if ((flags & 1) == 0 && c == '`')
	{
	  backq = 1;
	  i++;
	  continue;
	}
      else if ((flags & 1) == 0 && c == open)
	{
	  count++;
	  i++;
	  continue;
	}
      else if (c == close)
	{
	  count--;
	  if (count == 0)
	    break;
	  i++;
	  continue;
	}
      else if ((flags & 1) == 0 && (c == '\'' || c == '"'))
	{
	  i = (c == '\'') ? skip_single_quoted (ss, slen, ++i)
			  : skip_double_quoted (ss, slen, ++i);
	  /* no increment, the skip functions increment past the closing quote. */
	}
      else if ((flags&1) == 0 && c == '$' && (string[i+1] == LPAREN || string[i+1] == LBRACE))
	{
	  si = i + 2;
	  if (string[si] == '\0')
	    CQ_RETURN(si);

	  if (string[i+1] == LPAREN)
	    temp = extract_delimited_string (ss, &si, "$(", "(", ")", SX_NOALLOC|SX_COMMAND); /* ) */
	  else
	    temp = extract_dollar_brace_string (ss, &si, 0, SX_NOALLOC);
	  i = si;
	  if (string[i] == '\0')	/* don't increment i past EOS in loop */
	    break;
	  i++;
	  continue;
	}
      else
	ADVANCE_CHAR (string, slen, i);
    }

  CQ_RETURN(i);
}

#if defined (ARRAY_VARS)
int
skipsubscript (string, start, flags)
     const char *string;
     int start, flags;
{
  return (skip_matched_pair (string, start, '[', ']', flags));
}
#endif

/* Skip characters in STRING until we find a character in DELIMS, and return
   the index of that character.  START is the index into string at which we
   begin.  This is similar in spirit to strpbrk, but it returns an index into
   STRING and takes a starting index.  This little piece of code knows quite
   a lot of shell syntax.  It's very similar to skip_double_quoted and other
   functions of that ilk. */
int
skip_to_delim (string, start, delims, flags)
     char *string;
     int start;
     char *delims;
     int flags;
{
  int i, pass_next, backq, si, c, invert, skipquote, skipcmd;
  size_t slen;
  char *temp;
  DECLARE_MBSTATE;

  slen = strlen (string + start) + start;
  if (flags & SD_NOJMP)
    no_longjmp_on_fatal_error = 1;
  invert = (flags & SD_INVERT);
  skipcmd = (flags & SD_NOSKIPCMD) == 0;

  i = start;
  pass_next = backq = 0;
  while (c = string[i])
    {
      /* If this is non-zero, we should not let quote characters be delimiters
	 and the current character is a single or double quote.  We should not
	 test whether or not it's a delimiter until after we skip single- or
	 double-quoted strings. */
      skipquote = ((flags & SD_NOQUOTEDELIM) && (c == '\'' || c =='"'));
      if (pass_next)
	{
	  pass_next = 0;
	  if (c == 0)
	    CQ_RETURN(i);
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}
      else if (c == '\\')
	{
	  pass_next = 1;
	  i++;
	  continue;
	}
      else if (backq)
	{
	  if (c == '`')
	    backq = 0;
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}
      else if (c == '`')
	{
	  backq = 1;
	  i++;
	  continue;
	}
      else if (skipquote == 0 && invert == 0 && member (c, delims))
	break;
      else if (c == '\'' || c == '"')
	{
	  i = (c == '\'') ? skip_single_quoted (string, slen, ++i)
			  : skip_double_quoted (string, slen, ++i);
	  /* no increment, the skip functions increment past the closing quote. */
	}
      else if (c == '$' && ((skipcmd && string[i+1] == LPAREN) || string[i+1] == LBRACE))
	{
	  si = i + 2;
	  if (string[si] == '\0')
	    CQ_RETURN(si);

	  if (string[i+1] == LPAREN)
	    temp = extract_delimited_string (string, &si, "$(", "(", ")", SX_NOALLOC|SX_COMMAND); /* ) */
	  else
	    temp = extract_dollar_brace_string (string, &si, 0, SX_NOALLOC);
	  i = si;
	  if (string[i] == '\0')	/* don't increment i past EOS in loop */
	    break;
	  i++;
	  continue;
	}
#if defined (PROCESS_SUBSTITUTION)
      else if (skipcmd && (c == '<' || c == '>') && string[i+1] == LPAREN)
	{
	  si = i + 2;
	  if (string[si] == '\0')
	    CQ_RETURN(si);
	  temp = extract_process_subst (string, (c == '<') ? "<(" : ">(", &si);
	  i = si;
	  if (string[i] == '\0')
	    break;
	  i++;
	  continue;
	}
#endif /* PROCESS_SUBSTITUTION */
      else if ((skipquote || invert) && (member (c, delims) == 0))
	break;
      else
	ADVANCE_CHAR (string, slen, i);
    }

  CQ_RETURN(i);
}

#if defined (READLINE)
/* Return 1 if the portion of STRING ending at EINDEX is quoted (there is
   an unclosed quoted string), or if the character at EINDEX is quoted
   by a backslash. NO_LONGJMP_ON_FATAL_ERROR is used to flag that the various
   single and double-quoted string parsing functions should not return an
   error if there are unclosed quotes or braces.  The characters that this
   recognizes need to be the same as the contents of
   rl_completer_quote_characters. */

int
char_is_quoted (string, eindex)
     char *string;
     int eindex;
{
  int i, pass_next, c;
  size_t slen;
  DECLARE_MBSTATE;

  slen = strlen (string);
  no_longjmp_on_fatal_error = 1;
  i = pass_next = 0;
  while (i <= eindex)
    {
      c = string[i];

      if (pass_next)
	{
	  pass_next = 0;
	  if (i >= eindex)	/* XXX was if (i >= eindex - 1) */
	    CQ_RETURN(1);
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}
      else if (c == '\\')
	{
	  pass_next = 1;
	  i++;
	  continue;
	}
      else if (c == '\'' || c == '"')
	{
	  i = (c == '\'') ? skip_single_quoted (string, slen, ++i)
			  : skip_double_quoted (string, slen, ++i);
	  if (i > eindex)
	    CQ_RETURN(1);
	  /* no increment, the skip_xxx functions go one past end */
	}
      else
	ADVANCE_CHAR (string, slen, i);
    }

  CQ_RETURN(0);
}

int
unclosed_pair (string, eindex, openstr)
     char *string;
     int eindex;
     char *openstr;
{
  int i, pass_next, openc, olen;
  size_t slen;
  DECLARE_MBSTATE;

  slen = strlen (string);
  olen = strlen (openstr);
  i = pass_next = openc = 0;
  while (i <= eindex)
    {
      if (pass_next)
	{
	  pass_next = 0;
	  if (i >= eindex)	/* XXX was if (i >= eindex - 1) */
	    return 0;
	  ADVANCE_CHAR (string, slen, i);
	  continue;
	}
      else if (string[i] == '\\')
	{
	  pass_next = 1;
	  i++;
	  continue;
	}
      else if (STREQN (string + i, openstr, olen))
	{
	  openc = 1 - openc;
	  i += olen;
	}
      else if (string[i] == '\'' || string[i] == '"')
	{
	  i = (string[i] == '\'') ? skip_single_quoted (string, slen, i)
				  : skip_double_quoted (string, slen, i);
	  if (i > eindex)
	    return 0;
	}
      else
	ADVANCE_CHAR (string, slen, i);
    }
  return (openc);
}

/* Split STRING (length SLEN) at DELIMS, and return a WORD_LIST with the
   individual words.  If DELIMS is NULL, the current value of $IFS is used
   to split the string, and the function follows the shell field splitting
   rules.  SENTINEL is an index to look for.  NWP, if non-NULL,
   gets the number of words in the returned list.  CWP, if non-NULL, gets
   the index of the word containing SENTINEL.  Non-whitespace chars in
   DELIMS delimit separate fields. */
WORD_LIST *
split_at_delims (string, slen, delims, sentinel, flags, nwp, cwp)
     char *string;
     int slen;
     char *delims;
     int sentinel, flags;
     int *nwp, *cwp;
{
  int ts, te, i, nw, cw, ifs_split, dflags;
  char *token, *d, *d2;
  WORD_LIST *ret, *tl;

  if (string == 0 || *string == '\0')
    {
      if (nwp)
	*nwp = 0;
      if (cwp)
	*cwp = 0;	
      return ((WORD_LIST *)NULL);
    }

  d = (delims == 0) ? ifs_value : delims;
  ifs_split = delims == 0;

  /* Make d2 the non-whitespace characters in delims */
  d2 = 0;
  if (delims)
    {
      size_t slength;
#if defined (HANDLE_MULTIBYTE)
      size_t mblength = 1;
#endif
      DECLARE_MBSTATE;

      slength = strlen (delims);
      d2 = (char *)xmalloc (slength + 1);
      i = ts = 0;
      while (delims[i])
	{
#if defined (HANDLE_MULTIBYTE)
	  mbstate_t state_bak;
	  state_bak = state;
	  mblength = MBRLEN (delims + i, slength, &state);
	  if (MB_INVALIDCH (mblength))
	    state = state_bak;
	  else if (mblength > 1)
	    {
	      memcpy (d2 + ts, delims + i, mblength);
	      ts += mblength;
	      i += mblength;
	      slength -= mblength;
	      continue;
	    }
#endif
	  if (whitespace (delims[i]) == 0)
	    d2[ts++] = delims[i];

	  i++;
	  slength--;
	}
      d2[ts] = '\0';
    }

  ret = (WORD_LIST *)NULL;

  /* Remove sequences of whitespace characters at the start of the string, as
     long as those characters are delimiters. */
  for (i = 0; member (string[i], d) && spctabnl (string[i]); i++)
    ;
  if (string[i] == '\0')
    return (ret);

  ts = i;
  nw = 0;
  cw = -1;
  dflags = flags|SD_NOJMP;
  while (1)
    {
      te = skip_to_delim (string, ts, d, dflags);

      /* If we have a non-whitespace delimiter character, use it to make a
	 separate field.  This is just about what $IFS splitting does and
	 is closer to the behavior of the shell parser. */
      if (ts == te && d2 && member (string[ts], d2))
	{
	  te = ts + 1;
	  /* If we're using IFS splitting, the non-whitespace delimiter char
	     and any additional IFS whitespace delimits a field. */
	  if (ifs_split)
	    while (member (string[te], d) && spctabnl (string[te]))
	      te++;
	  else
	    while (member (string[te], d2))
	      te++;
	}

      token = substring (string, ts, te);

      ret = add_string_to_list (token, ret);
      free (token);
      nw++;

      if (sentinel >= ts && sentinel <= te)
	cw = nw;

      /* If the cursor is at whitespace just before word start, set the
	 sentinel word to the current word. */
      if (cwp && cw == -1 && sentinel == ts-1)
	cw = nw;

      /* If the cursor is at whitespace between two words, make a new, empty
	 word, add it before (well, after, since the list is in reverse order)
	 the word we just added, and set the current word to that one. */
      if (cwp && cw == -1 && sentinel < ts)
	{
	  tl = make_word_list (make_word (""), ret->next);
	  ret->next = tl;
	  cw = nw;
	  nw++;
	}

      if (string[te] == 0)
	break;

      i = te;
      while (member (string[i], d) && (ifs_split || spctabnl(string[i])))
	i++;

      if (string[i])
	ts = i;
      else
	break;
    }

  /* Special case for SENTINEL at the end of STRING.  If we haven't found
     the word containing SENTINEL yet, and the index we're looking for is at
     the end of STRING (or past the end of the previously-found token,
     possible if the end of the line is composed solely of IFS whitespace)
     add an additional null argument and set the current word pointer to that. */
  if (cwp && cw == -1 && (sentinel >= slen || sentinel >= te))
    {
      if (whitespace (string[sentinel - 1]))
	{
	  token = "";
	  ret = add_string_to_list (token, ret);
	  nw++;
	}
      cw = nw;
    }

  if (nwp)
    *nwp = nw;
  if (cwp)
    *cwp = cw;

  return (REVERSE_LIST (ret, WORD_LIST *));
}
#endif /* READLINE */

#if 0
/* UNUSED */
/* Extract the name of the variable to bind to from the assignment string. */
char *
assignment_name (string)
     char *string;
{
  int offset;
  char *temp;

  offset = assignment (string, 0);
  if (offset == 0)
    return (char *)NULL;
  temp = substring (string, 0, offset);
  return (temp);
}
#endif

/* **************************************************************** */
/*								    */
/*     Functions to convert strings to WORD_LISTs and vice versa    */
/*								    */
/* **************************************************************** */

/* Return a single string of all the words in LIST.  SEP is the separator
   to put between individual elements of LIST in the output string. */
char *
string_list_internal (list, sep)
     WORD_LIST *list;
     char *sep;
{
  register WORD_LIST *t;
  char *result, *r;
  int word_len, sep_len, result_size;

  if (list == 0)
    return ((char *)NULL);

  /* Short-circuit quickly if we don't need to separate anything. */
  if (list->next == 0)
    return (savestring (list->word->word));

  /* This is nearly always called with either sep[0] == 0 or sep[1] == 0. */
  sep_len = STRLEN (sep);
  result_size = 0;

  for (t = list; t; t = t->next)
    {
      if (t != list)
	result_size += sep_len;
      result_size += strlen (t->word->word);
    }

  r = result = (char *)xmalloc (result_size + 1);

  for (t = list; t; t = t->next)
    {
      if (t != list && sep_len)
	{
	  if (sep_len > 1)
	    {
	      FASTCOPY (sep, r, sep_len);
	      r += sep_len;
	    }
	  else
	    *r++ = sep[0];
	}

      word_len = strlen (t->word->word);
      FASTCOPY (t->word->word, r, word_len);
      r += word_len;
    }

  *r = '\0';
  return (result);
}

/* Return a single string of all the words present in LIST, separating
   each word with a space. */
char *
string_list (list)
     WORD_LIST *list;
{
  return (string_list_internal (list, " "));
}

/* An external interface that can be used by the rest of the shell to
   obtain a string containing the first character in $IFS.  Handles all
   the multibyte complications.  If LENP is non-null, it is set to the
   length of the returned string. */
char *
ifs_firstchar (lenp)
     int *lenp;
{
  char *ret;
  int len;

  ret = xmalloc (MB_LEN_MAX + 1);
#if defined (HANDLE_MULTIBYTE)
  if (ifs_firstc_len == 1)
    {
      ret[0] = ifs_firstc[0];
      ret[1] = '\0';
      len = ret[0] ? 1 : 0;
    }
  else
    {
      memcpy (ret, ifs_firstc, ifs_firstc_len);
      ret[len = ifs_firstc_len] = '\0';
    }
#else
  ret[0] = ifs_firstc;
  ret[1] = '\0';
  len = ret[0] ? 0 : 1;
#endif

  if (lenp)
    *lenp = len;

  return ret;
}

/* Return a single string of all the words present in LIST, obeying the
   quoting rules for "$*", to wit: (P1003.2, draft 11, 3.5.2) "If the
   expansion [of $*] appears within a double quoted string, it expands
   to a single field with the value of each parameter separated by the
   first character of the IFS variable, or by a <space> if IFS is unset." */
char *
string_list_dollar_star (list)
     WORD_LIST *list;
{
  char *ret;
#if defined (HANDLE_MULTIBYTE)
#  if defined (__GNUC__)
  char sep[MB_CUR_MAX + 1];
#  else
  char *sep = 0;
#  endif
#else
  char sep[2];
#endif

#if defined (HANDLE_MULTIBYTE)
#  if !defined (__GNUC__)
  sep = (char *)xmalloc (MB_CUR_MAX + 1);
#  endif /* !__GNUC__ */
  if (ifs_firstc_len == 1)
    {
      sep[0] = ifs_firstc[0];
      sep[1] = '\0';
    }
  else
    {
      memcpy (sep, ifs_firstc, ifs_firstc_len);
      sep[ifs_firstc_len] = '\0';
    }
#else
  sep[0] = ifs_firstc;
  sep[1] = '\0';
#endif

  ret = string_list_internal (list, sep);
#if defined (HANDLE_MULTIBYTE) && !defined (__GNUC__)
  free (sep);
#endif
  return ret;
}

/* Turn $@ into a string.  If (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
   is non-zero, the $@ appears within double quotes, and we should quote
   the list before converting it into a string.  If IFS is unset, and the
   word is not quoted, we just need to quote CTLESC and CTLNUL characters
   in the words in the list, because the default value of $IFS is
   <space><tab><newline>, IFS characters in the words in the list should
   also be split.  If IFS is null, and the word is not quoted, we need
   to quote the words in the list to preserve the positional parameters
   exactly. */
char *
string_list_dollar_at (list, quoted)
     WORD_LIST *list;
     int quoted;
{
  char *ifs, *ret;
#if defined (HANDLE_MULTIBYTE)
#  if defined (__GNUC__)
  char sep[MB_CUR_MAX + 1];
#  else
  char *sep = 0;
#  endif /* !__GNUC__ */
#else
  char sep[2];
#endif
  WORD_LIST *tlist;

  /* XXX this could just be ifs = ifs_value; */
  ifs = ifs_var ? value_cell (ifs_var) : (char *)0;

#if defined (HANDLE_MULTIBYTE)
#  if !defined (__GNUC__)
  sep = (char *)xmalloc (MB_CUR_MAX + 1);
#  endif /* !__GNUC__ */
  if (ifs && *ifs)
    {
      if (ifs_firstc_len == 1)
	{
	  sep[0] = ifs_firstc[0];
	  sep[1] = '\0';
	}
      else
	{
	  memcpy (sep, ifs_firstc, ifs_firstc_len);
	  sep[ifs_firstc_len] = '\0';
	}
    }
  else
    {
      sep[0] = ' ';
      sep[1] = '\0';
    }
#else
  sep[0] = (ifs == 0 || *ifs == 0) ? ' ' : *ifs;
  sep[1] = '\0';
#endif

  /* XXX -- why call quote_list if ifs == 0?  we can get away without doing
     it now that quote_escapes quotes spaces */
#if 0
  tlist = ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) || (ifs && *ifs == 0))
#else
  tlist = (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES|Q_PATQUOTE))
#endif
		? quote_list (list)
		: list_quote_escapes (list);

  ret = string_list_internal (tlist, sep);
#if defined (HANDLE_MULTIBYTE) && !defined (__GNUC__)
  free (sep);
#endif
  return ret;
}

/* Turn the positional paramters into a string, understanding quoting and
   the various subtleties of using the first character of $IFS as the
   separator.  Calls string_list_dollar_at, string_list_dollar_star, and
   string_list as appropriate. */
char *
string_list_pos_params (pchar, list, quoted)
     int pchar;
     WORD_LIST *list;
     int quoted;
{
  char *ret;
  WORD_LIST *tlist;

  if (pchar == '*' && (quoted & Q_DOUBLE_QUOTES))
    {
      tlist = quote_list (list);
      word_list_remove_quoted_nulls (tlist);
      ret = string_list_dollar_star (tlist);
    }
  else if (pchar == '*' && (quoted & Q_HERE_DOCUMENT))
    {
      tlist = quote_list (list);
      word_list_remove_quoted_nulls (tlist);
      ret = string_list (tlist);
    }
  else if (pchar == '*')
    {
      /* Even when unquoted, string_list_dollar_star does the right thing
	 making sure that the first character of $IFS is used as the
	 separator. */
      ret = string_list_dollar_star (list);
    }
  else if (pchar == '@' && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
    /* We use string_list_dollar_at, but only if the string is quoted, since
       that quotes the escapes if it's not, which we don't want.  We could
       use string_list (the old code did), but that doesn't do the right
       thing if the first character of $IFS is not a space.  We use
       string_list_dollar_star if the string is unquoted so we make sure that
       the elements of $@ are separated by the first character of $IFS for
       later splitting. */
    ret = string_list_dollar_at (list, quoted);
  else if (pchar == '@')
    ret = string_list_dollar_star (list);
  else
    ret = string_list ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) ? quote_list (list) : list);

  return ret;
}

/* Return the list of words present in STRING.  Separate the string into
   words at any of the characters found in SEPARATORS.  If QUOTED is
   non-zero then word in the list will have its quoted flag set, otherwise
   the quoted flag is left as make_word () deemed fit.

   This obeys the P1003.2 word splitting semantics.  If `separators' is
   exactly <space><tab><newline>, then the splitting algorithm is that of
   the Bourne shell, which treats any sequence of characters from `separators'
   as a delimiter.  If IFS is unset, which results in `separators' being set
   to "", no splitting occurs.  If separators has some other value, the
   following rules are applied (`IFS white space' means zero or more
   occurrences of <space>, <tab>, or <newline>, as long as those characters
   are in `separators'):

	1) IFS white space is ignored at the start and the end of the
	   string.
	2) Each occurrence of a character in `separators' that is not
	   IFS white space, along with any adjacent occurrences of
	   IFS white space delimits a field.
	3) Any nonzero-length sequence of IFS white space delimits a field.
   */

/* BEWARE!  list_string strips null arguments.  Don't call it twice and
   expect to have "" preserved! */

/* This performs word splitting and quoted null character removal on
   STRING. */
#define issep(c) \
	(((separators)[0]) ? ((separators)[1] ? isifs(c) \
					      : (c) == (separators)[0]) \
			   : 0)

WORD_LIST *
list_string (string, separators, quoted)
     register char *string, *separators;
     int quoted;
{
  WORD_LIST *result;
  WORD_DESC *t;
  char *current_word, *s;
  int sindex, sh_style_split, whitesep, xflags;
  size_t slen;

  if (!string || !*string)
    return ((WORD_LIST *)NULL);

  sh_style_split = separators && separators[0] == ' ' &&
				 separators[1] == '\t' &&
				 separators[2] == '\n' &&
				 separators[3] == '\0';
  for (xflags = 0, s = ifs_value; s && *s; s++)
    {
      if (*s == CTLESC) xflags |= SX_NOCTLESC;
      else if (*s == CTLNUL) xflags |= SX_NOESCCTLNUL;
    }

  slen = 0;
  /* Remove sequences of whitespace at the beginning of STRING, as
     long as those characters appear in IFS.  Do not do this if
     STRING is quoted or if there are no separator characters. */
  if (!quoted || !separators || !*separators)
    {
      for (s = string; *s && spctabnl (*s) && issep (*s); s++);

      if (!*s)
	return ((WORD_LIST *)NULL);

      string = s;
    }

  /* OK, now STRING points to a word that does not begin with white space.
     The splitting algorithm is:
	extract a word, stopping at a separator
	skip sequences of spc, tab, or nl as long as they are separators
     This obeys the field splitting rules in Posix.2. */
  slen = (MB_CUR_MAX > 1) ? strlen (string) : 1;
  for (result = (WORD_LIST *)NULL, sindex = 0; string[sindex]; )
    {
      /* Don't need string length in ADVANCE_CHAR or string_extract_verbatim
	 unless multibyte chars are possible. */
      current_word = string_extract_verbatim (string, slen, &sindex, separators, xflags);
      if (current_word == 0)
	break;

      /* If we have a quoted empty string, add a quoted null argument.  We
	 want to preserve the quoted null character iff this is a quoted
	 empty string; otherwise the quoted null characters are removed
	 below. */
      if (QUOTED_NULL (current_word))
	{
	  t = alloc_word_desc ();
	  t->word = make_quoted_char ('\0');
	  t->flags |= W_QUOTED|W_HASQUOTEDNULL;
	  result = make_word_list (t, result);
	}
      else if (current_word[0] != '\0')
	{
	  /* If we have something, then add it regardless.  However,
	     perform quoted null character removal on the current word. */
	  remove_quoted_nulls (current_word);
	  result = add_string_to_list (current_word, result);
	  result->word->flags &= ~W_HASQUOTEDNULL;	/* just to be sure */
	  if (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT))
	    result->word->flags |= W_QUOTED;
	}

      /* If we're not doing sequences of separators in the traditional
	 Bourne shell style, then add a quoted null argument. */
      else if (!sh_style_split && !spctabnl (string[sindex]))
	{
	  t = alloc_word_desc ();
	  t->word = make_quoted_char ('\0');
	  t->flags |= W_QUOTED|W_HASQUOTEDNULL;
	  result = make_word_list (t, result);
	}

      free (current_word);

      /* Note whether or not the separator is IFS whitespace, used later. */
      whitesep = string[sindex] && spctabnl (string[sindex]);

      /* Move past the current separator character. */
      if (string[sindex])
	{
	  DECLARE_MBSTATE;
	  ADVANCE_CHAR (string, slen, sindex);
	}

      /* Now skip sequences of space, tab, or newline characters if they are
	 in the list of separators. */
      while (string[sindex] && spctabnl (string[sindex]) && issep (string[sindex]))
	sindex++;

      /* If the first separator was IFS whitespace and the current character
	 is a non-whitespace IFS character, it should be part of the current
	 field delimiter, not a separate delimiter that would result in an
	 empty field.  Look at POSIX.2, 3.6.5, (3)(b). */
      if (string[sindex] && whitesep && issep (string[sindex]) && !spctabnl (string[sindex]))
	{
	  sindex++;
	  /* An IFS character that is not IFS white space, along with any
	     adjacent IFS white space, shall delimit a field. (SUSv3) */
	  while (string[sindex] && spctabnl (string[sindex]) && isifs (string[sindex]))
	    sindex++;
	}
    }
  return (REVERSE_LIST (result, WORD_LIST *));
}

/* Parse a single word from STRING, using SEPARATORS to separate fields.
   ENDPTR is set to the first character after the word.  This is used by
   the `read' builtin.  This is never called with SEPARATORS != $IFS;
   it should be simplified.

   XXX - this function is very similar to list_string; they should be
	 combined - XXX */
char *
get_word_from_string (stringp, separators, endptr)
     char **stringp, *separators, **endptr;
{
  register char *s;
  char *current_word;
  int sindex, sh_style_split, whitesep, xflags;
  size_t slen;

  if (!stringp || !*stringp || !**stringp)
    return ((char *)NULL);

  sh_style_split = separators && separators[0] == ' ' &&
				 separators[1] == '\t' &&
				 separators[2] == '\n' &&
				 separators[3] == '\0';
  for (xflags = 0, s = ifs_value; s && *s; s++)
    {
      if (*s == CTLESC) xflags |= SX_NOCTLESC;
      if (*s == CTLNUL) xflags |= SX_NOESCCTLNUL;
    }

  s = *stringp;
  slen = 0;

  /* Remove sequences of whitespace at the beginning of STRING, as
     long as those characters appear in IFS. */
  if (sh_style_split || !separators || !*separators)
    {
      for (; *s && spctabnl (*s) && isifs (*s); s++);

      /* If the string is nothing but whitespace, update it and return. */
      if (!*s)
	{
	  *stringp = s;
	  if (endptr)
	    *endptr = s;
	  return ((char *)NULL);
	}
    }

  /* OK, S points to a word that does not begin with white space.
     Now extract a word, stopping at a separator, save a pointer to
     the first character after the word, then skip sequences of spc,
     tab, or nl as long as they are separators.

     This obeys the field splitting rules in Posix.2. */
  sindex = 0;
  /* Don't need string length in ADVANCE_CHAR or string_extract_verbatim
     unless multibyte chars are possible. */
  slen = (MB_CUR_MAX > 1) ? strlen (s) : 1;
  current_word = string_extract_verbatim (s, slen, &sindex, separators, xflags);

  /* Set ENDPTR to the first character after the end of the word. */
  if (endptr)
    *endptr = s + sindex;

  /* Note whether or not the separator is IFS whitespace, used later. */
  whitesep = s[sindex] && spctabnl (s[sindex]);

  /* Move past the current separator character. */
  if (s[sindex])
    {
      DECLARE_MBSTATE;
      ADVANCE_CHAR (s, slen, sindex);
    }

  /* Now skip sequences of space, tab, or newline characters if they are
     in the list of separators. */
  while (s[sindex] && spctabnl (s[sindex]) && isifs (s[sindex]))
    sindex++;

  /* If the first separator was IFS whitespace and the current character is
     a non-whitespace IFS character, it should be part of the current field
     delimiter, not a separate delimiter that would result in an empty field.
     Look at POSIX.2, 3.6.5, (3)(b). */
  if (s[sindex] && whitesep && isifs (s[sindex]) && !spctabnl (s[sindex]))
    {
      sindex++;
      /* An IFS character that is not IFS white space, along with any adjacent
	 IFS white space, shall delimit a field. */
      while (s[sindex] && spctabnl (s[sindex]) && isifs (s[sindex]))
	sindex++;
    }

  /* Update STRING to point to the next field. */
  *stringp = s + sindex;
  return (current_word);
}

/* Remove IFS white space at the end of STRING.  Start at the end
   of the string and walk backwards until the beginning of the string
   or we find a character that's not IFS white space and not CTLESC.
   Only let CTLESC escape a white space character if SAW_ESCAPE is
   non-zero.  */
char *
strip_trailing_ifs_whitespace (string, separators, saw_escape)
     char *string, *separators;
     int saw_escape;
{
  char *s;

  s = string + STRLEN (string) - 1;
  while (s > string && ((spctabnl (*s) && isifs (*s)) ||
			(saw_escape && *s == CTLESC && spctabnl (s[1]))))
    s--;
  *++s = '\0';
  return string;
}

#if 0
/* UNUSED */
/* Split STRING into words at whitespace.  Obeys shell-style quoting with
   backslashes, single and double quotes. */
WORD_LIST *
list_string_with_quotes (string)
     char *string;
{
  WORD_LIST *list;
  char *token, *s;
  size_t s_len;
  int c, i, tokstart, len;

  for (s = string; s && *s && spctabnl (*s); s++)
    ;
  if (s == 0 || *s == 0)
    return ((WORD_LIST *)NULL);

  s_len = strlen (s);
  tokstart = i = 0;
  list = (WORD_LIST *)NULL;
  while (1)
    {
      c = s[i];
      if (c == '\\')
	{
	  i++;
	  if (s[i])
	    i++;
	}
      else if (c == '\'')
	i = skip_single_quoted (s, s_len, ++i);
      else if (c == '"')
	i = skip_double_quoted (s, s_len, ++i);
      else if (c == 0 || spctabnl (c))
	{
	  /* We have found the end of a token.  Make a word out of it and
	     add it to the word list. */
	  token = substring (s, tokstart, i);
	  list = add_string_to_list (token, list);
	  free (token);
	  while (spctabnl (s[i]))
	    i++;
	  if (s[i])
	    tokstart = i;
	  else
	    break;
	}
      else
	i++;	/* normal character */
    }
  return (REVERSE_LIST (list, WORD_LIST *));
}
#endif

/********************************************************/
/*							*/
/*	Functions to perform assignment statements	*/
/*							*/
/********************************************************/

#if defined (ARRAY_VARS)
static SHELL_VAR *
do_compound_assignment (name, value, flags)
     char *name, *value;
     int flags;
{
  SHELL_VAR *v;
  int mklocal, mkassoc;
  WORD_LIST *list;

  mklocal = flags & ASS_MKLOCAL;
  mkassoc = flags & ASS_MKASSOC;

  if (mklocal && variable_context)
    {
      v = find_variable (name);
      list = expand_compound_array_assignment (v, value, flags);
      if (mkassoc)
	v = make_local_assoc_variable (name);
      else if (v == 0 || (array_p (v) == 0 && assoc_p (v) == 0) || v->context != variable_context)
        v = make_local_array_variable (name);
      assign_compound_array_list (v, list, flags);
    }
  else
    v = assign_array_from_string (name, value, flags);

  return (v);
}
#endif

/* Given STRING, an assignment string, get the value of the right side
   of the `=', and bind it to the left side.  If EXPAND is true, then
   perform parameter expansion, command substitution, and arithmetic
   expansion on the right-hand side.  Perform tilde expansion in any
   case.  Do not perform word splitting on the result of expansion. */
static int
do_assignment_internal (word, expand)
     const WORD_DESC *word;
     int expand;
{
  int offset, tlen, appendop, assign_list, aflags, retval;
  char *name, *value;
  SHELL_VAR *entry;
#if defined (ARRAY_VARS)
  char *t;
  int ni;
#endif
  const char *string;

  if (word == 0 || word->word == 0)
    return 0;

  appendop = assign_list = aflags = 0;
  string = word->word;
  offset = assignment (string, 0);
  name = savestring (string);
  value = (char *)NULL;

  if (name[offset] == '=')
    {
      char *temp;

      if (name[offset - 1] == '+')
	{
	  appendop = 1;
	  name[offset - 1] = '\0';
	}

      name[offset] = 0;		/* might need this set later */
      temp = name + offset + 1;
      tlen = STRLEN (temp);

#if defined (ARRAY_VARS)
      if (expand && (word->flags & W_COMPASSIGN))
	{
	  assign_list = ni = 1;
	  value = extract_array_assignment_list (temp, &ni);
	}
      else
#endif
      if (expand && temp[0])
	value = expand_string_if_necessary (temp, 0, expand_string_assignment);
      else
	value = savestring (temp);
    }

  if (value == 0)
    {
      value = (char *)xmalloc (1);
      value[0] = '\0';
    }

  if (echo_command_at_execute)
    {
      if (appendop)
	name[offset - 1] = '+';
      xtrace_print_assignment (name, value, assign_list, 1);
      if (appendop)
	name[offset - 1] = '\0';
    }

#define ASSIGN_RETURN(r)	do { FREE (value); free (name); return (r); } while (0)

  if (appendop)
    aflags |= ASS_APPEND;

#if defined (ARRAY_VARS)
  if (t = mbschr (name, '['))	/*]*/
    {
      if (assign_list)
	{
	  report_error (_("%s: cannot assign list to array member"), name);
	  ASSIGN_RETURN (0);
	}
      entry = assign_array_element (name, value, aflags);
      if (entry == 0)
	ASSIGN_RETURN (0);
    }
  else if (assign_list)
    {
      if (word->flags & W_ASSIGNARG)
	aflags |= ASS_MKLOCAL;
      if (word->flags & W_ASSIGNASSOC)
	aflags |= ASS_MKASSOC;
      entry = do_compound_assignment (name, value, aflags);
    }
  else
#endif /* ARRAY_VARS */
  entry = bind_variable (name, value, aflags);

  stupidly_hack_special_variables (name);

#if 1
  /* Return 1 if the assignment seems to have been performed correctly. */
  if (entry == 0 || readonly_p (entry))
    retval = 0;		/* assignment failure */
  else if (noassign_p (entry))
    {
      last_command_exit_value = EXECUTION_FAILURE;
      retval = 1;	/* error status, but not assignment failure */
    }
  else
    retval = 1;

  if (entry && retval != 0 && noassign_p (entry) == 0)
    VUNSETATTR (entry, att_invisible);

  ASSIGN_RETURN (retval);
#else
  if (entry)
    VUNSETATTR (entry, att_invisible);

  ASSIGN_RETURN (entry ? ((readonly_p (entry) == 0) && noassign_p (entry) == 0) : 0);
#endif
}

/* Perform the assignment statement in STRING, and expand the
   right side by doing tilde, command and parameter expansion. */
int
do_assignment (string)
     char *string;
{
  WORD_DESC td;

  td.flags = W_ASSIGNMENT;
  td.word = string;

  return do_assignment_internal (&td, 1);
}

int
do_word_assignment (word)
     WORD_DESC *word;
{
  return do_assignment_internal (word, 1);
}

/* Given STRING, an assignment string, get the value of the right side
   of the `=', and bind it to the left side.  Do not perform any word
   expansions on the right hand side. */
int
do_assignment_no_expand (string)
     char *string;
{
  WORD_DESC td;

  td.flags = W_ASSIGNMENT;
  td.word = string;

  return (do_assignment_internal (&td, 0));
}

/***************************************************
 *						   *
 *  Functions to manage the positional parameters  *
 *						   *
 ***************************************************/

/* Return the word list that corresponds to `$*'. */
WORD_LIST *
list_rest_of_args ()
{
  register WORD_LIST *list, *args;
  int i;

  /* Break out of the loop as soon as one of the dollar variables is null. */
  for (i = 1, list = (WORD_LIST *)NULL; i < 10 && dollar_vars[i]; i++)
    list = make_word_list (make_bare_word (dollar_vars[i]), list);

  for (args = rest_of_args; args; args = args->next)
    list = make_word_list (make_bare_word (args->word->word), list);

  return (REVERSE_LIST (list, WORD_LIST *));
}

int
number_of_args ()
{
  register WORD_LIST *list;
  int n;

  for (n = 0; n < 9 && dollar_vars[n+1]; n++)
    ;
  for (list = rest_of_args; list; list = list->next)
    n++;
  return n;
}

/* Return the value of a positional parameter.  This handles values > 10. */
char *
get_dollar_var_value (ind)
     intmax_t ind;
{
  char *temp;
  WORD_LIST *p;

  if (ind < 10)
    temp = dollar_vars[ind] ? savestring (dollar_vars[ind]) : (char *)NULL;
  else	/* We want something like ${11} */
    {
      ind -= 10;
      for (p = rest_of_args; p && ind--; p = p->next)
	;
      temp = p ? savestring (p->word->word) : (char *)NULL;
    }
  return (temp);
}

/* Make a single large string out of the dollar digit variables,
   and the rest_of_args.  If DOLLAR_STAR is 1, then obey the special
   case of "$*" with respect to IFS. */
char *
string_rest_of_args (dollar_star)
     int dollar_star;
{
  register WORD_LIST *list;
  char *string;

  list = list_rest_of_args ();
  string = dollar_star ? string_list_dollar_star (list) : string_list (list);
  dispose_words (list);
  return (string);
}

/* Return a string containing the positional parameters from START to
   END, inclusive.  If STRING[0] == '*', we obey the rules for $*,
   which only makes a difference if QUOTED is non-zero.  If QUOTED includes
   Q_HERE_DOCUMENT or Q_DOUBLE_QUOTES, this returns a quoted list, otherwise
   no quoting chars are added. */
static char *
pos_params (string, start, end, quoted)
     char *string;
     int start, end, quoted;
{
  WORD_LIST *save, *params, *h, *t;
  char *ret;
  int i;

  /* see if we can short-circuit.  if start == end, we want 0 parameters. */
  if (start == end)
    return ((char *)NULL);

  save = params = list_rest_of_args ();
  if (save == 0)
    return ((char *)NULL);

  if (start == 0)		/* handle ${@:0[:x]} specially */
    {
      t = make_word_list (make_word (dollar_vars[0]), params);
      save = params = t;
    }

  for (i = start ? 1 : 0; params && i < start; i++)
    params = params->next;
  if (params == 0)
    return ((char *)NULL);
  for (h = t = params; params && i < end; i++)
    {
      t = params;
      params = params->next;
    }

  t->next = (WORD_LIST *)NULL;

  ret = string_list_pos_params (string[0], h, quoted);

  if (t != params)
    t->next = params;

  dispose_words (save);
  return (ret);
}

/******************************************************************/
/*								  */
/*	Functions to expand strings to strings or WORD_LISTs      */
/*								  */
/******************************************************************/

#if defined (PROCESS_SUBSTITUTION)
#define EXP_CHAR(s) (s == '$' || s == '`' || s == '<' || s == '>' || s == CTLESC || s == '~')
#else
#define EXP_CHAR(s) (s == '$' || s == '`' || s == CTLESC || s == '~')
#endif

/* If there are any characters in STRING that require full expansion,
   then call FUNC to expand STRING; otherwise just perform quote
   removal if necessary.  This returns a new string. */
static char *
expand_string_if_necessary (string, quoted, func)
     char *string;
     int quoted;
     EXPFUNC *func;
{
  WORD_LIST *list;
  size_t slen;
  int i, saw_quote;
  char *ret;
  DECLARE_MBSTATE;

  /* Don't need string length for ADVANCE_CHAR unless multibyte chars possible. */
  slen = (MB_CUR_MAX > 1) ? strlen (string) : 0;
  i = saw_quote = 0;
  while (string[i])
    {
      if (EXP_CHAR (string[i]))
	break;
      else if (string[i] == '\'' || string[i] == '\\' || string[i] == '"')
	saw_quote = 1;
      ADVANCE_CHAR (string, slen, i);
    }

  if (string[i])
    {
      list = (*func) (string, quoted);
      if (list)
	{
	  ret = string_list (list);
	  dispose_words (list);
	}
      else
	ret = (char *)NULL;
    }
  else if (saw_quote && ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) == 0))
    ret = string_quote_removal (string, quoted);
  else
    ret = savestring (string);

  return ret;
}

static inline char *
expand_string_to_string_internal (string, quoted, func)
     char *string;
     int quoted;
     EXPFUNC *func;
{
  WORD_LIST *list;
  char *ret;

  if (string == 0 || *string == '\0')
    return ((char *)NULL);

  list = (*func) (string, quoted);
  if (list)
    {
      ret = string_list (list);
      dispose_words (list);
    }
  else
    ret = (char *)NULL;

  return (ret);
}

char *
expand_string_to_string (string, quoted)
     char *string;
     int quoted;
{
  return (expand_string_to_string_internal (string, quoted, expand_string));
}

char *
expand_string_unsplit_to_string (string, quoted)
     char *string;
     int quoted;
{
  return (expand_string_to_string_internal (string, quoted, expand_string_unsplit));
}

char *
expand_assignment_string_to_string (string, quoted)
     char *string;
     int quoted;
{
  return (expand_string_to_string_internal (string, quoted, expand_string_assignment));
}

char *
expand_arith_string (string, quoted)
     char *string;
     int quoted;
{
  return (expand_string_if_necessary (string, quoted, expand_string));
}

#if defined (COND_COMMAND)
/* Just remove backslashes in STRING.  Returns a new string. */
char *
remove_backslashes (string)
     char *string;
{
  char *r, *ret, *s;

  r = ret = (char *)xmalloc (strlen (string) + 1);
  for (s = string; s && *s; )
    {
      if (*s == '\\')
	s++;
      if (*s == 0)
	break;
      *r++ = *s++;
    }
  *r = '\0';
  return ret;
}

/* This needs better error handling. */
/* Expand W for use as an argument to a unary or binary operator in a
   [[...]] expression.  If SPECIAL is 1, this is the rhs argument
   to the != or == operator, and should be treated as a pattern.  In
   this case, we quote the string specially for the globbing code.  If
   SPECIAL is 2, this is an rhs argument for the =~ operator, and should
   be quoted appropriately for regcomp/regexec.  The caller is responsible
   for removing the backslashes if the unquoted word is needed later. */   
char *
cond_expand_word (w, special)
     WORD_DESC *w;
     int special;
{
  char *r, *p;
  WORD_LIST *l;
  int qflags;

  if (w->word == 0 || w->word[0] == '\0')
    return ((char *)NULL);

  w->flags |= W_NOSPLIT2;
  l = call_expand_word_internal (w, 0, 0, (int *)0, (int *)0);
  if (l)
    {
      if (special == 0)
	{
	  dequote_list (l);
	  r = string_list (l);
	}
      else
	{
	  qflags = QGLOB_CVTNULL;
	  if (special == 2)
	    qflags |= QGLOB_REGEXP;
	  p = string_list (l);
	  r = quote_string_for_globbing (p, qflags);
	  free (p);
	}
      dispose_words (l);
    }
  else
    r = (char *)NULL;

  return r;
}
#endif

/* Call expand_word_internal to expand W and handle error returns.
   A convenience function for functions that don't want to handle
   any errors or free any memory before aborting. */
static WORD_LIST *
call_expand_word_internal (w, q, i, c, e)
     WORD_DESC *w;
     int q, i, *c, *e;
{
  WORD_LIST *result;

  result = expand_word_internal (w, q, i, c, e);
  if (result == &expand_word_error || result == &expand_word_fatal)
    {
      /* By convention, each time this error is returned, w->word has
	 already been freed (it sometimes may not be in the fatal case,
	 but that doesn't result in a memory leak because we're going
	 to exit in most cases). */
      w->word = (char *)NULL;
      last_command_exit_value = EXECUTION_FAILURE;
      exp_jump_to_top_level ((result == &expand_word_error) ? DISCARD : FORCE_EOF);
      /* NOTREACHED */
    }
  else
    return (result);
}

/* Perform parameter expansion, command substitution, and arithmetic
   expansion on STRING, as if it were a word.  Leave the result quoted. */
static WORD_LIST *
expand_string_internal (string, quoted)
     char *string;
     int quoted;
{
  WORD_DESC td;
  WORD_LIST *tresult;

  if (string == 0 || *string == 0)
    return ((WORD_LIST *)NULL);

  td.flags = 0;
  td.word = savestring (string);

  tresult = call_expand_word_internal (&td, quoted, 0, (int *)NULL, (int *)NULL);

  FREE (td.word);
  return (tresult);
}

/* Expand STRING by performing parameter expansion, command substitution,
   and arithmetic expansion.  Dequote the resulting WORD_LIST before
   returning it, but do not perform word splitting.  The call to
   remove_quoted_nulls () is in here because word splitting normally
   takes care of quote removal. */
WORD_LIST *
expand_string_unsplit (string, quoted)
     char *string;
     int quoted;
{
  WORD_LIST *value;

  if (string == 0 || *string == '\0')
    return ((WORD_LIST *)NULL);

  expand_no_split_dollar_star = 1;
  value = expand_string_internal (string, quoted);
  expand_no_split_dollar_star = 0;

  if (value)
    {
      if (value->word)
	{
	  remove_quoted_nulls (value->word->word);
	  value->word->flags &= ~W_HASQUOTEDNULL;
	}
      dequote_list (value);
    }
  return (value);
}

/* Expand the rhs of an assignment statement */
WORD_LIST *
expand_string_assignment (string, quoted)
     char *string;
     int quoted;
{
  WORD_DESC td;
  WORD_LIST *value;

  if (string == 0 || *string == '\0')
    return ((WORD_LIST *)NULL);

  expand_no_split_dollar_star = 1;

  td.flags = W_ASSIGNRHS;
  td.word = savestring (string);
  value = call_expand_word_internal (&td, quoted, 0, (int *)NULL, (int *)NULL);
  FREE (td.word);

  expand_no_split_dollar_star = 0;

  if (value)
    {
      if (value->word)
	{
	  remove_quoted_nulls (value->word->word);
	  value->word->flags &= ~W_HASQUOTEDNULL;
	}
      dequote_list (value);
    }
  return (value);
}


/* Expand one of the PS? prompt strings. This is a sort of combination of
   expand_string_unsplit and expand_string_internal, but returns the
   passed string when an error occurs.  Might want to trap other calls
   to jump_to_top_level here so we don't endlessly loop. */
WORD_LIST *
expand_prompt_string (string, quoted, wflags)
     char *string;
     int quoted;
     int wflags;
{
  WORD_LIST *value;
  WORD_DESC td;

  if (string == 0 || *string == 0)
    return ((WORD_LIST *)NULL);

  td.flags = wflags;
  td.word = savestring (string);

  no_longjmp_on_fatal_error = 1;
  value = expand_word_internal (&td, quoted, 0, (int *)NULL, (int *)NULL);
  no_longjmp_on_fatal_error = 0;

  if (value == &expand_word_error || value == &expand_word_fatal)
    {
      value = make_word_list (make_bare_word (string), (WORD_LIST *)NULL);
      return value;
    }
  FREE (td.word);
  if (value)
    {
      if (value->word)
	{
	  remove_quoted_nulls (value->word->word);
	  value->word->flags &= ~W_HASQUOTEDNULL;
	}
      dequote_list (value);
    }
  return (value);
}

/* Expand STRING just as if you were expanding a word, but do not dequote
   the resultant WORD_LIST.  This is called only from within this file,
   and is used to correctly preserve quoted characters when expanding
   things like ${1+"$@"}.  This does parameter expansion, command
   substitution, arithmetic expansion, and word splitting. */
static WORD_LIST *
expand_string_leave_quoted (string, quoted)
     char *string;
     int quoted;
{
  WORD_LIST *tlist;
  WORD_LIST *tresult;

  if (string == 0 || *string == '\0')
    return ((WORD_LIST *)NULL);

  tlist = expand_string_internal (string, quoted);

  if (tlist)
    {
      tresult = word_list_split (tlist);
      dispose_words (tlist);
      return (tresult);
    }
  return ((WORD_LIST *)NULL);
}

/* This does not perform word splitting or dequote the WORD_LIST
   it returns. */
static WORD_LIST *
expand_string_for_rhs (string, quoted, dollar_at_p, has_dollar_at)
     char *string;
     int quoted, *dollar_at_p, *has_dollar_at;
{
  WORD_DESC td;
  WORD_LIST *tresult;

  if (string == 0 || *string == '\0')
    return (WORD_LIST *)NULL;

  td.flags = 0;
  td.word = string;
  tresult = call_expand_word_internal (&td, quoted, 1, dollar_at_p, has_dollar_at);
  return (tresult);
}

/* Expand STRING just as if you were expanding a word.  This also returns
   a list of words.  Note that filename globbing is *NOT* done for word
   or string expansion, just when the shell is expanding a command.  This
   does parameter expansion, command substitution, arithmetic expansion,
   and word splitting.  Dequote the resultant WORD_LIST before returning. */
WORD_LIST *
expand_string (string, quoted)
     char *string;
     int quoted;
{
  WORD_LIST *result;

  if (string == 0 || *string == '\0')
    return ((WORD_LIST *)NULL);

  result = expand_string_leave_quoted (string, quoted);
  return (result ? dequote_list (result) : result);
}

/***************************************************
 *						   *
 *	Functions to handle quoting chars	   *
 *						   *
 ***************************************************/

/* Conventions:

     A string with s[0] == CTLNUL && s[1] == 0 is a quoted null string.
     The parser passes CTLNUL as CTLESC CTLNUL. */

/* Quote escape characters in string s, but no other characters.  This is
   used to protect CTLESC and CTLNUL in variable values from the rest of
   the word expansion process after the variable is expanded (word splitting
   and filename generation).  If IFS is null, we quote spaces as well, just
   in case we split on spaces later (in the case of unquoted $@, we will
   eventually attempt to split the entire word on spaces).  Corresponding
   code exists in dequote_escapes.  Even if we don't end up splitting on
   spaces, quoting spaces is not a problem.  This should never be called on
   a string that is quoted with single or double quotes or part of a here
   document (effectively double-quoted). */
char *
quote_escapes (string)
     char *string;
{
  register char *s, *t;
  size_t slen;
  char *result, *send;
  int quote_spaces, skip_ctlesc, skip_ctlnul;
  DECLARE_MBSTATE; 

  slen = strlen (string);
  send = string + slen;

  quote_spaces = (ifs_value && *ifs_value == 0);

  for (skip_ctlesc = skip_ctlnul = 0, s = ifs_value; s && *s; s++)
    skip_ctlesc |= *s == CTLESC, skip_ctlnul |= *s == CTLNUL;

  t = result = (char *)xmalloc ((slen * 2) + 1);
  s = string;

  while (*s)
    {
      if ((skip_ctlesc == 0 && *s == CTLESC) || (skip_ctlnul == 0 && *s == CTLNUL) || (quote_spaces && *s == ' '))
	*t++ = CTLESC;
      COPY_CHAR_P (t, s, send);
    }
  *t = '\0';
  return (result);
}

static WORD_LIST *
list_quote_escapes (list)
     WORD_LIST *list;
{
  register WORD_LIST *w;
  char *t;

  for (w = list; w; w = w->next)
    {
      t = w->word->word;
      w->word->word = quote_escapes (t);
      free (t);
    }
  return list;
}

/* Inverse of quote_escapes; remove CTLESC protecting CTLESC or CTLNUL.

   The parser passes us CTLESC as CTLESC CTLESC and CTLNUL as CTLESC CTLNUL.
   This is necessary to make unquoted CTLESC and CTLNUL characters in the
   data stream pass through properly.

   We need to remove doubled CTLESC characters inside quoted strings before
   quoting the entire string, so we do not double the number of CTLESC
   characters.

   Also used by parts of the pattern substitution code. */
char *
dequote_escapes (string)
     char *string;
{
  register char *s, *t, *s1;
  size_t slen;
  char *result, *send;
  int quote_spaces;
  DECLARE_MBSTATE;

  if (string == 0)
    return string;

  slen = strlen (string);
  send = string + slen;

  t = result = (char *)xmalloc (slen + 1);

  if (strchr (string, CTLESC) == 0)
    return (strcpy (result, string));

  quote_spaces = (ifs_value && *ifs_value == 0);

  s = string;
  while (*s)
    {
      if (*s == CTLESC && (s[1] == CTLESC || s[1] == CTLNUL || (quote_spaces && s[1] == ' ')))
	{
	  s++;
	  if (*s == '\0')
	    break;
	}
      COPY_CHAR_P (t, s, send);
    }
  *t = '\0';
  return result;
}

/* Return a new string with the quoted representation of character C.
   This turns "" into QUOTED_NULL, so the W_HASQUOTEDNULL flag needs to be
   set in any resultant WORD_DESC where this value is the word. */
static char *
make_quoted_char (c)
     int c;
{
  char *temp;

  temp = (char *)xmalloc (3);
  if (c == 0)
    {
      temp[0] = CTLNUL;
      temp[1] = '\0';
    }
  else
    {
      temp[0] = CTLESC;
      temp[1] = c;
      temp[2] = '\0';
    }
  return (temp);
}

/* Quote STRING, returning a new string.  This turns "" into QUOTED_NULL, so
   the W_HASQUOTEDNULL flag needs to be set in any resultant WORD_DESC where
   this value is the word. */
char *
quote_string (string)
     char *string;
{
  register char *t;
  size_t slen;
  char *result, *send;

  if (*string == 0)
    {
      result = (char *)xmalloc (2);
      result[0] = CTLNUL;
      result[1] = '\0';
    }
  else
    {
      DECLARE_MBSTATE;

      slen = strlen (string);
      send = string + slen;

      result = (char *)xmalloc ((slen * 2) + 1);

      for (t = result; string < send; )
	{
	  *t++ = CTLESC;
	  COPY_CHAR_P (t, string, send);
	}
      *t = '\0';
    }
  return (result);
}

/* De-quote quoted characters in STRING. */
char *
dequote_string (string)
     char *string;
{
  register char *s, *t;
  size_t slen;
  char *result, *send;
  DECLARE_MBSTATE;

  slen = strlen (string);

  t = result = (char *)xmalloc (slen + 1);

  if (QUOTED_NULL (string))
    {
      result[0] = '\0';
      return (result);
    }

  /* If no character in the string can be quoted, don't bother examining
     each character.  Just return a copy of the string passed to us. */
  if (strchr (string, CTLESC) == NULL)
    return (strcpy (result, string));

  send = string + slen;
  s = string;
  while (*s)
    {
      if (*s == CTLESC)
	{
	  s++;
	  if (*s == '\0')
	    break;
	}
      COPY_CHAR_P (t, s, send);
    }

  *t = '\0';
  return (result);
}

/* Quote the entire WORD_LIST list. */
static WORD_LIST *
quote_list (list)
     WORD_LIST *list;
{
  register WORD_LIST *w;
  char *t;

  for (w = list; w; w = w->next)
    {
      t = w->word->word;
      w->word->word = quote_string (t);
      if (*t == 0)
	w->word->flags |= W_HASQUOTEDNULL;	/* XXX - turn on W_HASQUOTEDNULL here? */
      w->word->flags |= W_QUOTED;
      free (t);
    }
  return list;
}

/* De-quote quoted characters in each word in LIST. */
WORD_LIST *
dequote_list (list)
     WORD_LIST *list;
{
  register char *s;
  register WORD_LIST *tlist;

  for (tlist = list; tlist; tlist = tlist->next)
    {
      s = dequote_string (tlist->word->word);
      if (QUOTED_NULL (tlist->word->word))
	tlist->word->flags &= ~W_HASQUOTEDNULL;
      free (tlist->word->word);
      tlist->word->word = s;
    }
  return list;
}

/* Remove CTLESC protecting a CTLESC or CTLNUL in place.  Return the passed
   string. */
char *
remove_quoted_escapes (string)
     char *string;
{
  char *t;

  if (string)
    {
      t = dequote_escapes (string);
      strcpy (string, t);
      free (t);
    }

  return (string);
}

/* Perform quoted null character removal on STRING.  We don't allow any
   quoted null characters in the middle or at the ends of strings because
   of how expand_word_internal works.  remove_quoted_nulls () turns
   STRING into an empty string iff it only consists of a quoted null,
   and removes all unquoted CTLNUL characters. */
char *
remove_quoted_nulls (string)
     char *string;
{
  register size_t slen;
  register int i, j, prev_i;
  DECLARE_MBSTATE;

  if (strchr (string, CTLNUL) == 0)		/* XXX */
    return string;				/* XXX */

  slen = strlen (string);
  i = j = 0;

  while (i < slen)
    {
      if (string[i] == CTLESC)
	{
	  /* Old code had j++, but we cannot assume that i == j at this
	     point -- what if a CTLNUL has already been removed from the
	     string?  We don't want to drop the CTLESC or recopy characters
	     that we've already copied down. */
	  i++; string[j++] = CTLESC;
	  if (i == slen)
	    break;
	}
      else if (string[i] == CTLNUL)
	i++;

      prev_i = i;
      ADVANCE_CHAR (string, slen, i);
      if (j < prev_i)
	{
	  do string[j++] = string[prev_i++]; while (prev_i < i);
	}
      else
	j = i;
    }
  string[j] = '\0';

  return (string);
}

/* Perform quoted null character removal on each element of LIST.
   This modifies LIST. */
void
word_list_remove_quoted_nulls (list)
     WORD_LIST *list;
{
  register WORD_LIST *t;

  for (t = list; t; t = t->next)
    {
      remove_quoted_nulls (t->word->word);
      t->word->flags &= ~W_HASQUOTEDNULL;
    }
}

/* **************************************************************** */
/*								    */
/*	   Functions for Matching and Removing Patterns		    */
/*								    */
/* **************************************************************** */

#if defined (HANDLE_MULTIBYTE)
#if 0 /* Currently unused */
static unsigned char *
mb_getcharlens (string, len)
     char *string;
     int len;
{
  int i, offset, last;
  unsigned char *ret;
  char *p;
  DECLARE_MBSTATE;

  i = offset = 0;
  last = 0;
  ret = (unsigned char *)xmalloc (len);
  memset (ret, 0, len);
  while (string[last])
    {
      ADVANCE_CHAR (string, len, offset);
      ret[last] = offset - last;
      last = offset;
    }
  return ret;
}
#endif
#endif

/* Remove the portion of PARAM matched by PATTERN according to OP, where OP
   can have one of 4 values:
	RP_LONG_LEFT	remove longest matching portion at start of PARAM
	RP_SHORT_LEFT	remove shortest matching portion at start of PARAM
	RP_LONG_RIGHT	remove longest matching portion at end of PARAM
	RP_SHORT_RIGHT	remove shortest matching portion at end of PARAM
*/

#define RP_LONG_LEFT	1
#define RP_SHORT_LEFT	2
#define RP_LONG_RIGHT	3
#define RP_SHORT_RIGHT	4

static char *
remove_upattern (param, pattern, op)
     char *param, *pattern;
     int op;
{
  register int len;
  register char *end;
  register char *p, *ret, c;

  len = STRLEN (param);
  end = param + len;

  switch (op)
    {
      case RP_LONG_LEFT:	/* remove longest match at start */
	for (p = end; p >= param; p--)
	  {
	    c = *p; *p = '\0';
	    if (strmatch (pattern, param, FNMATCH_EXTFLAG) != FNM_NOMATCH)
	      {
		*p = c;
		return (savestring (p));
	      }
	    *p = c;

	  }
	break;

      case RP_SHORT_LEFT:	/* remove shortest match at start */
	for (p = param; p <= end; p++)
	  {
	    c = *p; *p = '\0';
	    if (strmatch (pattern, param, FNMATCH_EXTFLAG) != FNM_NOMATCH)
	      {
		*p = c;
		return (savestring (p));
	      }
	    *p = c;
	  }
	break;

      case RP_LONG_RIGHT:	/* remove longest match at end */
	for (p = param; p <= end; p++)
	  {
	    if (strmatch (pattern, p, FNMATCH_EXTFLAG) != FNM_NOMATCH)
	      {
		c = *p; *p = '\0';
		ret = savestring (param);
		*p = c;
		return (ret);
	      }
	  }
	break;

      case RP_SHORT_RIGHT:	/* remove shortest match at end */
	for (p = end; p >= param; p--)
	  {
	    if (strmatch (pattern, p, FNMATCH_EXTFLAG) != FNM_NOMATCH)
	      {
		c = *p; *p = '\0';
		ret = savestring (param);
		*p = c;
		return (ret);
	      }
	  }
	break;
    }

  return (savestring (param));	/* no match, return original string */
}

#if defined (HANDLE_MULTIBYTE)
static wchar_t *
remove_wpattern (wparam, wstrlen, wpattern, op)
     wchar_t *wparam;
     size_t wstrlen;
     wchar_t *wpattern;
     int op;
{
  wchar_t wc, *ret;
  int n;

  switch (op)
    {
      case RP_LONG_LEFT:	/* remove longest match at start */
        for (n = wstrlen; n >= 0; n--)
	  {
	    wc = wparam[n]; wparam[n] = L'\0';
	    if (wcsmatch (wpattern, wparam, FNMATCH_EXTFLAG) != FNM_NOMATCH)
	      {
		wparam[n] = wc;
		return (wcsdup (wparam + n));
	      }
	    wparam[n] = wc;
	  }
	break;

      case RP_SHORT_LEFT:	/* remove shortest match at start */
	for (n = 0; n <= wstrlen; n++)
	  {
	    wc = wparam[n]; wparam[n] = L'\0';
	    if (wcsmatch (wpattern, wparam, FNMATCH_EXTFLAG) != FNM_NOMATCH)
	      {
		wparam[n] = wc;
		return (wcsdup (wparam + n));
	      }
	    wparam[n] = wc;
	  }
	break;

      case RP_LONG_RIGHT:	/* remove longest match at end */
        for (n = 0; n <= wstrlen; n++)
	  {
	    if (wcsmatch (wpattern, wparam + n, FNMATCH_EXTFLAG) != FNM_NOMATCH)
	      {
		wc = wparam[n]; wparam[n] = L'\0';
		ret = wcsdup (wparam);
		wparam[n] = wc;
		return (ret);
	      }
	  }
	break;

      case RP_SHORT_RIGHT:	/* remove shortest match at end */
	for (n = wstrlen; n >= 0; n--)
	  {
	    if (wcsmatch (wpattern, wparam + n, FNMATCH_EXTFLAG) != FNM_NOMATCH)
	      {
		wc = wparam[n]; wparam[n] = L'\0';
		ret = wcsdup (wparam);
		wparam[n] = wc;
		return (ret);
	      }
	  }
	break;
    }

  return (wcsdup (wparam));	/* no match, return original string */
}
#endif /* HANDLE_MULTIBYTE */

static char *
remove_pattern (param, pattern, op)
     char *param, *pattern;
     int op;
{
  if (param == NULL)
    return (param);
  if (*param == '\0' || pattern == NULL || *pattern == '\0')	/* minor optimization */
    return (savestring (param));

#if defined (HANDLE_MULTIBYTE)
  if (MB_CUR_MAX > 1)
    {
      wchar_t *ret, *oret;
      size_t n;
      wchar_t *wparam, *wpattern;
      mbstate_t ps;
      char *xret;

      n = xdupmbstowcs (&wpattern, NULL, pattern);
      if (n == (size_t)-1)
	return (remove_upattern (param, pattern, op));
      n = xdupmbstowcs (&wparam, NULL, param);
      if (n == (size_t)-1)
	{
	  free (wpattern);
	  return (remove_upattern (param, pattern, op));
	}
      oret = ret = remove_wpattern (wparam, n, wpattern, op);

      free (wparam);
      free (wpattern);

      n = strlen (param);
      xret = (char *)xmalloc (n + 1);
      memset (&ps, '\0', sizeof (mbstate_t));
      n = wcsrtombs (xret, (const wchar_t **)&ret, n, &ps);
      xret[n] = '\0';		/* just to make sure */
      free (oret);
      return xret;      
    }
  else
#endif
    return (remove_upattern (param, pattern, op));
}

/* Return 1 of the first character of STRING could match the first
   character of pattern PAT.  Used to avoid n2 calls to strmatch(). */
static int
match_pattern_char (pat, string)
     char *pat, *string;
{
  char c;

  if (*string == 0)
    return (0);

  switch (c = *pat++)
    {
    default:
      return (*string == c);
    case '\\':
      return (*string == *pat);
    case '?':
      return (*pat == LPAREN ? 1 : (*string != '\0'));
    case '*':
      return (1);
    case '+':
    case '!':
    case '@':
      return (*pat == LPAREN ? 1 : (*string == c));
    case '[':
      return (*string != '\0');
    }
}

/* Match PAT anywhere in STRING and return the match boundaries.
   This returns 1 in case of a successful match, 0 otherwise.  SP
   and EP are pointers into the string where the match begins and
   ends, respectively.  MTYPE controls what kind of match is attempted.
   MATCH_BEG and MATCH_END anchor the match at the beginning and end
   of the string, respectively.  The longest match is returned. */
static int
match_upattern (string, pat, mtype, sp, ep)
     char *string, *pat;
     int mtype;
     char **sp, **ep;
{
  int c, len;
  register char *p, *p1, *npat;
  char *end;

  /* If the pattern doesn't match anywhere in the string, go ahead and
     short-circuit right away.  A minor optimization, saves a bunch of
     unnecessary calls to strmatch (up to N calls for a string of N
     characters) if the match is unsuccessful.  To preserve the semantics
     of the substring matches below, we make sure that the pattern has
     `*' as first and last character, making a new pattern if necessary. */
  /* XXX - check this later if I ever implement `**' with special meaning,
     since this will potentially result in `**' at the beginning or end */
  len = STRLEN (pat);
  if (pat[0] != '*' || (pat[0] == '*' && pat[1] == LPAREN && extended_glob) || pat[len - 1] != '*')
    {
      p = npat = (char *)xmalloc (len + 3);
      p1 = pat;
      if (*p1 != '*' || (*p1 == '*' && p1[1] == LPAREN && extended_glob))
	*p++ = '*';
      while (*p1)
	*p++ = *p1++;
      if (p1[-1] != '*' || p[-2] == '\\')
	*p++ = '*';
      *p = '\0';
    }
  else
    npat = pat;
  c = strmatch (npat, string, FNMATCH_EXTFLAG);
  if (npat != pat)
    free (npat);
  if (c == FNM_NOMATCH)
    return (0);

  len = STRLEN (string);
  end = string + len;

  switch (mtype)
    {
    case MATCH_ANY:
      for (p = string; p <= end; p++)
	{
	  if (match_pattern_char (pat, p))
	    {
	      for (p1 = end; p1 >= p; p1--)
		{
		  c = *p1; *p1 = '\0';
		  if (strmatch (pat, p, FNMATCH_EXTFLAG) == 0)
		    {
		      *p1 = c;
		      *sp = p;
		      *ep = p1;
		      return 1;
		    }
		  *p1 = c;
		}
	    }
	}

      return (0);

    case MATCH_BEG:
      if (match_pattern_char (pat, string) == 0)
	return (0);

      for (p = end; p >= string; p--)
	{
	  c = *p; *p = '\0';
	  if (strmatch (pat, string, FNMATCH_EXTFLAG) == 0)
	    {
	      *p = c;
	      *sp = string;
	      *ep = p;
	      return 1;
	    }
	  *p = c;
	}

      return (0);

    case MATCH_END:
      for (p = string; p <= end; p++)
	{
	  if (strmatch (pat, p, FNMATCH_EXTFLAG) == 0)
	    {
	      *sp = p;
	      *ep = end;
	      return 1;
	    }

	}

      return (0);
    }

  return (0);
}

#if defined (HANDLE_MULTIBYTE)
/* Return 1 of the first character of WSTRING could match the first
   character of pattern WPAT.  Wide character version. */
static int
match_pattern_wchar (wpat, wstring)
     wchar_t *wpat, *wstring;
{
  wchar_t wc;

  if (*wstring == 0)
    return (0);

  switch (wc = *wpat++)
    {
    default:
      return (*wstring == wc);
    case L'\\':
      return (*wstring == *wpat);
    case L'?':
      return (*wpat == LPAREN ? 1 : (*wstring != L'\0'));
    case L'*':
      return (1);
    case L'+':
    case L'!':
    case L'@':
      return (*wpat == LPAREN ? 1 : (*wstring == wc));
    case L'[':
      return (*wstring != L'\0');
    }
}

/* Match WPAT anywhere in WSTRING and return the match boundaries.
   This returns 1 in case of a successful match, 0 otherwise.  Wide
   character version. */
static int
match_wpattern (wstring, indices, wstrlen, wpat, mtype, sp, ep)
     wchar_t *wstring;
     char **indices;
     size_t wstrlen;
     wchar_t *wpat;
     int mtype;
     char **sp, **ep;
{
  wchar_t wc, *wp, *nwpat, *wp1;
  int len;
#if 0
  size_t n, n1;	/* Apple's gcc seems to miscompile this badly */
#else
  int n, n1;
#endif

  /* If the pattern doesn't match anywhere in the string, go ahead and
     short-circuit right away.  A minor optimization, saves a bunch of
     unnecessary calls to strmatch (up to N calls for a string of N
     characters) if the match is unsuccessful.  To preserve the semantics
     of the substring matches below, we make sure that the pattern has
     `*' as first and last character, making a new pattern if necessary. */
  /* XXX - check this later if I ever implement `**' with special meaning,
     since this will potentially result in `**' at the beginning or end */
  len = wcslen (wpat);
  if (wpat[0] != L'*' || (wpat[0] == L'*' && wpat[1] == WLPAREN && extended_glob) || wpat[len - 1] != L'*')
    {
      wp = nwpat = (wchar_t *)xmalloc ((len + 3) * sizeof (wchar_t));
      wp1 = wpat;
      if (*wp1 != L'*' || (*wp1 == '*' && wp1[1] == WLPAREN && extended_glob))
	*wp++ = L'*';
      while (*wp1 != L'\0')
	*wp++ = *wp1++;
      if (wp1[-1] != L'*' || wp1[-2] == L'\\')
        *wp++ = L'*';
      *wp = '\0';
    }
  else
    nwpat = wpat;
  len = wcsmatch (nwpat, wstring, FNMATCH_EXTFLAG);
  if (nwpat != wpat)
    free (nwpat);
  if (len == FNM_NOMATCH)
    return (0);

  switch (mtype)
    {
    case MATCH_ANY:
      for (n = 0; n <= wstrlen; n++)
	{
	  if (match_pattern_wchar (wpat, wstring + n))
	    {
	      for (n1 = wstrlen; n1 >= n; n1--)
		{
		  wc = wstring[n1]; wstring[n1] = L'\0';
		  if (wcsmatch (wpat, wstring + n, FNMATCH_EXTFLAG) == 0)
		    {
		      wstring[n1] = wc;
		      *sp = indices[n];
		      *ep = indices[n1];
		      return 1;
		    }
		  wstring[n1] = wc;
		}
	    }
	}

      return (0);

    case MATCH_BEG:
      if (match_pattern_wchar (wpat, wstring) == 0)
	return (0);

      for (n = wstrlen; n >= 0; n--)
	{
	  wc = wstring[n]; wstring[n] = L'\0';
	  if (wcsmatch (wpat, wstring, FNMATCH_EXTFLAG) == 0)
	    {
	      wstring[n] = wc;
	      *sp = indices[0];
	      *ep = indices[n];
	      return 1;
	    }
	  wstring[n] = wc;
	}

      return (0);

    case MATCH_END:
      for (n = 0; n <= wstrlen; n++)
	{
	  if (wcsmatch (wpat, wstring + n, FNMATCH_EXTFLAG) == 0)
	    {
	      *sp = indices[n];
	      *ep = indices[wstrlen];
	      return 1;
	    }
	}

      return (0);
    }

  return (0);
}
#endif /* HANDLE_MULTIBYTE */

static int
match_pattern (string, pat, mtype, sp, ep)
     char *string, *pat;
     int mtype;
     char **sp, **ep;
{
#if defined (HANDLE_MULTIBYTE)
  int ret;
  size_t n;
  wchar_t *wstring, *wpat;
  char **indices;
#endif

  if (string == 0 || *string == 0 || pat == 0 || *pat == 0)
    return (0);

#if defined (HANDLE_MULTIBYTE)
  if (MB_CUR_MAX > 1)
    {
      n = xdupmbstowcs (&wpat, NULL, pat);
      if (n == (size_t)-1)
	return (match_upattern (string, pat, mtype, sp, ep));
      n = xdupmbstowcs (&wstring, &indices, string);
      if (n == (size_t)-1)
	{
	  free (wpat);
	  return (match_upattern (string, pat, mtype, sp, ep));
	}
      ret = match_wpattern (wstring, indices, n, wpat, mtype, sp, ep);

      free (wpat);
      free (wstring);
      free (indices);

      return (ret);
    }
  else
#endif
    return (match_upattern (string, pat, mtype, sp, ep));
}

static int
getpatspec (c, value)
     int c;
     char *value;
{
  if (c == '#')
    return ((*value == '#') ? RP_LONG_LEFT : RP_SHORT_LEFT);
  else	/* c == '%' */
    return ((*value == '%') ? RP_LONG_RIGHT : RP_SHORT_RIGHT);
}

/* Posix.2 says that the WORD should be run through tilde expansion,
   parameter expansion, command substitution and arithmetic expansion.
   This leaves the result quoted, so quote_string_for_globbing () has
   to be called to fix it up for strmatch ().  If QUOTED is non-zero,
   it means that the entire expression was enclosed in double quotes.
   This means that quoting characters in the pattern do not make any
   special pattern characters quoted.  For example, the `*' in the
   following retains its special meaning: "${foo#'*'}". */
static char *
getpattern (value, quoted, expandpat)
     char *value;
     int quoted, expandpat;
{
  char *pat, *tword;
  WORD_LIST *l;
#if 0
  int i;
#endif
  /* There is a problem here:  how to handle single or double quotes in the
     pattern string when the whole expression is between double quotes?
     POSIX.2 says that enclosing double quotes do not cause the pattern to
     be quoted, but does that leave us a problem with @ and array[@] and their
     expansions inside a pattern? */
#if 0
  if (expandpat && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && *tword)
    {
      i = 0;
      pat = string_extract_double_quoted (tword, &i, 1);
      free (tword);
      tword = pat;
    }
#endif

  /* expand_string_for_rhs () leaves WORD quoted and does not perform
     word splitting. */
  l = *value ? expand_string_for_rhs (value,
				      (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) ? Q_PATQUOTE : quoted,
				      (int *)NULL, (int *)NULL)
	     : (WORD_LIST *)0;
  pat = string_list (l);
  dispose_words (l);
  if (pat)
    {
      tword = quote_string_for_globbing (pat, QGLOB_CVTNULL);
      free (pat);
      pat = tword;
    }
  return (pat);
}

#if 0
/* Handle removing a pattern from a string as a result of ${name%[%]value}
   or ${name#[#]value}. */
static char *
variable_remove_pattern (value, pattern, patspec, quoted)
     char *value, *pattern;
     int patspec, quoted;
{
  char *tword;

  tword = remove_pattern (value, pattern, patspec);

  return (tword);
}
#endif

static char *
list_remove_pattern (list, pattern, patspec, itype, quoted)
     WORD_LIST *list;
     char *pattern;
     int patspec, itype, quoted;
{
  WORD_LIST *new, *l;
  WORD_DESC *w;
  char *tword;

  for (new = (WORD_LIST *)NULL, l = list; l; l = l->next)
    {
      tword = remove_pattern (l->word->word, pattern, patspec);
      w = alloc_word_desc ();
      w->word = tword ? tword : savestring ("");
      new = make_word_list (w, new);
    }

  l = REVERSE_LIST (new, WORD_LIST *);
  tword = string_list_pos_params (itype, l, quoted);
  dispose_words (l);

  return (tword);
}

static char *
parameter_list_remove_pattern (itype, pattern, patspec, quoted)
     int itype;
     char *pattern;
     int patspec, quoted;
{
  char *ret;
  WORD_LIST *list;

  list = list_rest_of_args ();
  if (list == 0)
    return ((char *)NULL);
  ret = list_remove_pattern (list, pattern, patspec, itype, quoted);
  dispose_words (list);
  return (ret);
}

#if defined (ARRAY_VARS)
static char *
array_remove_pattern (var, pattern, patspec, varname, quoted)
     SHELL_VAR *var;
     char *pattern;
     int patspec;
     char *varname;	/* so we can figure out how it's indexed */
     int quoted;
{
  ARRAY *a;
  HASH_TABLE *h;
  int itype;
  char *ret;
  WORD_LIST *list;
  SHELL_VAR *v;

  /* compute itype from varname here */
  v = array_variable_part (varname, &ret, 0);
  itype = ret[0];

  a = (v && array_p (v)) ? array_cell (v) : 0;
  h = (v && assoc_p (v)) ? assoc_cell (v) : 0;
  
  list = a ? array_to_word_list (a) : (h ? assoc_to_word_list (h) : 0);
  if (list == 0)
   return ((char *)NULL);
  ret = list_remove_pattern (list, pattern, patspec, itype, quoted);
  dispose_words (list);

  return ret;
}
#endif /* ARRAY_VARS */

static char *
parameter_brace_remove_pattern (varname, value, patstr, rtype, quoted)
     char *varname, *value, *patstr;
     int rtype, quoted;
{
  int vtype, patspec, starsub;
  char *temp1, *val, *pattern;
  SHELL_VAR *v;

  if (value == 0)
    return ((char *)NULL);

  this_command_name = varname;

  vtype = get_var_and_type (varname, value, quoted, &v, &val);
  if (vtype == -1)
    return ((char *)NULL);

  starsub = vtype & VT_STARSUB;
  vtype &= ~VT_STARSUB;

  patspec = getpatspec (rtype, patstr);
  if (patspec == RP_LONG_LEFT || patspec == RP_LONG_RIGHT)
    patstr++;

  /* Need to pass getpattern newly-allocated memory in case of expansion --
     the expansion code will free the passed string on an error. */
  temp1 = savestring (patstr);
  pattern = getpattern (temp1, quoted, 1);
  free (temp1);

  temp1 = (char *)NULL;		/* shut up gcc */
  switch (vtype)
    {
    case VT_VARIABLE:
    case VT_ARRAYMEMBER:
      temp1 = remove_pattern (val, pattern, patspec);
      if (vtype == VT_VARIABLE)
	FREE (val);
      if (temp1)
	{
	  val = (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
			? quote_string (temp1)
			: quote_escapes (temp1);
	  free (temp1);
	  temp1 = val;
	}
      break;
#if defined (ARRAY_VARS)
    case VT_ARRAYVAR:
      temp1 = array_remove_pattern (v, pattern, patspec, varname, quoted);
      if (temp1 && ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) == 0))
	{
	  val = quote_escapes (temp1);
	  free (temp1);
	  temp1 = val;
	}
      break;
#endif
    case VT_POSPARMS:
      temp1 = parameter_list_remove_pattern (varname[0], pattern, patspec, quoted);
      if (temp1 && ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) == 0))
	{
	  val = quote_escapes (temp1);
	  free (temp1);
	  temp1 = val;
	}
      break;
    }

  FREE (pattern);
  return temp1;
}    

/*******************************************
 *					   *
 *	Functions to expand WORD_DESCs	   *
 *					   *
 *******************************************/

/* Expand WORD, performing word splitting on the result.  This does
   parameter expansion, command substitution, arithmetic expansion,
   word splitting, and quote removal. */

WORD_LIST *
expand_word (word, quoted)
     WORD_DESC *word;
     int quoted;
{
  WORD_LIST *result, *tresult;

  tresult = call_expand_word_internal (word, quoted, 0, (int *)NULL, (int *)NULL);
  result = word_list_split (tresult);
  dispose_words (tresult);
  return (result ? dequote_list (result) : result);
}

/* Expand WORD, but do not perform word splitting on the result.  This
   does parameter expansion, command substitution, arithmetic expansion,
   and quote removal. */
WORD_LIST *
expand_word_unsplit (word, quoted)
     WORD_DESC *word;
     int quoted;
{
  WORD_LIST *result;

  expand_no_split_dollar_star = 1;
#if defined (HANDLE_MULTIBYTE)
  if (ifs_firstc[0] == 0)
#else
  if (ifs_firstc == 0)
#endif
    word->flags |= W_NOSPLIT;
  result = call_expand_word_internal (word, quoted, 0, (int *)NULL, (int *)NULL);
  expand_no_split_dollar_star = 0;

  return (result ? dequote_list (result) : result);
}

/* Perform shell expansions on WORD, but do not perform word splitting or
   quote removal on the result.  Virtually identical to expand_word_unsplit;
   could be combined if implementations don't diverge. */
WORD_LIST *
expand_word_leave_quoted (word, quoted)
     WORD_DESC *word;
     int quoted;
{
  WORD_LIST *result;

  expand_no_split_dollar_star = 1;
#if defined (HANDLE_MULTIBYTE)
  if (ifs_firstc[0] == 0)
#else
  if (ifs_firstc == 0)
#endif
    word->flags |= W_NOSPLIT;
  word->flags |= W_NOSPLIT2;
  result = call_expand_word_internal (word, quoted, 0, (int *)NULL, (int *)NULL);
  expand_no_split_dollar_star = 0;

  return result;
}

#if defined (PROCESS_SUBSTITUTION)

/*****************************************************************/
/*								 */
/*		    Hacking Process Substitution		 */
/*								 */
/*****************************************************************/

#if !defined (HAVE_DEV_FD)
/* Named pipes must be removed explicitly with `unlink'.  This keeps a list
   of FIFOs the shell has open.  unlink_fifo_list will walk the list and
   unlink all of them. add_fifo_list adds the name of an open FIFO to the
   list.  NFIFO is a count of the number of FIFOs in the list. */
#define FIFO_INCR 20

struct temp_fifo {
  char *file;
  pid_t proc;
};

static struct temp_fifo *fifo_list = (struct temp_fifo *)NULL;
static int nfifo;
static int fifo_list_size;

static void
add_fifo_list (pathname)
     char *pathname;
{
  if (nfifo >= fifo_list_size - 1)
    {
      fifo_list_size += FIFO_INCR;
      fifo_list = (struct temp_fifo *)xrealloc (fifo_list,
				fifo_list_size * sizeof (struct temp_fifo));
    }

  fifo_list[nfifo].file = savestring (pathname);
  nfifo++;
}

void
unlink_fifo_list ()
{
  int saved, i, j;

  if (nfifo == 0)
    return;

  for (i = saved = 0; i < nfifo; i++)
    {
      if ((fifo_list[i].proc == -1) || (kill(fifo_list[i].proc, 0) == -1))
	{
	  unlink (fifo_list[i].file);
	  free (fifo_list[i].file);
	  fifo_list[i].file = (char *)NULL;
	  fifo_list[i].proc = -1;
	}
      else
	saved++;
    }

  /* If we didn't remove some of the FIFOs, compact the list. */
  if (saved)
    {
      for (i = j = 0; i < nfifo; i++)
	if (fifo_list[i].file)
	  {
	    fifo_list[j].file = fifo_list[i].file;
	    fifo_list[j].proc = fifo_list[i].proc;
	    j++;
	  }
      nfifo = j;
    }
  else
    nfifo = 0;
}

int
fifos_pending ()
{
  return nfifo;
}

static char *
make_named_pipe ()
{
  char *tname;

  tname = sh_mktmpname ("sh-np", MT_USERANDOM|MT_USETMPDIR);
  if (mkfifo (tname, 0600) < 0)
    {
      free (tname);
      return ((char *)NULL);
    }

  add_fifo_list (tname);
  return (tname);
}

#else /* HAVE_DEV_FD */

/* DEV_FD_LIST is a bitmap of file descriptors attached to pipes the shell
   has open to children.  NFDS is a count of the number of bits currently
   set in DEV_FD_LIST.  TOTFDS is a count of the highest possible number
   of open files. */
static char *dev_fd_list = (char *)NULL;
static int nfds;
static int totfds;	/* The highest possible number of open files. */

static void
add_fifo_list (fd)
     int fd;
{
  if (!dev_fd_list || fd >= totfds)
    {
      int ofds;

      ofds = totfds;
      totfds = getdtablesize ();
      if (totfds < 0 || totfds > 256)
	totfds = 256;
      if (fd >= totfds)
	totfds = fd + 2;

      dev_fd_list = (char *)xrealloc (dev_fd_list, totfds);
      memset (dev_fd_list + ofds, '\0', totfds - ofds);
    }

  dev_fd_list[fd] = 1;
  nfds++;
}

int
fifos_pending ()
{
  return 0;	/* used for cleanup; not needed with /dev/fd */
}

void
unlink_fifo_list ()
{
  register int i;

  if (nfds == 0)
    return;

  for (i = 0; nfds && i < totfds; i++)
    if (dev_fd_list[i])
      {
	close (i);
	dev_fd_list[i] = 0;
	nfds--;
      }

  nfds = 0;
}

#if defined (NOTDEF)
print_dev_fd_list ()
{
  register int i;

  fprintf (stderr, "pid %ld: dev_fd_list:", (long)getpid ());
  fflush (stderr);

  for (i = 0; i < totfds; i++)
    {
      if (dev_fd_list[i])
	fprintf (stderr, " %d", i);
    }
  fprintf (stderr, "\n");
}
#endif /* NOTDEF */

static char *
make_dev_fd_filename (fd)
     int fd;
{
  char *ret, intbuf[INT_STRLEN_BOUND (int) + 1], *p;

  ret = (char *)xmalloc (sizeof (DEV_FD_PREFIX) + 8);

  strcpy (ret, DEV_FD_PREFIX);
  p = inttostr (fd, intbuf, sizeof (intbuf));
  strcpy (ret + sizeof (DEV_FD_PREFIX) - 1, p);

  add_fifo_list (fd);
  return (ret);
}

#endif /* HAVE_DEV_FD */

/* Return a filename that will open a connection to the process defined by
   executing STRING.  HAVE_DEV_FD, if defined, means open a pipe and return
   a filename in /dev/fd corresponding to a descriptor that is one of the
   ends of the pipe.  If not defined, we use named pipes on systems that have
   them.  Systems without /dev/fd and named pipes are out of luck.

   OPEN_FOR_READ_IN_CHILD, if 1, means open the named pipe for reading or
   use the read end of the pipe and dup that file descriptor to fd 0 in
   the child.  If OPEN_FOR_READ_IN_CHILD is 0, we open the named pipe for
   writing or use the write end of the pipe in the child, and dup that
   file descriptor to fd 1 in the child.  The parent does the opposite. */

static char *
process_substitute (string, open_for_read_in_child)
     char *string;
     int open_for_read_in_child;
{
  char *pathname;
  int fd, result;
  pid_t old_pid, pid;
#if defined (HAVE_DEV_FD)
  int parent_pipe_fd, child_pipe_fd;
  int fildes[2];
#endif /* HAVE_DEV_FD */
#if defined (JOB_CONTROL)
  pid_t old_pipeline_pgrp;
#endif

  if (!string || !*string || wordexp_only)
    return ((char *)NULL);

#if !defined (HAVE_DEV_FD)
  pathname = make_named_pipe ();
#else /* HAVE_DEV_FD */
  if (pipe (fildes) < 0)
    {
      sys_error (_("cannot make pipe for process substitution"));
      return ((char *)NULL);
    }
  /* If OPEN_FOR_READ_IN_CHILD == 1, we want to use the write end of
     the pipe in the parent, otherwise the read end. */
  parent_pipe_fd = fildes[open_for_read_in_child];
  child_pipe_fd = fildes[1 - open_for_read_in_child];
  /* Move the parent end of the pipe to some high file descriptor, to
     avoid clashes with FDs used by the script. */
  parent_pipe_fd = move_to_high_fd (parent_pipe_fd, 1, 64);

  pathname = make_dev_fd_filename (parent_pipe_fd);
#endif /* HAVE_DEV_FD */

  if (pathname == 0)
    {
      sys_error (_("cannot make pipe for process substitution"));
      return ((char *)NULL);
    }

  old_pid = last_made_pid;

#if defined (JOB_CONTROL)
  old_pipeline_pgrp = pipeline_pgrp;
  pipeline_pgrp = shell_pgrp;
  save_pipeline (1);
#endif /* JOB_CONTROL */

  pid = make_child ((char *)NULL, 1);
  if (pid == 0)
    {
      reset_terminating_signals ();	/* XXX */
      free_pushed_string_input ();
      /* Cancel traps, in trap.c. */
      restore_original_signals ();
      setup_async_signals ();
      subshell_environment |= SUBSHELL_COMSUB|SUBSHELL_PROCSUB;
    }

#if defined (JOB_CONTROL)
  set_sigchld_handler ();
  stop_making_children ();
  /* XXX - should we only do this in the parent? (as in command subst) */
  pipeline_pgrp = old_pipeline_pgrp;
#endif /* JOB_CONTROL */

  if (pid < 0)
    {
      sys_error (_("cannot make child for process substitution"));
      free (pathname);
#if defined (HAVE_DEV_FD)
      close (parent_pipe_fd);
      close (child_pipe_fd);
#endif /* HAVE_DEV_FD */
      return ((char *)NULL);
    }

  if (pid > 0)
    {
#if defined (JOB_CONTROL)
      restore_pipeline (1);
#endif

#if !defined (HAVE_DEV_FD)
      fifo_list[nfifo-1].proc = pid;
#endif

      last_made_pid = old_pid;

#if defined (JOB_CONTROL) && defined (PGRP_PIPE)
      close_pgrp_pipe ();
#endif /* JOB_CONTROL && PGRP_PIPE */

#if defined (HAVE_DEV_FD)
      close (child_pipe_fd);
#endif /* HAVE_DEV_FD */

      return (pathname);
    }

  set_sigint_handler ();

#if defined (JOB_CONTROL)
  set_job_control (0);
#endif /* JOB_CONTROL */

#if !defined (HAVE_DEV_FD)
  /* Open the named pipe in the child. */
  fd = open (pathname, open_for_read_in_child ? O_RDONLY|O_NONBLOCK : O_WRONLY);
  if (fd < 0)
    {
      /* Two separate strings for ease of translation. */
      if (open_for_read_in_child)
	sys_error (_("cannot open named pipe %s for reading"), pathname);
      else
	sys_error (_("cannot open named pipe %s for writing"), pathname);

      exit (127);
    }
  if (open_for_read_in_child)
    {
      if (sh_unset_nodelay_mode (fd) < 0)
	{
	  sys_error (_("cannot reset nodelay mode for fd %d"), fd);
	  exit (127);
	}
    }
#else /* HAVE_DEV_FD */
  fd = child_pipe_fd;
#endif /* HAVE_DEV_FD */

  if (dup2 (fd, open_for_read_in_child ? 0 : 1) < 0)
    {
      sys_error (_("cannot duplicate named pipe %s as fd %d"), pathname,
	open_for_read_in_child ? 0 : 1);
      exit (127);
    }

  if (fd != (open_for_read_in_child ? 0 : 1))
    close (fd);

  /* Need to close any files that this process has open to pipes inherited
     from its parent. */
  if (current_fds_to_close)
    {
      close_fd_bitmap (current_fds_to_close);
      current_fds_to_close = (struct fd_bitmap *)NULL;
    }

#if defined (HAVE_DEV_FD)
  /* Make sure we close the parent's end of the pipe and clear the slot
     in the fd list so it is not closed later, if reallocated by, for
     instance, pipe(2). */
  close (parent_pipe_fd);
  dev_fd_list[parent_pipe_fd] = 0;
#endif /* HAVE_DEV_FD */

  result = parse_and_execute (string, "process substitution", (SEVAL_NONINT|SEVAL_NOHIST));

#if !defined (HAVE_DEV_FD)
  /* Make sure we close the named pipe in the child before we exit. */
  close (open_for_read_in_child ? 0 : 1);
#endif /* !HAVE_DEV_FD */

  exit (result);
  /*NOTREACHED*/
}
#endif /* PROCESS_SUBSTITUTION */

/***********************************/
/*				   */
/*	Command Substitution	   */
/*				   */
/***********************************/

static char *
read_comsub (fd, quoted, rflag)
     int fd, quoted;
     int *rflag;
{
  char *istring, buf[128], *bufp, *s;
  int istring_index, istring_size, c, tflag, skip_ctlesc, skip_ctlnul;
  ssize_t bufn;

  istring = (char *)NULL;
  istring_index = istring_size = bufn = tflag = 0;

  for (skip_ctlesc = skip_ctlnul = 0, s = ifs_value; s && *s; s++)
    skip_ctlesc |= *s == CTLESC, skip_ctlnul |= *s == CTLNUL;

#ifdef __CYGWIN__
  setmode (fd, O_TEXT);		/* we don't want CR/LF, we want Unix-style */
#endif

  /* Read the output of the command through the pipe.  This may need to be
     changed to understand multibyte characters in the future. */
  while (1)
    {
      if (fd < 0)
	break;
      if (--bufn <= 0)
	{
	  bufn = zread (fd, buf, sizeof (buf));
	  if (bufn <= 0) 
	    break;
	  bufp = buf;
	}
      c = *bufp++;

      if (c == 0)
	{
#if 0
	  internal_warning ("read_comsub: ignored null byte in input");
#endif
	  continue;
	}

      /* Add the character to ISTRING, possibly after resizing it. */
      RESIZE_MALLOCED_BUFFER (istring, istring_index, 2, istring_size, DEFAULT_ARRAY_SIZE);

      /* This is essentially quote_string inline */
      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) /* || c == CTLESC || c == CTLNUL */)
	istring[istring_index++] = CTLESC;
      /* Escape CTLESC and CTLNUL in the output to protect those characters
	 from the rest of the word expansions (word splitting and globbing.)
	 This is essentially quote_escapes inline. */
      else if (skip_ctlesc == 0 && c == CTLESC)
	{
	  tflag |= W_HASCTLESC;
	  istring[istring_index++] = CTLESC;
	}
      else if ((skip_ctlnul == 0 && c == CTLNUL) || (c == ' ' && (ifs_value && *ifs_value == 0)))
	istring[istring_index++] = CTLESC;

      istring[istring_index++] = c;

#if 0
#if defined (__CYGWIN__)
      if (c == '\n' && istring_index > 1 && istring[istring_index - 2] == '\r')
	{
	  istring_index--;
	  istring[istring_index - 1] = '\n';
	}
#endif
#endif
    }

  if (istring)
    istring[istring_index] = '\0';

  /* If we read no output, just return now and save ourselves some
     trouble. */
  if (istring_index == 0)
    {
      FREE (istring);
      if (rflag)
	*rflag = tflag;
      return (char *)NULL;
    }

  /* Strip trailing newlines from the output of the command. */
  if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
    {
      while (istring_index > 0)
	{
	  if (istring[istring_index - 1] == '\n')
	    {
	      --istring_index;

	      /* If the newline was quoted, remove the quoting char. */
	      if (istring[istring_index - 1] == CTLESC)
		--istring_index;
	    }
	  else
	    break;
	}
      istring[istring_index] = '\0';
    }
  else
    strip_trailing (istring, istring_index - 1, 1);

  if (rflag)
    *rflag = tflag;
  return istring;
}

/* Perform command substitution on STRING.  This returns a WORD_DESC * with the
   contained string possibly quoted. */
WORD_DESC *
command_substitute (string, quoted)
     char *string;
     int quoted;
{
  pid_t pid, old_pid, old_pipeline_pgrp, old_async_pid;
  char *istring;
  int result, fildes[2], function_value, pflags, rc, tflag;
  WORD_DESC *ret;

  istring = (char *)NULL;

  /* Don't fork () if there is no need to.  In the case of no command to
     run, just return NULL. */
  if (!string || !*string || (string[0] == '\n' && !string[1]))
    return ((WORD_DESC *)NULL);

  if (wordexp_only && read_but_dont_execute)
    {
      last_command_exit_value = EX_WEXPCOMSUB;
      jump_to_top_level (EXITPROG);
    }

  /* We're making the assumption here that the command substitution will
     eventually run a command from the file system.  Since we'll run
     maybe_make_export_env in this subshell before executing that command,
     the parent shell and any other shells it starts will have to remake
     the environment.  If we make it before we fork, other shells won't
     have to.  Don't bother if we have any temporary variable assignments,
     though, because the export environment will be remade after this
     command completes anyway, but do it if all the words to be expanded
     are variable assignments. */
  if (subst_assign_varlist == 0 || garglist == 0)
    maybe_make_export_env ();	/* XXX */

  /* Flags to pass to parse_and_execute() */
  pflags = (interactive && sourcelevel == 0) ? SEVAL_RESETLINE : 0;

  /* Pipe the output of executing STRING into the current shell. */
  if (pipe (fildes) < 0)
    {
      sys_error (_("cannot make pipe for command substitution"));
      goto error_exit;
    }

  old_pid = last_made_pid;
#if defined (JOB_CONTROL)
  old_pipeline_pgrp = pipeline_pgrp;
  /* Don't reset the pipeline pgrp if we're already a subshell in a pipeline. */
  if ((subshell_environment & SUBSHELL_PIPE) == 0)
    pipeline_pgrp = shell_pgrp;
  cleanup_the_pipeline ();
#endif /* JOB_CONTROL */

  old_async_pid = last_asynchronous_pid;
  pid = make_child ((char *)NULL, subshell_environment&SUBSHELL_ASYNC);
  last_asynchronous_pid = old_async_pid;

  if (pid == 0)
    /* Reset the signal handlers in the child, but don't free the
       trap strings. */
    reset_signal_handlers ();

#if defined (JOB_CONTROL)
  /* XXX DO THIS ONLY IN PARENT ? XXX */
  set_sigchld_handler ();
  stop_making_children ();
  if (pid != 0)
    pipeline_pgrp = old_pipeline_pgrp;
#else
  stop_making_children ();
#endif /* JOB_CONTROL */

  if (pid < 0)
    {
      sys_error (_("cannot make child for command substitution"));
    error_exit:

      FREE (istring);
      close (fildes[0]);
      close (fildes[1]);
      return ((WORD_DESC *)NULL);
    }

  if (pid == 0)
    {
      set_sigint_handler ();	/* XXX */

      free_pushed_string_input ();

      if (dup2 (fildes[1], 1) < 0)
	{
	  sys_error (_("command_substitute: cannot duplicate pipe as fd 1"));
	  exit (EXECUTION_FAILURE);
	}

      /* If standard output is closed in the parent shell
	 (such as after `exec >&-'), file descriptor 1 will be
	 the lowest available file descriptor, and end up in
	 fildes[0].  This can happen for stdin and stderr as well,
	 but stdout is more important -- it will cause no output
	 to be generated from this command. */
      if ((fildes[1] != fileno (stdin)) &&
	  (fildes[1] != fileno (stdout)) &&
	  (fildes[1] != fileno (stderr)))
	close (fildes[1]);

      if ((fildes[0] != fileno (stdin)) &&
	  (fildes[0] != fileno (stdout)) &&
	  (fildes[0] != fileno (stderr)))
	close (fildes[0]);

      /* The currently executing shell is not interactive. */
      interactive = 0;

      /* This is a subshell environment. */
      subshell_environment |= SUBSHELL_COMSUB;

      /* When not in POSIX mode, command substitution does not inherit
	 the -e flag. */
      if (posixly_correct == 0)
	exit_immediately_on_error = 0;

      remove_quoted_escapes (string);

      startup_state = 2;	/* see if we can avoid a fork */
      /* Give command substitution a place to jump back to on failure,
	 so we don't go back up to main (). */
      result = setjmp (top_level);

      /* If we're running a command substitution inside a shell function,
	 trap `return' so we don't return from the function in the subshell
	 and go off to never-never land. */
      if (result == 0 && return_catch_flag)
	function_value = setjmp (return_catch);
      else
	function_value = 0;

      if (result == ERREXIT)
	rc = last_command_exit_value;
      else if (result == EXITPROG)
	rc = last_command_exit_value;
      else if (result)
	rc = EXECUTION_FAILURE;
      else if (function_value)
	rc = return_catch_value;
      else
	{
	  subshell_level++;
	  rc = parse_and_execute (string, "command substitution", pflags|SEVAL_NOHIST);
	  subshell_level--;
	}

      last_command_exit_value = rc;
      rc = run_exit_trap ();
#if defined (PROCESS_SUBSTITUTION)
      unlink_fifo_list ();
#endif
      exit (rc);
    }
  else
    {
#if defined (JOB_CONTROL) && defined (PGRP_PIPE)
      close_pgrp_pipe ();
#endif /* JOB_CONTROL && PGRP_PIPE */

      close (fildes[1]);

      tflag = 0;
      istring = read_comsub (fildes[0], quoted, &tflag);

      close (fildes[0]);

      current_command_subst_pid = pid;
      last_command_exit_value = wait_for (pid);
      last_command_subst_pid = pid;
      last_made_pid = old_pid;

#if defined (JOB_CONTROL)
      /* If last_command_exit_value > 128, then the substituted command
	 was terminated by a signal.  If that signal was SIGINT, then send
	 SIGINT to ourselves.  This will break out of loops, for instance. */
      if (last_command_exit_value == (128 + SIGINT) && last_command_exit_signal == SIGINT)
	kill (getpid (), SIGINT);

      /* wait_for gives the terminal back to shell_pgrp.  If some other
	 process group should have it, give it away to that group here.
	 pipeline_pgrp is non-zero only while we are constructing a
	 pipline, so what we are concerned about is whether or not that
	 pipeline was started in the background.  A pipeline started in
	 the background should never get the tty back here. */
#if 0
      if (interactive && pipeline_pgrp != (pid_t)0 && pipeline_pgrp != last_asynchronous_pid)
#else
      if (interactive && pipeline_pgrp != (pid_t)0 && (subshell_environment & SUBSHELL_ASYNC) == 0)
#endif
	give_terminal_to (pipeline_pgrp, 0);
#endif /* JOB_CONTROL */

      ret = alloc_word_desc ();
      ret->word = istring;
      ret->flags = tflag;

      return ret;
    }
}

/********************************************************
 *							*
 *	Utility functions for parameter expansion	*
 *							*
 ********************************************************/

#if defined (ARRAY_VARS)

static arrayind_t
array_length_reference (s)
     char *s;
{
  int len;
  arrayind_t ind;
  char *akey;
  char *t, c;
  ARRAY *array;
  SHELL_VAR *var;

  var = array_variable_part (s, &t, &len);

  /* If unbound variables should generate an error, report one and return
     failure. */
  if ((var == 0 || (assoc_p (var) == 0 && array_p (var) == 0)) && unbound_vars_is_error)
    {
      c = *--t;
      *t = '\0';
      last_command_exit_value = EXECUTION_FAILURE;
      err_unboundvar (s);
      *t = c;
      return (-1);
    }
  else if (var == 0)
    return 0;

  /* We support a couple of expansions for variables that are not arrays.
     We'll return the length of the value for v[0], and 1 for v[@] or
     v[*].  Return 0 for everything else. */

  array = array_p (var) ? array_cell (var) : (ARRAY *)NULL;

  if (ALL_ELEMENT_SUB (t[0]) && t[1] == ']')
    {
      if (assoc_p (var))
	return (assoc_num_elements (assoc_cell (var)));
      else if (array_p (var))
	return (array_num_elements (array));
      else
	return 1;
    }

  if (assoc_p (var))
    {
      t[len - 1] = '\0';
      akey = expand_assignment_string_to_string (t, 0);	/* [ */
      t[len - 1] = ']';
      if (akey == 0 || *akey == 0)
	{
	  err_badarraysub (t);
	  return (-1);
	}
      t = assoc_reference (assoc_cell (var), akey);
    }
  else
    {
      ind = array_expand_index (t, len);
      if (ind < 0)
	{
	  err_badarraysub (t);
	  return (-1);
	}
      if (array_p (var))
	t = array_reference (array, ind);
      else
	t = (ind == 0) ? value_cell (var) : (char *)NULL;
    }

  len = MB_STRLEN (t);
  return (len);
}
#endif /* ARRAY_VARS */

static int
valid_brace_expansion_word (name, var_is_special)
     char *name;
     int var_is_special;
{
  if (DIGIT (*name) && all_digits (name))
    return 1;
  else if (var_is_special)
    return 1;
#if defined (ARRAY_VARS)
  else if (valid_array_reference (name))
    return 1;
#endif /* ARRAY_VARS */
  else if (legal_identifier (name))
    return 1;
  else
    return 0;
}

static int
chk_atstar (name, quoted, quoted_dollar_atp, contains_dollar_at)
     char *name;
     int quoted;
     int *quoted_dollar_atp, *contains_dollar_at;
{
  char *temp1;

  if (name == 0)
    {
      if (quoted_dollar_atp)
	*quoted_dollar_atp = 0;
      if (contains_dollar_at)
	*contains_dollar_at = 0;
      return 0;
    }

  /* check for $@ and $* */
  if (name[0] == '@' && name[1] == 0)
    {
      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
	*quoted_dollar_atp = 1;
      if (contains_dollar_at)
	*contains_dollar_at = 1;
      return 1;
    }
  else if (name[0] == '*' && name[1] == '\0' && quoted == 0)
    {
      if (contains_dollar_at)
	*contains_dollar_at = 1;
      return 1;
    }

  /* Now check for ${array[@]} and ${array[*]} */
#if defined (ARRAY_VARS)
  else if (valid_array_reference (name))
    {
      temp1 = mbschr (name, '[');
      if (temp1 && temp1[1] == '@' && temp1[2] == ']')
	{
	  if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
	    *quoted_dollar_atp = 1;
	  if (contains_dollar_at)
	    *contains_dollar_at = 1;
	  return 1;
	}	/* [ */
      /* ${array[*]}, when unquoted, should be treated like ${array[@]},
	 which should result in separate words even when IFS is unset. */
      if (temp1 && temp1[1] == '*' && temp1[2] == ']' && quoted == 0)
	{
	  if (contains_dollar_at)
	    *contains_dollar_at = 1;
	  return 1;
	}
    }
#endif
  return 0;
}

/* Parameter expand NAME, and return a new string which is the expansion,
   or NULL if there was no expansion.
   VAR_IS_SPECIAL is non-zero if NAME is one of the special variables in
   the shell, e.g., "@", "$", "*", etc.  QUOTED, if non-zero, means that
   NAME was found inside of a double-quoted expression. */
static WORD_DESC *
parameter_brace_expand_word (name, var_is_special, quoted, pflags)
     char *name;
     int var_is_special, quoted, pflags;
{
  WORD_DESC *ret;
  char *temp, *tt;
  intmax_t arg_index;
  SHELL_VAR *var;
  int atype, rflags;

  ret = 0;
  temp = 0;
  rflags = 0;

  /* Handle multiple digit arguments, as in ${11}. */  
  if (legal_number (name, &arg_index))
    {
      tt = get_dollar_var_value (arg_index);
      if (tt)
 	temp = (*tt && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
 		  ? quote_string (tt)
 		  : quote_escapes (tt);
      else
        temp = (char *)NULL;
      FREE (tt);
    }
  else if (var_is_special)      /* ${@} */
    {
      int sindex;
      tt = (char *)xmalloc (2 + strlen (name));
      tt[sindex = 0] = '$';
      strcpy (tt + 1, name);

      ret = param_expand (tt, &sindex, quoted, (int *)NULL, (int *)NULL,
			  (int *)NULL, (int *)NULL, pflags);
      free (tt);
    }
#if defined (ARRAY_VARS)
  else if (valid_array_reference (name))
    {
      temp = array_value (name, quoted, &atype);
      if (atype == 0 && temp)
 	temp = (*temp && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
 		  ? quote_string (temp)
 		  : quote_escapes (temp);
      else if (atype == 1 && temp && QUOTED_NULL (temp) && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
	rflags |= W_HASQUOTEDNULL;
    }
#endif
  else if (var = find_variable (name))
    {
      if (var_isset (var) && invisible_p (var) == 0)
	{
#if defined (ARRAY_VARS)
	  if (assoc_p (var))
	    temp = assoc_reference (assoc_cell (var), "0");
	  else if (array_p (var))
	    temp = array_reference (array_cell (var), 0);
	  else
	    temp = value_cell (var);
#else
	  temp = value_cell (var);
#endif

	  if (temp)
	    temp = (*temp && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
		      ? quote_string (temp)
		      : quote_escapes (temp);
	}
      else
	temp = (char *)NULL;
    }
  else
    temp = (char *)NULL;

  if (ret == 0)
    {
      ret = alloc_word_desc ();
      ret->word = temp;
      ret->flags |= rflags;
    }
  return ret;
}

/* Expand an indirect reference to a variable: ${!NAME} expands to the
   value of the variable whose name is the value of NAME. */
static WORD_DESC *
parameter_brace_expand_indir (name, var_is_special, quoted, quoted_dollar_atp, contains_dollar_at)
     char *name;
     int var_is_special, quoted;
     int *quoted_dollar_atp, *contains_dollar_at;
{
  char *temp, *t;
  WORD_DESC *w;

  w = parameter_brace_expand_word (name, var_is_special, quoted, PF_IGNUNBOUND);
  t = w->word;
  /* Have to dequote here if necessary */
  if (t)
    {
      temp = (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT))
		? dequote_string (t)
		: dequote_escapes (t);
      free (t);
      t = temp;
    }
  dispose_word_desc (w);

  chk_atstar (t, quoted, quoted_dollar_atp, contains_dollar_at);
  if (t == 0)
    return (WORD_DESC *)NULL;

  w = parameter_brace_expand_word (t, SPECIAL_VAR(t, 0), quoted, 0);
  free (t);

  return w;
}

/* Expand the right side of a parameter expansion of the form ${NAMEcVALUE},
   depending on the value of C, the separating character.  C can be one of
   "-", "+", or "=".  QUOTED is true if the entire brace expression occurs
   between double quotes. */
static WORD_DESC *
parameter_brace_expand_rhs (name, value, c, quoted, qdollaratp, hasdollarat)
     char *name, *value;
     int c, quoted, *qdollaratp, *hasdollarat;
{
  WORD_DESC *w;
  WORD_LIST *l;
  char *t, *t1, *temp;
  int hasdol;

  /* If the entire expression is between double quotes, we want to treat
     the value as a double-quoted string, with the exception that we strip
     embedded unescaped double quotes (for sh backwards compatibility). */
  if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && *value)
    {
      hasdol = 0;
      temp = string_extract_double_quoted (value, &hasdol, 1);
    }
  else
    temp = value;

  w = alloc_word_desc ();
  hasdol = 0;
  /* XXX was 0 not quoted */
  l = *temp ? expand_string_for_rhs (temp, quoted, &hasdol, (int *)NULL)
	    : (WORD_LIST *)0;
  if (hasdollarat)
    *hasdollarat = hasdol || (l && l->next);
  if (temp != value)
    free (temp);
  if (l)
    {
      /* The expansion of TEMP returned something.  We need to treat things
	  slightly differently if HASDOL is non-zero.  If we have "$@", the
	  individual words have already been quoted.  We need to turn them
	  into a string with the words separated by the first character of
	  $IFS without any additional quoting, so string_list_dollar_at won't
	  do the right thing.  We use string_list_dollar_star instead. */
      temp = (hasdol || l->next) ? string_list_dollar_star (l) : string_list (l);

      /* If l->next is not null, we know that TEMP contained "$@", since that
	 is the only expansion that creates more than one word. */
      if (qdollaratp && ((hasdol && quoted) || l->next))
	*qdollaratp = 1;
      dispose_words (l);
    }
  else if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && hasdol)
    {
      /* The brace expansion occurred between double quotes and there was
	 a $@ in TEMP.  It does not matter if the $@ is quoted, as long as
	 it does not expand to anything.  In this case, we want to return
	 a quoted empty string. */
      temp = make_quoted_char ('\0');
      w->flags |= W_HASQUOTEDNULL;
    }
  else
    temp = (char *)NULL;

  if (c == '-' || c == '+')
    {
      w->word = temp;
      return w;
    }

  /* c == '=' */
  t = temp ? savestring (temp) : savestring ("");
  t1 = dequote_string (t);
  free (t);
#if defined (ARRAY_VARS)
  if (valid_array_reference (name))
    assign_array_element (name, t1, 0);
  else
#endif /* ARRAY_VARS */
  bind_variable (name, t1, 0);
  free (t1);

  w->word = temp;
  return w;
}

/* Deal with the right hand side of a ${name:?value} expansion in the case
   that NAME is null or not set.  If VALUE is non-null it is expanded and
   used as the error message to print, otherwise a standard message is
   printed. */
static void
parameter_brace_expand_error (name, value)
     char *name, *value;
{
  WORD_LIST *l;
  char *temp;

  if (value && *value)
    {
      l = expand_string (value, 0);
      temp =  string_list (l);
      report_error ("%s: %s", name, temp ? temp : "");	/* XXX was value not "" */
      FREE (temp);
      dispose_words (l);
    }
  else
    report_error (_("%s: parameter null or not set"), name);

  /* Free the data we have allocated during this expansion, since we
     are about to longjmp out. */
  free (name);
  FREE (value);
}

/* Return 1 if NAME is something for which parameter_brace_expand_length is
   OK to do. */
static int
valid_length_expression (name)
     char *name;
{
  return (name[1] == '\0' ||					/* ${#} */
	  ((sh_syntaxtab[(unsigned char) name[1]] & CSPECVAR) && name[2] == '\0') ||  /* special param */
	  (DIGIT (name[1]) && all_digits (name + 1)) ||	/* ${#11} */
#if defined (ARRAY_VARS)
	  valid_array_reference (name + 1) ||			/* ${#a[7]} */
#endif
	  legal_identifier (name + 1));				/* ${#PS1} */
}

#if defined (HANDLE_MULTIBYTE)
size_t
mbstrlen (s)
     const char *s;
{
  size_t clen, nc;
  mbstate_t mbs, mbsbak;

  nc = 0;
  memset (&mbs, 0, sizeof (mbs));
  mbsbak = mbs;
  while ((clen = mbrlen(s, MB_CUR_MAX, &mbs)) != 0)
    {
      if (MB_INVALIDCH(clen))
        {
	  clen = 1;	/* assume single byte */
	  mbs = mbsbak;
        }

      s += clen;
      nc++;
      mbsbak = mbs;
    }
  return nc;
}
#endif
      

/* Handle the parameter brace expansion that requires us to return the
   length of a parameter. */
static intmax_t
parameter_brace_expand_length (name)
     char *name;
{
  char *t, *newname;
  intmax_t number, arg_index;
  WORD_LIST *list;
#if defined (ARRAY_VARS)
  SHELL_VAR *var;
#endif

  if (name[1] == '\0')			/* ${#} */
    number = number_of_args ();
  else if ((name[1] == '@' || name[1] == '*') && name[2] == '\0')	/* ${#@}, ${#*} */
    number = number_of_args ();
  else if ((sh_syntaxtab[(unsigned char) name[1]] & CSPECVAR) && name[2] == '\0')
    {
      /* Take the lengths of some of the shell's special parameters. */
      switch (name[1])
	{
	case '-':
	  t = which_set_flags ();
	  break;
	case '?':
	  t = itos (last_command_exit_value);
	  break;
	case '$':
	  t = itos (dollar_dollar_pid);
	  break;
	case '!':
	  if (last_asynchronous_pid == NO_PID)
	    t = (char *)NULL;
	  else
	    t = itos (last_asynchronous_pid);
	  break;
	case '#':
	  t = itos (number_of_args ());
	  break;
	}
      number = STRLEN (t);
      FREE (t);
    }
#if defined (ARRAY_VARS)
  else if (valid_array_reference (name + 1))
    number = array_length_reference (name + 1);
#endif /* ARRAY_VARS */
  else
    {
      number = 0;

      if (legal_number (name + 1, &arg_index))		/* ${#1} */
	{
	  t = get_dollar_var_value (arg_index);
	  number = MB_STRLEN (t);
	  FREE (t);
	}
#if defined (ARRAY_VARS)
      else if ((var = find_variable (name + 1)) && (invisible_p (var) == 0) && (array_p (var) || assoc_p (var)))
	{
	  if (assoc_p (var))
	    t = assoc_reference (assoc_cell (var), "0");
	  else
	    t = array_reference (array_cell (var), 0);
	  number = MB_STRLEN (t);
	}
#endif
      else				/* ${#PS1} */
	{
	  newname = savestring (name);
	  newname[0] = '$';
	  list = expand_string (newname, Q_DOUBLE_QUOTES);
	  t = list ? string_list (list) : (char *)NULL;
	  free (newname);
	  if (list)
	    dispose_words (list);

	  number = MB_STRLEN (t);
	  FREE (t);
	}
    }

  return (number);
}

/* Skip characters in SUBSTR until DELIM.  SUBSTR is an arithmetic expression,
   so we do some ad-hoc parsing of an arithmetic expression to find
   the first DELIM, instead of using strchr(3).  Two rules:
	1.  If the substring contains a `(', read until closing `)'.
	2.  If the substring contains a `?', read past one `:' for each `?'.
*/

static char *
skiparith (substr, delim)
     char *substr;
     int delim;
{
  size_t sublen;
  int skipcol, pcount, i;
  DECLARE_MBSTATE;

  sublen = strlen (substr);
  i = skipcol = pcount = 0;
  while (substr[i])
    {
      /* Balance parens */
      if (substr[i] == LPAREN)
	{
	  pcount++;
	  i++;
	  continue;
	}
      if (substr[i] == RPAREN && pcount)
	{
	  pcount--;
	  i++;
	  continue;
	}
      if (pcount)
	{
	  ADVANCE_CHAR (substr, sublen, i);
	  continue;
	}

      /* Skip one `:' for each `?' */
      if (substr[i] == ':' && skipcol)
	{
	  skipcol--;
	  i++;
	  continue;
	}
      if (substr[i] == delim)
	break;
      if (substr[i] == '?')
	{
	  skipcol++;
	  i++;
	  continue;
	}
      ADVANCE_CHAR (substr, sublen, i);
    }

  return (substr + i);
}

/* Verify and limit the start and end of the desired substring.  If
   VTYPE == 0, a regular shell variable is being used; if it is 1,
   then the positional parameters are being used; if it is 2, then
   VALUE is really a pointer to an array variable that should be used.
   Return value is 1 if both values were OK, 0 if there was a problem
   with an invalid expression, or -1 if the values were out of range. */
static int
verify_substring_values (v, value, substr, vtype, e1p, e2p)
     SHELL_VAR *v;
     char *value, *substr;
     int vtype;
     intmax_t *e1p, *e2p;
{
  char *t, *temp1, *temp2;
  arrayind_t len;
  int expok;
#if defined (ARRAY_VARS)
 ARRAY *a;
 HASH_TABLE *h;
#endif

  /* duplicate behavior of strchr(3) */
  t = skiparith (substr, ':');
  if (*t && *t == ':')
    *t = '\0';
  else
    t = (char *)0;

  temp1 = expand_arith_string (substr, Q_DOUBLE_QUOTES);
  *e1p = evalexp (temp1, &expok);
  free (temp1);
  if (expok == 0)
    return (0);

  len = -1;	/* paranoia */
  switch (vtype)
    {
    case VT_VARIABLE:
    case VT_ARRAYMEMBER:
      len = MB_STRLEN (value);
      break;
    case VT_POSPARMS:
      len = number_of_args () + 1;
      if (*e1p == 0)
	len++;		/* add one arg if counting from $0 */
      break;
#if defined (ARRAY_VARS)
    case VT_ARRAYVAR:
      /* For arrays, the first value deals with array indices.  Negative
	 offsets count from one past the array's maximum index.  Associative
	 arrays treat the number of elements as the maximum index. */
      if (assoc_p (v))
	{
	  h = assoc_cell (v);
	  len = assoc_num_elements (h) + (*e1p < 0);
	}
      else
	{
	  a = (ARRAY *)value;
	  len = array_max_index (a) + (*e1p < 0);	/* arrays index from 0 to n - 1 */
	}
      break;
#endif
    }

  if (len == -1)	/* paranoia */
    return -1;

  if (*e1p < 0)		/* negative offsets count from end */
    *e1p += len;

  if (*e1p > len || *e1p < 0)
    return (-1);

#if defined (ARRAY_VARS)
  /* For arrays, the second offset deals with the number of elements. */
  if (vtype == VT_ARRAYVAR)
    len = assoc_p (v) ? assoc_num_elements (h) : array_num_elements (a);
#endif

  if (t)
    {
      t++;
      temp2 = savestring (t);
      temp1 = expand_arith_string (temp2, Q_DOUBLE_QUOTES);
      free (temp2);
      t[-1] = ':';
      *e2p = evalexp (temp1, &expok);
      free (temp1);
      if (expok == 0)
	return (0);
      if (*e2p < 0)
	{
	  internal_error (_("%s: substring expression < 0"), t);
	  return (0);
	}
#if defined (ARRAY_VARS)
      /* In order to deal with sparse arrays, push the intelligence about how
	 to deal with the number of elements desired down to the array-
	 specific functions.  */
      if (vtype != VT_ARRAYVAR)
#endif
	{
	  *e2p += *e1p;		/* want E2 chars starting at E1 */
	  if (*e2p > len)
	    *e2p = len;
	}
    }
  else
    *e2p = len;

  return (1);
}

/* Return the type of variable specified by VARNAME (simple variable,
   positional param, or array variable).  Also return the value specified
   by VARNAME (value of a variable or a reference to an array element).
   If this returns VT_VARIABLE, the caller assumes that CTLESC and CTLNUL
   characters in the value are quoted with CTLESC and takes appropriate
   steps.  For convenience, *VALP is set to the dequoted VALUE. */
static int
get_var_and_type (varname, value, quoted, varp, valp)
     char *varname, *value;
     int quoted;
     SHELL_VAR **varp;
     char **valp;
{
  int vtype;
  char *temp;
#if defined (ARRAY_VARS)
  SHELL_VAR *v;
#endif

  /* This sets vtype to VT_VARIABLE or VT_POSPARMS */
  vtype = (varname[0] == '@' || varname[0] == '*') && varname[1] == '\0';
  if (vtype == VT_POSPARMS && varname[0] == '*')
    vtype |= VT_STARSUB;
  *varp = (SHELL_VAR *)NULL;

#if defined (ARRAY_VARS)
  if (valid_array_reference (varname))
    {
      v = array_variable_part (varname, &temp, (int *)0);
      if (v && (array_p (v) || assoc_p (v)))
	{ /* [ */
	  if (ALL_ELEMENT_SUB (temp[0]) && temp[1] == ']')
	    {
	      /* Callers have to differentiate betwen indexed and associative */
	      vtype = VT_ARRAYVAR;
	      if (temp[0] == '*')
		vtype |= VT_STARSUB;
	      *valp = array_p (v) ? (char *)array_cell (v) : (char *)assoc_cell (v);
	    }
	  else
	    {
	      vtype = VT_ARRAYMEMBER;
	      *valp = array_value (varname, 1, (int *)NULL);
	    }
	  *varp = v;
	}
      else if (v && (ALL_ELEMENT_SUB (temp[0]) && temp[1] == ']'))
	{
	  vtype = VT_VARIABLE;
	  *varp = v;
	  if (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT))
	    *valp = dequote_string (value);
	  else
	    *valp = dequote_escapes (value);
	}
      else
	{
	  vtype = VT_ARRAYMEMBER;
	  *varp = v;
	  *valp = array_value (varname, 1, (int *)NULL);
	}
    }
  else if ((v = find_variable (varname)) && (invisible_p (v) == 0) && (assoc_p (v) || array_p (v)))
    {
      vtype = VT_ARRAYMEMBER;
      *varp = v;
      *valp = assoc_p (v) ? assoc_reference (assoc_cell (v), "0") : array_reference (array_cell (v), 0);
    }
  else
#endif
    {
      if (value && vtype == VT_VARIABLE)
	{
	  if (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT))
	    *valp = dequote_string (value);
	  else
	    *valp = dequote_escapes (value);
	}
      else
	*valp = value;
    }

  return vtype;
}

/******************************************************/
/*						      */
/* Functions to extract substrings of variable values */
/*						      */
/******************************************************/

#if defined (HANDLE_MULTIBYTE)
/* Character-oriented rather than strictly byte-oriented substrings.  S and
   E, rather being strict indices into STRING, indicate character (possibly
   multibyte character) positions that require calculation.
   Used by the ${param:offset[:length]} expansion. */
static char *
mb_substring (string, s, e)
     char *string;
     int s, e;
{
  char *tt;
  int start, stop, i, slen;
  DECLARE_MBSTATE;

  start = 0;
  /* Don't need string length in ADVANCE_CHAR unless multibyte chars possible. */
  slen = (MB_CUR_MAX > 1) ? STRLEN (string) : 0;

  i = s;
  while (string[start] && i--)
    ADVANCE_CHAR (string, slen, start);
  stop = start;
  i = e - s;
  while (string[stop] && i--)
    ADVANCE_CHAR (string, slen, stop);
  tt = substring (string, start, stop);
  return tt;
}
#endif
  
/* Process a variable substring expansion: ${name:e1[:e2]}.  If VARNAME
   is `@', use the positional parameters; otherwise, use the value of
   VARNAME.  If VARNAME is an array variable, use the array elements. */

static char *
parameter_brace_substring (varname, value, substr, quoted)
     char *varname, *value, *substr;
     int quoted;
{
  intmax_t e1, e2;
  int vtype, r, starsub;
  char *temp, *val, *tt, *oname;
  SHELL_VAR *v;

  if (value == 0)
    return ((char *)NULL);

  oname = this_command_name;
  this_command_name = varname;

  vtype = get_var_and_type (varname, value, quoted, &v, &val);
  if (vtype == -1)
    {
      this_command_name = oname;
      return ((char *)NULL);
    }

  starsub = vtype & VT_STARSUB;
  vtype &= ~VT_STARSUB;

  r = verify_substring_values (v, val, substr, vtype, &e1, &e2);
  this_command_name = oname;
  if (r <= 0)
    return ((r == 0) ? &expand_param_error : (char *)NULL);

  switch (vtype)
    {
    case VT_VARIABLE:
    case VT_ARRAYMEMBER:
#if defined (HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1)
	tt = mb_substring (val, e1, e2);
      else
#endif
      tt = substring (val, e1, e2);

      if (vtype == VT_VARIABLE)
	FREE (val);
      if (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT))
	temp = quote_string (tt);
      else
	temp = tt ? quote_escapes (tt) : (char *)NULL;
      FREE (tt);
      break;
    case VT_POSPARMS:
      tt = pos_params (varname, e1, e2, quoted);
      if ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) == 0)
	{
	  temp = tt ? quote_escapes (tt) : (char *)NULL;
	  FREE (tt);
	}
      else
	temp = tt;
      break;
#if defined (ARRAY_VARS)
    case VT_ARRAYVAR:
      if (assoc_p (v))
	/* we convert to list and take first e2 elements starting at e1th
	   element -- officially undefined for now */	
	temp = assoc_subrange (assoc_cell (v), e1, e2, starsub, quoted);
      else
      /* We want E2 to be the number of elements desired (arrays can be sparse,
	 so verify_substring_values just returns the numbers specified and we
	 rely on array_subrange to understand how to deal with them). */
	temp = array_subrange (array_cell (v), e1, e2, starsub, quoted);
      /* array_subrange now calls array_quote_escapes as appropriate, so the
	 caller no longer needs to. */
      break;
#endif
    default:
      temp = (char *)NULL;
    }

  return temp;
}

/****************************************************************/
/*								*/
/* Functions to perform pattern substitution on variable values */
/*								*/
/****************************************************************/

char *
pat_subst (string, pat, rep, mflags)
     char *string, *pat, *rep;
     int mflags;
{
  char *ret, *s, *e, *str;
  int rsize, rptr, l, replen, mtype;

  mtype = mflags & MATCH_TYPEMASK;

  /* Special cases:
   * 	1.  A null pattern with mtype == MATCH_BEG means to prefix STRING
   *	    with REP and return the result.
   *	2.  A null pattern with mtype == MATCH_END means to append REP to
   *	    STRING and return the result.
   */
  if ((pat == 0 || *pat == 0) && (mtype == MATCH_BEG || mtype == MATCH_END))
    {
      replen = STRLEN (rep);
      l = strlen (string);
      ret = (char *)xmalloc (replen + l + 2);
      if (replen == 0)
	strcpy (ret, string);
      else if (mtype == MATCH_BEG)
	{
	  strcpy (ret, rep);
	  strcpy (ret + replen, string);
	}
      else
	{
	  strcpy (ret, string);
	  strcpy (ret + l, rep);
	}
      return (ret);
    }

  ret = (char *)xmalloc (rsize = 64);
  ret[0] = '\0';

  for (replen = STRLEN (rep), rptr = 0, str = string;;)
    {
      if (match_pattern (str, pat, mtype, &s, &e) == 0)
	break;
      l = s - str;
      RESIZE_MALLOCED_BUFFER (ret, rptr, (l + replen), rsize, 64);

      /* OK, now copy the leading unmatched portion of the string (from
	 str to s) to ret starting at rptr (the current offset).  Then copy
	 the replacement string at ret + rptr + (s - str).  Increment
	 rptr (if necessary) and str and go on. */
      if (l)
	{
	  strncpy (ret + rptr, str, l);
	  rptr += l;
	}
      if (replen)
	{
	  strncpy (ret + rptr, rep, replen);
	  rptr += replen;
	}
      str = e;		/* e == end of match */

      if (((mflags & MATCH_GLOBREP) == 0) || mtype != MATCH_ANY)
	break;

      if (s == e)
	{
	  /* On a zero-length match, make sure we copy one character, since
	     we increment one character to avoid infinite recursion. */
	  RESIZE_MALLOCED_BUFFER (ret, rptr, 1, rsize, 64);
	  ret[rptr++] = *str++;
	  e++;		/* avoid infinite recursion on zero-length match */
	}
    }

  /* Now copy the unmatched portion of the input string */
  if (*str)
    {
      RESIZE_MALLOCED_BUFFER (ret, rptr, STRLEN(str) + 1, rsize, 64);
      strcpy (ret + rptr, str);
    }
  else
    ret[rptr] = '\0';

  return ret;
}

/* Do pattern match and replacement on the positional parameters. */
static char *
pos_params_pat_subst (string, pat, rep, mflags)
     char *string, *pat, *rep;
     int mflags;
{
  WORD_LIST *save, *params;
  WORD_DESC *w;
  char *ret;
  int pchar, qflags;

  save = params = list_rest_of_args ();
  if (save == 0)
    return ((char *)NULL);

  for ( ; params; params = params->next)
    {
      ret = pat_subst (params->word->word, pat, rep, mflags);
      w = alloc_word_desc ();
      w->word = ret ? ret : savestring ("");
      dispose_word (params->word);
      params->word = w;
    }

  pchar = (mflags & MATCH_STARSUB) == MATCH_STARSUB ? '*' : '@';
  qflags = (mflags & MATCH_QUOTED) == MATCH_QUOTED ? Q_DOUBLE_QUOTES : 0;

#if 0
  if ((mflags & (MATCH_QUOTED|MATCH_STARSUB)) == (MATCH_QUOTED|MATCH_STARSUB))
    ret = string_list_dollar_star (quote_list (save));
  else if ((mflags & MATCH_STARSUB) == MATCH_STARSUB)
    ret = string_list_dollar_star (save);
  else if ((mflags & MATCH_QUOTED) == MATCH_QUOTED)
    ret = string_list_dollar_at (save, qflags);
  else
    ret = string_list_dollar_star (save);
#else
  ret = string_list_pos_params (pchar, save, qflags);
#endif

  dispose_words (save);

  return (ret);
}

/* Perform pattern substitution on VALUE, which is the expansion of
   VARNAME.  PATSUB is an expression supplying the pattern to match
   and the string to substitute.  QUOTED is a flags word containing
   the type of quoting currently in effect. */
static char *
parameter_brace_patsub (varname, value, patsub, quoted)
     char *varname, *value, *patsub;
     int quoted;
{
  int vtype, mflags, starsub, delim;
  char *val, *temp, *pat, *rep, *p, *lpatsub, *tt;
  SHELL_VAR *v;

  if (value == 0)
    return ((char *)NULL);

  this_command_name = varname;

  vtype = get_var_and_type (varname, value, quoted, &v, &val);
  if (vtype == -1)
    return ((char *)NULL);

  starsub = vtype & VT_STARSUB;
  vtype &= ~VT_STARSUB;

  mflags = 0;
  if (patsub && *patsub == '/')
    {
      mflags |= MATCH_GLOBREP;
      patsub++;
    }

  /* Malloc this because expand_string_if_necessary or one of the expansion
     functions in its call chain may free it on a substitution error. */
  lpatsub = savestring (patsub);

  if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
    mflags |= MATCH_QUOTED;

  if (starsub)
    mflags |= MATCH_STARSUB;

  /* If the pattern starts with a `/', make sure we skip over it when looking
     for the replacement delimiter. */
#if 0
  if (rep = quoted_strchr ((*patsub == '/') ? lpatsub+1 : lpatsub, '/', ST_BACKSL))
    *rep++ = '\0';
  else
    rep = (char *)NULL;
#else
  delim = skip_to_delim (lpatsub, ((*patsub == '/') ? 1 : 0), "/", 0);
  if (lpatsub[delim] == '/')
    {
      lpatsub[delim] = 0;
      rep = lpatsub + delim + 1;
    }
  else
    rep = (char *)NULL;
#endif

  if (rep && *rep == '\0')
    rep = (char *)NULL;

  /* Perform the same expansions on the pattern as performed by the
     pattern removal expansions. */
  pat = getpattern (lpatsub, quoted, 1);

  if (rep)
    {
      if ((mflags & MATCH_QUOTED) == 0)
	rep = expand_string_if_necessary (rep, quoted, expand_string_unsplit);
      else
	rep = expand_string_to_string_internal (rep, quoted, expand_string_unsplit);
    }

  /* ksh93 doesn't allow the match specifier to be a part of the expanded
     pattern.  This is an extension.  Make sure we don't anchor the pattern
     at the beginning or end of the string if we're doing global replacement,
     though. */
  p = pat;
  if (mflags & MATCH_GLOBREP)
    mflags |= MATCH_ANY;
  else if (pat && pat[0] == '#')
    {
      mflags |= MATCH_BEG;
      p++;
    }
  else if (pat && pat[0] == '%')
    {
      mflags |= MATCH_END;
      p++;
    }
  else
    mflags |= MATCH_ANY;

  /* OK, we now want to substitute REP for PAT in VAL.  If
     flags & MATCH_GLOBREP is non-zero, the substitution is done
     everywhere, otherwise only the first occurrence of PAT is
     replaced.  The pattern matching code doesn't understand
     CTLESC quoting CTLESC and CTLNUL so we use the dequoted variable
     values passed in (VT_VARIABLE) so the pattern substitution
     code works right.  We need to requote special chars after
     we're done for VT_VARIABLE and VT_ARRAYMEMBER, and for the
     other cases if QUOTED == 0, since the posparams and arrays
     indexed by * or @ do special things when QUOTED != 0. */

  switch (vtype)
    {
    case VT_VARIABLE:
    case VT_ARRAYMEMBER:
      temp = pat_subst (val, p, rep, mflags);
      if (vtype == VT_VARIABLE)
	FREE (val);
      if (temp)
	{
	  tt = (mflags & MATCH_QUOTED) ? quote_string (temp) : quote_escapes (temp);
	  free (temp);
	  temp = tt;
	}
      break;
    case VT_POSPARMS:
      temp = pos_params_pat_subst (val, p, rep, mflags);
      if (temp && (mflags & MATCH_QUOTED) == 0)
	{
	  tt = quote_escapes (temp);
	  free (temp);
	  temp = tt;
	}
      break;
#if defined (ARRAY_VARS)
    case VT_ARRAYVAR:
      temp = assoc_p (v) ? assoc_patsub (assoc_cell (v), p, rep, mflags)
			 : array_patsub (array_cell (v), p, rep, mflags);
      /* Don't call quote_escapes anymore; array_patsub calls
	 array_quote_escapes as appropriate before adding the
	 space separators; ditto for assoc_patsub. */
      break;
#endif
    }

  FREE (pat);
  FREE (rep);
  free (lpatsub);

  return temp;
}

/****************************************************************/
/*								*/
/*   Functions to perform case modification on variable values  */
/*								*/
/****************************************************************/

/* Do case modification on the positional parameters. */

static char *
pos_params_modcase (string, pat, modop, mflags)
     char *string, *pat;
     int modop;
     int mflags;
{
  WORD_LIST *save, *params;
  WORD_DESC *w;
  char *ret;
  int pchar, qflags;

  save = params = list_rest_of_args ();
  if (save == 0)
    return ((char *)NULL);

  for ( ; params; params = params->next)
    {
      ret = sh_modcase (params->word->word, pat, modop);
      w = alloc_word_desc ();
      w->word = ret ? ret : savestring ("");
      dispose_word (params->word);
      params->word = w;
    }

  pchar = (mflags & MATCH_STARSUB) == MATCH_STARSUB ? '*' : '@';
  qflags = (mflags & MATCH_QUOTED) == MATCH_QUOTED ? Q_DOUBLE_QUOTES : 0;

  ret = string_list_pos_params (pchar, save, qflags);
  dispose_words (save);

  return (ret);
}

/* Perform case modification on VALUE, which is the expansion of
   VARNAME.  MODSPEC is an expression supplying the type of modification
   to perform.  QUOTED is a flags word containing the type of quoting
   currently in effect. */
static char *
parameter_brace_casemod (varname, value, modspec, patspec, quoted)
     char *varname, *value;
     int modspec;
     char *patspec;
     int quoted;
{
  int vtype, starsub, modop, mflags, x;
  char *val, *temp, *pat, *p, *lpat, *tt;
  SHELL_VAR *v;

  if (value == 0)
    return ((char *)NULL);

  this_command_name = varname;

  vtype = get_var_and_type (varname, value, quoted, &v, &val);
  if (vtype == -1)
    return ((char *)NULL);

  starsub = vtype & VT_STARSUB;
  vtype &= ~VT_STARSUB;

  modop = 0;
  mflags = 0;
  if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
    mflags |= MATCH_QUOTED;
  if (starsub)
    mflags |= MATCH_STARSUB;
  
  p = patspec;
  if (modspec == '^')
    {
      x = p && p[0] == modspec;
      modop = x ? CASE_UPPER : CASE_UPFIRST;
      p += x;
    }
  else if (modspec == ',')
    {
      x = p && p[0] == modspec;
      modop = x ? CASE_LOWER : CASE_LOWFIRST;
      p += x;
    }
  else if (modspec == '~')
    {
      x = p && p[0] == modspec;
      modop = x ? CASE_TOGGLEALL : CASE_TOGGLE;
      p += x;
    }
    
  lpat = p ? savestring (p) : 0;
  /* Perform the same expansions on the pattern as performed by the
     pattern removal expansions.  FOR LATER */
  pat = lpat ? getpattern (lpat, quoted, 1) : 0;

  /* OK, now we do the case modification. */
  switch (vtype)
    {
    case VT_VARIABLE:
    case VT_ARRAYMEMBER:
      temp = sh_modcase (val, pat, modop);
      if (vtype == VT_VARIABLE)
	FREE (val);
      if (temp)
	{
	  tt = (mflags & MATCH_QUOTED) ? quote_string (temp) : quote_escapes (temp);
	  free (temp);
	  temp = tt;
	}
      break;

    case VT_POSPARMS:
      temp = pos_params_modcase (val, pat, modop, mflags);
      if (temp && (mflags & MATCH_QUOTED)  == 0)
	{
	  tt = quote_escapes (temp);
	  free (temp);
	  temp = tt;
	}
      break;

#if defined (ARRAY_VARS)
    case VT_ARRAYVAR:
      temp = assoc_p (v) ? assoc_modcase (assoc_cell (v), pat, modop, mflags)
			 : array_modcase (array_cell (v), pat, modop, mflags);
      /* Don't call quote_escapes; array_modcase calls array_quote_escapes
	 as appropriate before adding the space separators; ditto for
	 assoc_modcase. */
      break;
#endif
    }

  FREE (pat);
  free (lpat);

  return temp;
}

/* Check for unbalanced parens in S, which is the contents of $(( ... )).  If
   any occur, this must be a nested command substitution, so return 0.
   Otherwise, return 1.  A valid arithmetic expression must always have a
   ( before a matching ), so any cases where there are more right parens
   means that this must not be an arithmetic expression, though the parser
   will not accept it without a balanced total number of parens. */
static int
chk_arithsub (s, len)
     const char *s;
     int len;
{
  int i, count;
  DECLARE_MBSTATE;

  i = count = 0;
  while (i < len)
    {
      if (s[i] == LPAREN)
	count++;
      else if (s[i] == RPAREN)
	{
	  count--;
	  if (count < 0)
	    return 0;
	}

      switch (s[i])
	{
	default:
	  ADVANCE_CHAR (s, len, i);
	  break;

	case '\\':
	  i++;
	  if (s[i])
	    ADVANCE_CHAR (s, len, i);
	  break;

	case '\'':
	  i = skip_single_quoted (s, len, ++i);
	  break;

	case '"':
	  i = skip_double_quoted ((char *)s, len, ++i);
	  break;
	}
    }

  return (count == 0);
}

/****************************************************************/
/*								*/
/*	Functions to perform parameter expansion on a string	*/
/*								*/
/****************************************************************/

/* ${[#][!]name[[:][^[^]][,[,]]#[#]%[%]-=?+[word][:e1[:e2]]]} */
static WORD_DESC *
parameter_brace_expand (string, indexp, quoted, pflags, quoted_dollar_atp, contains_dollar_at)
     char *string;
     int *indexp, quoted, *quoted_dollar_atp, *contains_dollar_at, pflags;
{
  int check_nullness, var_is_set, var_is_null, var_is_special;
  int want_substring, want_indir, want_patsub, want_casemod;
  char *name, *value, *temp, *temp1;
  WORD_DESC *tdesc, *ret;
  int t_index, sindex, c, tflag, modspec;
  intmax_t number;

  temp = temp1 = value = (char *)NULL;
  var_is_set = var_is_null = var_is_special = check_nullness = 0;
  want_substring = want_indir = want_patsub = want_casemod = 0;

  sindex = *indexp;
  t_index = ++sindex;
  /* ${#var} doesn't have any of the other parameter expansions on it. */
  if (string[t_index] == '#' && legal_variable_starter (string[t_index+1]))		/* {{ */
    name = string_extract (string, &t_index, "}", SX_VARNAME);
  else
#if defined (CASEMOD_EXPANSIONS)
    /* To enable case-toggling expansions using the `~' operator character
       change the 1 to 0. */
#  if defined (CASEMOD_CAPCASE)
    name = string_extract (string, &t_index, "#%^,~:-=?+/}", SX_VARNAME);
#  else
    name = string_extract (string, &t_index, "#%^,:-=?+/}", SX_VARNAME);
#  endif /* CASEMOD_CAPCASE */
#else
    name = string_extract (string, &t_index, "#%:-=?+/}", SX_VARNAME);
#endif /* CASEMOD_EXPANSIONS */

  ret = 0;
  tflag = 0;

  /* If the name really consists of a special variable, then make sure
     that we have the entire name.  We don't allow indirect references
     to special variables except `#', `?', `@' and `*'. */
  if ((sindex == t_index &&
	(string[t_index] == '-' ||
	 string[t_index] == '?' ||
	 string[t_index] == '#')) ||
      (sindex == t_index - 1 && string[sindex] == '!' &&
	(string[t_index] == '#' ||
	 string[t_index] == '?' ||
	 string[t_index] == '@' ||
	 string[t_index] == '*')))
    {
      t_index++;
      free (name);
      temp1 = string_extract (string, &t_index, "#%:-=?+/}", 0);
      name = (char *)xmalloc (3 + (strlen (temp1)));
      *name = string[sindex];
      if (string[sindex] == '!')
	{
	  /* indirect reference of $#, $?, $@, or $* */
	  name[1] = string[sindex + 1];
	  strcpy (name + 2, temp1);
	}
      else	
	strcpy (name + 1, temp1);
      free (temp1);
    }
  sindex = t_index;

  /* Find out what character ended the variable name.  Then
     do the appropriate thing. */
  if (c = string[sindex])
    sindex++;

  /* If c is followed by one of the valid parameter expansion
     characters, move past it as normal.  If not, assume that
     a substring specification is being given, and do not move
     past it. */
  if (c == ':' && VALID_PARAM_EXPAND_CHAR (string[sindex]))
    {
      check_nullness++;
      if (c = string[sindex])
	sindex++;
    }
  else if (c == ':' && string[sindex] != RBRACE)
    want_substring = 1;
  else if (c == '/' && string[sindex] != RBRACE)
    want_patsub = 1;
#if defined (CASEMOD_EXPANSIONS)
  else if (c == '^' || c == ',' || c == '~')
    {
      modspec = c;
      want_casemod = 1;
    }
#endif

  /* Catch the valid and invalid brace expressions that made it through the
     tests above. */
  /* ${#-} is a valid expansion and means to take the length of $-.
     Similarly for ${#?} and ${##}... */
  if (name[0] == '#' && name[1] == '\0' && check_nullness == 0 &&
	VALID_SPECIAL_LENGTH_PARAM (c) && string[sindex] == RBRACE)
    {
      name = (char *)xrealloc (name, 3);
      name[1] = c;
      name[2] = '\0';
      c = string[sindex++];
    }

  /* ...but ${#%}, ${#:}, ${#=}, ${#+}, and ${#/} are errors. */
  if (name[0] == '#' && name[1] == '\0' && check_nullness == 0 &&
	member (c, "%:=+/") && string[sindex] == RBRACE)
    {
      temp = (char *)NULL;
      goto bad_substitution;
    }

  /* Indirect expansion begins with a `!'.  A valid indirect expansion is
     either a variable name, one of the positional parameters or a special
     variable that expands to one of the positional parameters. */
  want_indir = *name == '!' &&
    (legal_variable_starter ((unsigned char)name[1]) || DIGIT (name[1])
					|| VALID_INDIR_PARAM (name[1]));

  /* Determine the value of this variable. */

  /* Check for special variables, directly referenced. */
  if (SPECIAL_VAR (name, want_indir))
    var_is_special++;

  /* Check for special expansion things, like the length of a parameter */
  if (*name == '#' && name[1])
    {
      /* If we are not pointing at the character just after the
	 closing brace, then we haven't gotten all of the name.
	 Since it begins with a special character, this is a bad
	 substitution.  Also check NAME for validity before trying
	 to go on. */
      if (string[sindex - 1] != RBRACE || (valid_length_expression (name) == 0))
	{
	  temp = (char *)NULL;
	  goto bad_substitution;
	}

      number = parameter_brace_expand_length (name);
      free (name);

      *indexp = sindex;
      if (number < 0)
        return (&expand_wdesc_error);
      else
	{
	  ret = alloc_word_desc ();
	  ret->word = itos (number);
	  return ret;
	}
    }

  /* ${@} is identical to $@. */
  if (name[0] == '@' && name[1] == '\0')
    {
      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
	*quoted_dollar_atp = 1;

      if (contains_dollar_at)
	*contains_dollar_at = 1;
    }

  /* Process ${!PREFIX*} expansion. */
  if (want_indir && string[sindex - 1] == RBRACE &&
      (string[sindex - 2] == '*' || string[sindex - 2] == '@') &&
      legal_variable_starter ((unsigned char) name[1]))
    {
      char **x;
      WORD_LIST *xlist;

      temp1 = savestring (name + 1);
      number = strlen (temp1);
      temp1[number - 1] = '\0';
      x = all_variables_matching_prefix (temp1);
      xlist = strvec_to_word_list (x, 0, 0);
      if (string[sindex - 2] == '*')
	temp = string_list_dollar_star (xlist);
      else
	{
	  temp = string_list_dollar_at (xlist, quoted);
	  if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
	    *quoted_dollar_atp = 1;
	  if (contains_dollar_at)
	    *contains_dollar_at = 1;
	}
      free (x);
      dispose_words (xlist);
      free (temp1);
      *indexp = sindex;

      ret = alloc_word_desc ();
      ret->word = temp;
      return ret;
    }

#if defined (ARRAY_VARS)      
  /* Process ${!ARRAY[@]} and ${!ARRAY[*]} expansion. */ /* [ */
  if (want_indir && string[sindex - 1] == RBRACE &&
      string[sindex - 2] == ']' && valid_array_reference (name+1))
    {
      char *x, *x1;

      temp1 = savestring (name + 1);
      x = array_variable_name (temp1, &x1, (int *)0);	/* [ */
      FREE (x);
      if (ALL_ELEMENT_SUB (x1[0]) && x1[1] == ']')
	{
	  temp = array_keys (temp1, quoted);	/* handles assoc vars too */
	  if (x1[0] == '@')
	    {
	      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
		*quoted_dollar_atp = 1;
	      if (contains_dollar_at)
		*contains_dollar_at = 1;
	    }	    

	  free (temp1);
	  *indexp = sindex;

	  ret = alloc_word_desc ();
	  ret->word = temp;
	  return ret;
	}

      free (temp1);
    }
#endif /* ARRAY_VARS */
      
  /* Make sure that NAME is valid before trying to go on. */
  if (valid_brace_expansion_word (want_indir ? name + 1 : name,
					var_is_special) == 0)
    {
      temp = (char *)NULL;
      goto bad_substitution;
    }

  if (want_indir)
    tdesc = parameter_brace_expand_indir (name + 1, var_is_special, quoted, quoted_dollar_atp, contains_dollar_at);
  else
    tdesc = parameter_brace_expand_word (name, var_is_special, quoted, PF_IGNUNBOUND|(pflags&PF_NOSPLIT2));

  if (tdesc)
    {
      temp = tdesc->word;
      tflag = tdesc->flags;
      dispose_word_desc (tdesc);
    }
  else
    temp = (char  *)0;

#if defined (ARRAY_VARS)
  if (valid_array_reference (name))
    chk_atstar (name, quoted, quoted_dollar_atp, contains_dollar_at);
#endif

  var_is_set = temp != (char *)0;
  var_is_null = check_nullness && (var_is_set == 0 || *temp == 0);

  /* Get the rest of the stuff inside the braces. */
  if (c && c != RBRACE)
    {
      /* Extract the contents of the ${ ... } expansion
	 according to the Posix.2 rules. */
      value = extract_dollar_brace_string (string, &sindex, quoted, 0);
      if (string[sindex] == RBRACE)
	sindex++;
      else
	goto bad_substitution;
    }
  else
    value = (char *)NULL;

  *indexp = sindex;

  /* If this is a substring spec, process it and add the result. */
  if (want_substring)
    {
      temp1 = parameter_brace_substring (name, temp, value, quoted);
      FREE (name);
      FREE (value);
      FREE (temp);

      if (temp1 == &expand_param_error)
	return (&expand_wdesc_error);
      else if (temp1 == &expand_param_fatal)
	return (&expand_wdesc_fatal);

      ret = alloc_word_desc ();
      ret->word = temp1;
      if (temp1 && QUOTED_NULL (temp1) && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	ret->flags |= W_QUOTED|W_HASQUOTEDNULL;
      return ret;
    }
  else if (want_patsub)
    {
      temp1 = parameter_brace_patsub (name, temp, value, quoted);
      FREE (name);
      FREE (value);
      FREE (temp);

      if (temp1 == &expand_param_error)
	return (&expand_wdesc_error);
      else if (temp1 == &expand_param_fatal)
	return (&expand_wdesc_fatal);

      ret = alloc_word_desc ();
      ret->word = temp1;
      ret = alloc_word_desc ();
      ret->word = temp1;
      if (temp1 && QUOTED_NULL (temp1) && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	ret->flags |= W_QUOTED|W_HASQUOTEDNULL;
      return ret;
    }
#if defined (CASEMOD_EXPANSIONS)
  else if (want_casemod)
    {
      temp1 = parameter_brace_casemod (name, temp, modspec, value, quoted);
      FREE (name);
      FREE (value);
      FREE (temp);

      if (temp1 == &expand_param_error)
	return (&expand_wdesc_error);
      else if (temp1 == &expand_param_fatal)
	return (&expand_wdesc_fatal);

      ret = alloc_word_desc ();
      ret->word = temp1;
      if (temp1 && QUOTED_NULL (temp1) && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	ret->flags |= W_QUOTED|W_HASQUOTEDNULL;
      return ret;
    }
#endif

  /* Do the right thing based on which character ended the variable name. */
  switch (c)
    {
    default:
    case '\0':
    bad_substitution:
      report_error (_("%s: bad substitution"), string ? string : "??");
      FREE (value);
      FREE (temp);
      free (name);
      return &expand_wdesc_error;

    case RBRACE:
      if (var_is_set == 0 && unbound_vars_is_error && ((name[0] != '@' && name[0] != '*') || name[1]))
	{
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (name);
	  FREE (value);
	  FREE (temp);
	  free (name);
	  return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	}
      break;

    case '#':	/* ${param#[#]pattern} */
    case '%':	/* ${param%[%]pattern} */
      if (value == 0 || *value == '\0' || temp == 0 || *temp == '\0')
	{
	  FREE (value);
	  break;
	}
      temp1 = parameter_brace_remove_pattern (name, temp, value, c, quoted);
      free (temp);
      free (value);

      ret = alloc_word_desc ();
      ret->word = temp1;
      if (temp1 && QUOTED_NULL (temp1) && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	ret->flags |= W_QUOTED|W_HASQUOTEDNULL;
      return ret;

    case '-':
    case '=':
    case '?':
    case '+':
      if (var_is_set && var_is_null == 0)
	{
	  /* If the operator is `+', we don't want the value of the named
	     variable for anything, just the value of the right hand side. */

	  if (c == '+')
	    {
	      /* XXX -- if we're double-quoted and the named variable is "$@",
			we want to turn off any special handling of "$@" --
			we're not using it, so whatever is on the rhs applies. */
	      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
		*quoted_dollar_atp = 0;
	      if (contains_dollar_at)
		*contains_dollar_at = 0;

	      FREE (temp);
	      if (value)
		{
		  ret = parameter_brace_expand_rhs (name, value, c,
						    quoted,
						    quoted_dollar_atp,
						    contains_dollar_at);
		  /* XXX - fix up later, esp. noting presence of
			   W_HASQUOTEDNULL in ret->flags */
		  free (value);
		}
	      else
		temp = (char *)NULL;
	    }
	  else
	    {
	      FREE (value);
	    }
	  /* Otherwise do nothing; just use the value in TEMP. */
	}
      else	/* VAR not set or VAR is NULL. */
	{
	  FREE (temp);
	  temp = (char *)NULL;
	  if (c == '=' && var_is_special)
	    {
	      report_error (_("$%s: cannot assign in this way"), name);
	      free (name);
	      free (value);
	      return &expand_wdesc_error;
	    }
	  else if (c == '?')
	    {
	      parameter_brace_expand_error (name, value);
	      return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	    }
	  else if (c != '+')
	    {
	      /* XXX -- if we're double-quoted and the named variable is "$@",
			we want to turn off any special handling of "$@" --
			we're not using it, so whatever is on the rhs applies. */
	      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
		*quoted_dollar_atp = 0;
	      if (contains_dollar_at)
		*contains_dollar_at = 0;

	      ret = parameter_brace_expand_rhs (name, value, c, quoted,
						quoted_dollar_atp,
						contains_dollar_at);
	      /* XXX - fix up later, esp. noting presence of
		       W_HASQUOTEDNULL in tdesc->flags */
	    }
	  free (value);
	}

      break;
    }
  free (name);

  if (ret == 0)
    {
      ret = alloc_word_desc ();
      ret->flags = tflag;
      ret->word = temp;
    }
  return (ret);
}

/* Expand a single ${xxx} expansion.  The braces are optional.  When
   the braces are used, parameter_brace_expand() does the work,
   possibly calling param_expand recursively. */
static WORD_DESC *
param_expand (string, sindex, quoted, expanded_something,
	      contains_dollar_at, quoted_dollar_at_p, had_quoted_null_p,
	      pflags)
     char *string;
     int *sindex, quoted, *expanded_something, *contains_dollar_at;
     int *quoted_dollar_at_p, *had_quoted_null_p, pflags;
{
  char *temp, *temp1, uerror[3];
  int zindex, t_index, expok;
  unsigned char c;
  intmax_t number;
  SHELL_VAR *var;
  WORD_LIST *list;
  WORD_DESC *tdesc, *ret;
  int tflag;

  zindex = *sindex;
  c = string[++zindex];

  temp = (char *)NULL;
  ret = tdesc = (WORD_DESC *)NULL;
  tflag = 0;

  /* Do simple cases first. Switch on what follows '$'. */
  switch (c)
    {
    /* $0 .. $9? */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      temp1 = dollar_vars[TODIGIT (c)];
      if (unbound_vars_is_error && temp1 == (char *)NULL)
	{
	  uerror[0] = '$';
	  uerror[1] = c;
	  uerror[2] = '\0';
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (uerror);
	  return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	}
      if (temp1)
	temp = (*temp1 && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
		  ? quote_string (temp1)
		  : quote_escapes (temp1);
      else
	temp = (char *)NULL;

      break;

    /* $$ -- pid of the invoking shell. */
    case '$':
      temp = itos (dollar_dollar_pid);
      break;

    /* $# -- number of positional parameters. */
    case '#':
      temp = itos (number_of_args ());
      break;

    /* $? -- return value of the last synchronous command. */
    case '?':
      temp = itos (last_command_exit_value);
      break;

    /* $- -- flags supplied to the shell on invocation or by `set'. */
    case '-':
      temp = which_set_flags ();
      break;

      /* $! -- Pid of the last asynchronous command. */
    case '!':
      /* If no asynchronous pids have been created, expand to nothing.
	 If `set -u' has been executed, and no async processes have
	 been created, this is an expansion error. */
      if (last_asynchronous_pid == NO_PID)
	{
	  if (expanded_something)
	    *expanded_something = 0;
	  temp = (char *)NULL;
	  if (unbound_vars_is_error)
	    {
	      uerror[0] = '$';
	      uerror[1] = c;
	      uerror[2] = '\0';
	      last_command_exit_value = EXECUTION_FAILURE;
	      err_unboundvar (uerror);
	      return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	    }
	}
      else
	temp = itos (last_asynchronous_pid);
      break;

    /* The only difference between this and $@ is when the arg is quoted. */
    case '*':		/* `$*' */
      list = list_rest_of_args ();

#if 0
      /* According to austin-group posix proposal by Geoff Clare in
	 <20090505091501.GA10097@squonk.masqnet> of 5 May 2009:

 	"The shell shall write a message to standard error and
 	 immediately exit when it tries to expand an unset parameter
 	 other than the '@' and '*' special parameters."
      */

      if (list == 0 && unbound_vars_is_error && (pflags & PF_IGNUNBOUND) == 0)
	{
	  uerror[0] = '$';
	  uerror[1] = '*';
	  uerror[2] = '\0';
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (uerror);
	  return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	}
#endif

      /* If there are no command-line arguments, this should just
	 disappear if there are other characters in the expansion,
	 even if it's quoted. */
      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && list == 0)
	temp = (char *)NULL;
      else if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES|Q_PATQUOTE))
	{
	  /* If we have "$*" we want to make a string of the positional
	     parameters, separated by the first character of $IFS, and
	     quote the whole string, including the separators.  If IFS
	     is unset, the parameters are separated by ' '; if $IFS is
	     null, the parameters are concatenated. */
	  temp = (quoted & (Q_DOUBLE_QUOTES|Q_PATQUOTE)) ? string_list_dollar_star (list) : string_list (list);
	  temp1 = quote_string (temp);
	  if (*temp == 0)
	    tflag |= W_HASQUOTEDNULL;
	  free (temp);
	  temp = temp1;
	}
      else
	{
	  /* We check whether or not we're eventually going to split $* here,
	     for example when IFS is empty and we are processing the rhs of
	     an assignment statement.  In that case, we don't separate the
	     arguments at all.  Otherwise, if the $* is not quoted it is
	     identical to $@ */
#if 1
#  if defined (HANDLE_MULTIBYTE)
	  if (expand_no_split_dollar_star && ifs_firstc[0] == 0)
#  else
	  if (expand_no_split_dollar_star && ifs_firstc == 0)
#  endif
	    temp = string_list_dollar_star (list);
	  else
	    temp = string_list_dollar_at (list, quoted);
#else
	  temp = string_list_dollar_at (list, quoted);
#endif
	  if (expand_no_split_dollar_star == 0 && contains_dollar_at)
	    *contains_dollar_at = 1;
	}

      dispose_words (list);
      break;

    /* When we have "$@" what we want is "$1" "$2" "$3" ... This
       means that we have to turn quoting off after we split into
       the individually quoted arguments so that the final split
       on the first character of $IFS is still done.  */
    case '@':		/* `$@' */
      list = list_rest_of_args ();

#if 0
      /* According to austin-group posix proposal by Geoff Clare in
	 <20090505091501.GA10097@squonk.masqnet> of 5 May 2009:

 	"The shell shall write a message to standard error and
 	 immediately exit when it tries to expand an unset parameter
 	 other than the '@' and '*' special parameters."
      */

      if (list == 0 && unbound_vars_is_error && (pflags & PF_IGNUNBOUND) == 0)
	{
	  uerror[0] = '$';
	  uerror[1] = '@';
	  uerror[2] = '\0';
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (uerror);
	  return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	}
#endif

      /* We want to flag the fact that we saw this.  We can't turn
	 off quoting entirely, because other characters in the
	 string might need it (consider "\"$@\""), but we need some
	 way to signal that the final split on the first character
	 of $IFS should be done, even though QUOTED is 1. */
      /* XXX - should this test include Q_PATQUOTE? */
      if (quoted_dollar_at_p && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	*quoted_dollar_at_p = 1;
      if (contains_dollar_at)
	*contains_dollar_at = 1;

#if 0
      if (pflags & PF_NOSPLIT2)
	temp = string_list_internal (quoted ? quote_list (list) : list, " ");
      else
#endif
      /* We want to separate the positional parameters with the first
	 character of $IFS in case $IFS is something other than a space.
	 We also want to make sure that splitting is done no matter what --
	 according to POSIX.2, this expands to a list of the positional
	 parameters no matter what IFS is set to. */
      temp = string_list_dollar_at (list, quoted);

      dispose_words (list);
      break;

    case LBRACE:
      tdesc = parameter_brace_expand (string, &zindex, quoted, pflags,
				      quoted_dollar_at_p,
				      contains_dollar_at);

      if (tdesc == &expand_wdesc_error || tdesc == &expand_wdesc_fatal)
	return (tdesc);
      temp = tdesc ? tdesc->word : (char *)0;

      /* XXX */
      /* Quoted nulls should be removed if there is anything else
	 in the string. */
      /* Note that we saw the quoted null so we can add one back at
	 the end of this function if there are no other characters
	 in the string, discard TEMP, and go on.  The exception to
	 this is when we have "${@}" and $1 is '', since $@ needs
	 special handling. */
      if (tdesc && tdesc->word && (tdesc->flags & W_HASQUOTEDNULL) && QUOTED_NULL (temp))
	{
	  if (had_quoted_null_p)
	    *had_quoted_null_p = 1;
	  if (*quoted_dollar_at_p == 0)
	    {
	      free (temp);
	      tdesc->word = temp = (char *)NULL;
	    }
	    
	}

      ret = tdesc;
      goto return0;

    /* Do command or arithmetic substitution. */
    case LPAREN:
      /* We have to extract the contents of this paren substitution. */
      t_index = zindex + 1;
      temp = extract_command_subst (string, &t_index, 0);
      zindex = t_index;

      /* For Posix.2-style `$(( ))' arithmetic substitution,
	 extract the expression and pass it to the evaluator. */
      if (temp && *temp == LPAREN)
	{
	  char *temp2;
	  temp1 = temp + 1;
	  temp2 = savestring (temp1);
	  t_index = strlen (temp2) - 1;

	  if (temp2[t_index] != RPAREN)
	    {
	      free (temp2);
	      goto comsub;
	    }

	  /* Cut off ending `)' */
	  temp2[t_index] = '\0';

	  if (chk_arithsub (temp2, t_index) == 0)
	    {
	      free (temp2);
#if 0
	      internal_warning (_("future versions of the shell will force evaluation as an arithmetic substitution"));
#endif
	      goto comsub;
	    }

	  /* Expand variables found inside the expression. */
	  temp1 = expand_arith_string (temp2, Q_DOUBLE_QUOTES);
	  free (temp2);

arithsub:
	  /* No error messages. */
	  this_command_name = (char *)NULL;
	  number = evalexp (temp1, &expok);
	  free (temp);
	  free (temp1);
	  if (expok == 0)
	    {
	      if (interactive_shell == 0 && posixly_correct)
		{
		  last_command_exit_value = EXECUTION_FAILURE;
		  return (&expand_wdesc_fatal);
		}
	      else
		return (&expand_wdesc_error);
	    }
	  temp = itos (number);
	  break;
	}

comsub:
      if (pflags & PF_NOCOMSUB)
	/* we need zindex+1 because string[zindex] == RPAREN */
	temp1 = substring (string, *sindex, zindex+1);
      else
	{
	  tdesc = command_substitute (temp, quoted);
	  temp1 = tdesc ? tdesc->word : (char *)NULL;
	  if (tdesc)
	    dispose_word_desc (tdesc);
	}
      FREE (temp);
      temp = temp1;
      break;

    /* Do POSIX.2d9-style arithmetic substitution.  This will probably go
       away in a future bash release. */
    case '[':
      /* Extract the contents of this arithmetic substitution. */
      t_index = zindex + 1;
      temp = extract_arithmetic_subst (string, &t_index);
      zindex = t_index;
      if (temp == 0)
	{
	  temp = savestring (string);
	  if (expanded_something)
	    *expanded_something = 0;
	  goto return0;
	}	  

       /* Do initial variable expansion. */
      temp1 = expand_arith_string (temp, Q_DOUBLE_QUOTES);

      goto arithsub;

    default:
      /* Find the variable in VARIABLE_LIST. */
      temp = (char *)NULL;

      for (t_index = zindex; (c = string[zindex]) && legal_variable_char (c); zindex++)
	;
      temp1 = (zindex > t_index) ? substring (string, t_index, zindex) : (char *)NULL;

      /* If this isn't a variable name, then just output the `$'. */
      if (temp1 == 0 || *temp1 == '\0')
	{
	  FREE (temp1);
	  temp = (char *)xmalloc (2);
	  temp[0] = '$';
	  temp[1] = '\0';
	  if (expanded_something)
	    *expanded_something = 0;
	  goto return0;
	}

      /* If the variable exists, return its value cell. */
      var = find_variable (temp1);

      if (var && invisible_p (var) == 0 && var_isset (var))
	{
#if defined (ARRAY_VARS)
	  if (assoc_p (var) || array_p (var))
	    {
	      temp = array_p (var) ? array_reference (array_cell (var), 0)
				   : assoc_reference (assoc_cell (var), "0");
	      if (temp)
		temp = (*temp && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
			  ? quote_string (temp)
			  : quote_escapes (temp);
	      else if (unbound_vars_is_error)
		goto unbound_variable;
	    }
	  else
#endif
	    {
	      temp = value_cell (var);

	      temp = (*temp && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
			? quote_string (temp)
			: quote_escapes (temp);
	    }

	  free (temp1);

	  goto return0;
	}

      temp = (char *)NULL;

unbound_variable:
      if (unbound_vars_is_error)
	{
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (temp1);
	}
      else
	{
	  free (temp1);
	  goto return0;
	}

      free (temp1);
      last_command_exit_value = EXECUTION_FAILURE;
      return ((unbound_vars_is_error && interactive_shell == 0)
		? &expand_wdesc_fatal
		: &expand_wdesc_error);
    }

  if (string[zindex])
    zindex++;

return0:
  *sindex = zindex;

  if (ret == 0)
    {
      ret = alloc_word_desc ();
      ret->flags = tflag;	/* XXX */
      ret->word = temp;
    }
  return ret;
}

/* Make a word list which is the result of parameter and variable
   expansion, command substitution, arithmetic substitution, and
   quote removal of WORD.  Return a pointer to a WORD_LIST which is
   the result of the expansion.  If WORD contains a null word, the
   word list returned is also null.

   QUOTED contains flag values defined in shell.h.

   ISEXP is used to tell expand_word_internal that the word should be
   treated as the result of an expansion.  This has implications for
   how IFS characters in the word are treated.

   CONTAINS_DOLLAR_AT and EXPANDED_SOMETHING are return values; when non-null
   they point to an integer value which receives information about expansion.
   CONTAINS_DOLLAR_AT gets non-zero if WORD contained "$@", else zero.
   EXPANDED_SOMETHING get non-zero if WORD contained any parameter expansions,
   else zero.

   This only does word splitting in the case of $@ expansion.  In that
   case, we split on ' '. */

/* Values for the local variable quoted_state. */
#define UNQUOTED	 0
#define PARTIALLY_QUOTED 1
#define WHOLLY_QUOTED    2

static WORD_LIST *
expand_word_internal (word, quoted, isexp, contains_dollar_at, expanded_something)
     WORD_DESC *word;
     int quoted, isexp;
     int *contains_dollar_at;
     int *expanded_something;
{
  WORD_LIST *list;
  WORD_DESC *tword;

  /* The intermediate string that we build while expanding. */
  char *istring;

  /* The current size of the above object. */
  int istring_size;

  /* Index into ISTRING. */
  int istring_index;

  /* Temporary string storage. */
  char *temp, *temp1;

  /* The text of WORD. */
  register char *string;

  /* The size of STRING. */
  size_t string_size;

  /* The index into STRING. */
  int sindex;

  /* This gets 1 if we see a $@ while quoted. */
  int quoted_dollar_at;

  /* One of UNQUOTED, PARTIALLY_QUOTED, or WHOLLY_QUOTED, depending on
     whether WORD contains no quoting characters, a partially quoted
     string (e.g., "xx"ab), or is fully quoted (e.g., "xxab"). */
  int quoted_state;

  /* State flags */
  int had_quoted_null;
  int has_dollar_at;
  int tflag;
  int pflags;			/* flags passed to param_expand */

  int assignoff;		/* If assignment, offset of `=' */

  register unsigned char c;	/* Current character. */
  int t_index;			/* For calls to string_extract_xxx. */

  char twochars[2];

  DECLARE_MBSTATE;

  istring = (char *)xmalloc (istring_size = DEFAULT_INITIAL_ARRAY_SIZE);
  istring[istring_index = 0] = '\0';
  quoted_dollar_at = had_quoted_null = has_dollar_at = 0;
  quoted_state = UNQUOTED;

  string = word->word;
  if (string == 0)
    goto finished_with_string;
  /* Don't need the string length for the SADD... and COPY_ macros unless
     multibyte characters are possible. */
  string_size = (MB_CUR_MAX > 1) ? strlen (string) : 1;

  if (contains_dollar_at)
    *contains_dollar_at = 0;

  assignoff = -1;

  /* Begin the expansion. */

  for (sindex = 0; ;)
    {
      c = string[sindex];

      /* Case on toplevel character. */
      switch (c)
	{
	case '\0':
	  goto finished_with_string;

	case CTLESC:
	  sindex++;
#if HANDLE_MULTIBYTE
	  if (MB_CUR_MAX > 1 && string[sindex])
	    {
	      SADD_MBQCHAR_BODY(temp, string, sindex, string_size);
	    }
	  else
#endif
	    {
	      temp = (char *)xmalloc (3);
	      temp[0] = CTLESC;
	      temp[1] = c = string[sindex];
	      temp[2] = '\0';
	    }

dollar_add_string:
	  if (string[sindex])
	    sindex++;

add_string:
	  if (temp)
	    {
	      istring = sub_append_string (temp, istring, &istring_index, &istring_size);
	      temp = (char *)0;
	    }

	  break;

#if defined (PROCESS_SUBSTITUTION)
	  /* Process substitution. */
	case '<':
	case '>':
	  {
	    if (string[++sindex] != LPAREN || (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) || (word->flags & (W_DQUOTE|W_NOPROCSUB)) || posixly_correct)
	      {
		sindex--;	/* add_character: label increments sindex */
		goto add_character;
	      }
	    else
	      t_index = sindex + 1; /* skip past both '<' and LPAREN */

	    temp1 = extract_process_subst (string, (c == '<') ? "<(" : ">(", &t_index); /*))*/
	    sindex = t_index;

	    /* If the process substitution specification is `<()', we want to
	       open the pipe for writing in the child and produce output; if
	       it is `>()', we want to open the pipe for reading in the child
	       and consume input. */
	    temp = temp1 ? process_substitute (temp1, (c == '>')) : (char *)0;

	    FREE (temp1);

	    goto dollar_add_string;
	  }
#endif /* PROCESS_SUBSTITUTION */

	case '=':
	  /* Posix.2 section 3.6.1 says that tildes following `=' in words
	     which are not assignment statements are not expanded.  If the
	     shell isn't in posix mode, though, we perform tilde expansion
	     on `likely candidate' unquoted assignment statements (flags
	     include W_ASSIGNMENT but not W_QUOTED).  A likely candidate
	     contains an unquoted :~ or =~.  Something to think about: we
	     now have a flag that says  to perform tilde expansion on arguments
	     to `assignment builtins' like declare and export that look like
	     assignment statements.  We now do tilde expansion on such words
	     even in POSIX mode. */	
	  if (word->flags & (W_ASSIGNRHS|W_NOTILDE))
	    {
	      if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c))
		goto add_ifs_character;
	      else
		goto add_character;
	    }
	  /* If we're not in posix mode or forcing assignment-statement tilde
	     expansion, note where the `=' appears in the word and prepare to
	     do tilde expansion following the first `='. */
	  if ((word->flags & W_ASSIGNMENT) &&
	      (posixly_correct == 0 || (word->flags & W_TILDEEXP)) &&
	      assignoff == -1 && sindex > 0)
	    assignoff = sindex;
	  if (sindex == assignoff && string[sindex+1] == '~')	/* XXX */
	    word->flags |= W_ITILDE;
#if 0
	  else if ((word->flags & W_ASSIGNMENT) &&
		   (posixly_correct == 0 || (word->flags & W_TILDEEXP)) &&
		   string[sindex+1] == '~')
	    word->flags |= W_ITILDE;
#endif
	  if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c))
	    goto add_ifs_character;
	  else
	    goto add_character;

	case ':':
	  if (word->flags & W_NOTILDE)
	    {
	      if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c))
		goto add_ifs_character;
	      else
		goto add_character;
	    }

	  if ((word->flags & (W_ASSIGNMENT|W_ASSIGNRHS|W_TILDEEXP)) &&
	      string[sindex+1] == '~')
	    word->flags |= W_ITILDE;

	  if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c))
	    goto add_ifs_character;
	  else
	    goto add_character;

	case '~':
	  /* If the word isn't supposed to be tilde expanded, or we're not
	     at the start of a word or after an unquoted : or = in an
	     assignment statement, we don't do tilde expansion. */
	  if ((word->flags & (W_NOTILDE|W_DQUOTE)) ||
	      (sindex > 0 && ((word->flags & W_ITILDE) == 0)) ||
	      (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
	    {
	      word->flags &= ~W_ITILDE;
	      if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c) && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) == 0)
		goto add_ifs_character;
	      else
		goto add_character;
	    }

	  if (word->flags & W_ASSIGNRHS)
	    tflag = 2;
	  else if (word->flags & (W_ASSIGNMENT|W_TILDEEXP))
	    tflag = 1;
	  else
	    tflag = 0;

	  temp = bash_tilde_find_word (string + sindex, tflag, &t_index);
	    
	  word->flags &= ~W_ITILDE;

	  if (temp && *temp && t_index > 0)
	    {
	      temp1 = bash_tilde_expand (temp, tflag);
	      if  (temp1 && *temp1 == '~' && STREQ (temp, temp1))
		{
		  FREE (temp);
		  FREE (temp1);
		  goto add_character;		/* tilde expansion failed */
		}
	      free (temp);
	      temp = temp1;
	      sindex += t_index;
	      goto add_quoted_string;		/* XXX was add_string */
	    }
	  else
	    {
	      FREE (temp);
	      goto add_character;
	    }
	
	case '$':
	  if (expanded_something)
	    *expanded_something = 1;

	  has_dollar_at = 0;
	  pflags = (word->flags & W_NOCOMSUB) ? PF_NOCOMSUB : 0;
	  if (word->flags & W_NOSPLIT2)
	    pflags |= PF_NOSPLIT2;
	  tword = param_expand (string, &sindex, quoted, expanded_something,
			       &has_dollar_at, &quoted_dollar_at,
			       &had_quoted_null, pflags);

	  if (tword == &expand_wdesc_error || tword == &expand_wdesc_fatal)
	    {
	      free (string);
	      free (istring);
	      return ((tword == &expand_wdesc_error) ? &expand_word_error
						     : &expand_word_fatal);
	    }
	  if (contains_dollar_at && has_dollar_at)
	    *contains_dollar_at = 1;

	  if (tword && (tword->flags & W_HASQUOTEDNULL))
	    had_quoted_null = 1;

	  temp = tword->word;
	  dispose_word_desc (tword);

	  goto add_string;
	  break;

	case '`':		/* Backquoted command substitution. */
	  {
	    t_index = sindex++;

	    temp = string_extract (string, &sindex, "`", SX_REQMATCH);
	    /* The test of sindex against t_index is to allow bare instances of
	       ` to pass through, for backwards compatibility. */
	    if (temp == &extract_string_error || temp == &extract_string_fatal)
	      {
		if (sindex - 1 == t_index)
		  {
		    sindex = t_index;
		    goto add_character;
		  }
		report_error (_("bad substitution: no closing \"`\" in %s") , string+t_index);
		free (string);
		free (istring);
		return ((temp == &extract_string_error) ? &expand_word_error
							: &expand_word_fatal);
	      }
		
	    if (expanded_something)
	      *expanded_something = 1;

	    if (word->flags & W_NOCOMSUB)
	      /* sindex + 1 because string[sindex] == '`' */
	      temp1 = substring (string, t_index, sindex + 1);
	    else
	      {
		de_backslash (temp);
		tword = command_substitute (temp, quoted);
		temp1 = tword ? tword->word : (char *)NULL;
		if (tword)
		  dispose_word_desc (tword);
	      }
	    FREE (temp);
	    temp = temp1;
	    goto dollar_add_string;
	  }

	case '\\':
	  if (string[sindex + 1] == '\n')
	    {
	      sindex += 2;
	      continue;
	    }

	  c = string[++sindex];

	  if (quoted & Q_HERE_DOCUMENT)
	    tflag = CBSHDOC;
	  else if (quoted & Q_DOUBLE_QUOTES)
	    tflag = CBSDQUOTE;
	  else
	    tflag = 0;

	  if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && ((sh_syntaxtab[c] & tflag) == 0))
	    {
	      SCOPY_CHAR_I (twochars, '\\', c, string, sindex, string_size);
	    }
	  else if (c == 0)
	    {
	      c = CTLNUL;
	      sindex--;		/* add_character: label increments sindex */
	      goto add_character;
	    }
	  else
	    {
	      SCOPY_CHAR_I (twochars, CTLESC, c, string, sindex, string_size);
	    }

	  sindex++;
add_twochars:
	  /* BEFORE jumping here, we need to increment sindex if appropriate */
	  RESIZE_MALLOCED_BUFFER (istring, istring_index, 2, istring_size,
				  DEFAULT_ARRAY_SIZE);
	  istring[istring_index++] = twochars[0];
	  istring[istring_index++] = twochars[1];
	  istring[istring_index] = '\0';

	  break;

	case '"':
#if 0
	  if ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) || (word->flags & W_DQUOTE))
#else
	  if ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
#endif
	    goto add_character;

	  t_index = ++sindex;
	  temp = string_extract_double_quoted (string, &sindex, 0);

	  /* If the quotes surrounded the entire string, then the
	     whole word was quoted. */
	  quoted_state = (t_index == 1 && string[sindex] == '\0')
			    ? WHOLLY_QUOTED
			    : PARTIALLY_QUOTED;

	  if (temp && *temp)
	    {
	      tword = alloc_word_desc ();
	      tword->word = temp;

	      temp = (char *)NULL;

	      has_dollar_at = 0;
	      /* Need to get W_HASQUOTEDNULL flag through this function. */
	      list = expand_word_internal (tword, Q_DOUBLE_QUOTES, 0, &has_dollar_at, (int *)NULL);

	      if (list == &expand_word_error || list == &expand_word_fatal)
		{
		  free (istring);
		  free (string);
		  /* expand_word_internal has already freed temp_word->word
		     for us because of the way it prints error messages. */
		  tword->word = (char *)NULL;
		  dispose_word (tword);
		  return list;
		}

	      dispose_word (tword);

	      /* "$@" (a double-quoted dollar-at) expands into nothing,
		 not even a NULL word, when there are no positional
		 parameters. */
	      if (list == 0 && has_dollar_at)
		{
		  quoted_dollar_at++;
		  break;
		}

	      /* If we get "$@", we know we have expanded something, so we
		 need to remember it for the final split on $IFS.  This is
		 a special case; it's the only case where a quoted string
		 can expand into more than one word.  It's going to come back
		 from the above call to expand_word_internal as a list with
		 a single word, in which all characters are quoted and
		 separated by blanks.  What we want to do is to turn it back
		 into a list for the next piece of code. */
	      if (list)
		dequote_list (list);

	      if (list && list->word && (list->word->flags & W_HASQUOTEDNULL))
		had_quoted_null = 1;

	      if (has_dollar_at)
		{
		  quoted_dollar_at++;
		  if (contains_dollar_at)
		    *contains_dollar_at = 1;
		  if (expanded_something)
		    *expanded_something = 1;
		}
	    }
	  else
	    {
	      /* What we have is "".  This is a minor optimization. */
	      FREE (temp);
	      list = (WORD_LIST *)NULL;
	    }

	  /* The code above *might* return a list (consider the case of "$@",
	     where it returns "$1", "$2", etc.).  We can't throw away the
	     rest of the list, and we have to make sure each word gets added
	     as quoted.  We test on tresult->next:  if it is non-NULL, we
	     quote the whole list, save it to a string with string_list, and
	     add that string. We don't need to quote the results of this
	     (and it would be wrong, since that would quote the separators
	     as well), so we go directly to add_string. */
	  if (list)
	    {
	      if (list->next)
		{
#if 0
		  if (quoted_dollar_at && word->flags & W_NOSPLIT2)
		    temp = string_list_internal (quote_list (list), " ");
		  else
#endif
		  /* Testing quoted_dollar_at makes sure that "$@" is
		     split correctly when $IFS does not contain a space. */
		  temp = quoted_dollar_at
				? string_list_dollar_at (list, Q_DOUBLE_QUOTES)
				: string_list (quote_list (list));
		  dispose_words (list);
		  goto add_string;
		}
	      else
		{
		  temp = savestring (list->word->word);
		  tflag = list->word->flags;
		  dispose_words (list);

		  /* If the string is not a quoted null string, we want
		     to remove any embedded unquoted CTLNUL characters.
		     We do not want to turn quoted null strings back into
		     the empty string, though.  We do this because we
		     want to remove any quoted nulls from expansions that
		     contain other characters.  For example, if we have
		     x"$*"y or "x$*y" and there are no positional parameters,
		     the $* should expand into nothing. */
		  /* We use the W_HASQUOTEDNULL flag to differentiate the
		     cases:  a quoted null character as above and when
		     CTLNUL is contained in the (non-null) expansion
		     of some variable.  We use the had_quoted_null flag to
		     pass the value through this function to its caller. */
		  if ((tflag & W_HASQUOTEDNULL) && QUOTED_NULL (temp) == 0)
		    remove_quoted_nulls (temp);	/* XXX */
		}
	    }
	  else
	    temp = (char *)NULL;

	  /* We do not want to add quoted nulls to strings that are only
	     partially quoted; we can throw them away. */
	  if (temp == 0 && quoted_state == PARTIALLY_QUOTED)
	    continue;

	add_quoted_string:

	  if (temp)
	    {
	      temp1 = temp;
	      temp = quote_string (temp);
	      free (temp1);
	      goto add_string;
	    }
	  else
	    {
	      /* Add NULL arg. */
	      c = CTLNUL;
	      sindex--;		/* add_character: label increments sindex */
	      goto add_character;
	    }

	  /* break; */

	case '\'':
#if 0
	  if ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) || (word->flags & W_DQUOTE))
#else
	  if ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
#endif
	    goto add_character;

	  t_index = ++sindex;
	  temp = string_extract_single_quoted (string, &sindex);

	  /* If the entire STRING was surrounded by single quotes,
	     then the string is wholly quoted. */
	  quoted_state = (t_index == 1 && string[sindex] == '\0')
			    ? WHOLLY_QUOTED
			    : PARTIALLY_QUOTED;

	  /* If all we had was '', it is a null expansion. */
	  if (*temp == '\0')
	    {
	      free (temp);
	      temp = (char *)NULL;
	    }
	  else
	    remove_quoted_escapes (temp);	/* ??? */

	  /* We do not want to add quoted nulls to strings that are only
	     partially quoted; such nulls are discarded. */
	  if (temp == 0 && (quoted_state == PARTIALLY_QUOTED))
	    continue;

	  /* If we have a quoted null expansion, add a quoted NULL to istring. */
	  if (temp == 0)
	    {
	      c = CTLNUL;
	      sindex--;		/* add_character: label increments sindex */
	      goto add_character;
	    }
	  else
	    goto add_quoted_string;

	  /* break; */

	default:
	  /* This is the fix for " $@ " */
	add_ifs_character:
	  if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) || (isexp == 0 && isifs (c)))
	    {
	      if (string[sindex])	/* from old goto dollar_add_string */
		sindex++;
	      if (c == 0)
		{
		  c = CTLNUL;
		  goto add_character;
		}
	      else
		{
#if HANDLE_MULTIBYTE
		  if (MB_CUR_MAX > 1)
		    sindex--;

		  if (MB_CUR_MAX > 1)
		    {
		      SADD_MBQCHAR_BODY(temp, string, sindex, string_size);
		    }
		  else
#endif
		    {
		      twochars[0] = CTLESC;
		      twochars[1] = c;
		      goto add_twochars;
		    }
		}
	    }

	  SADD_MBCHAR (temp, string, sindex, string_size);

	add_character:
	  RESIZE_MALLOCED_BUFFER (istring, istring_index, 1, istring_size,
				  DEFAULT_ARRAY_SIZE);
	  istring[istring_index++] = c;
	  istring[istring_index] = '\0';

	  /* Next character. */
	  sindex++;
	}
    }

finished_with_string:
  /* OK, we're ready to return.  If we have a quoted string, and
     quoted_dollar_at is not set, we do no splitting at all; otherwise
     we split on ' '.  The routines that call this will handle what to
     do if nothing has been expanded. */

  /* Partially and wholly quoted strings which expand to the empty
     string are retained as an empty arguments.  Unquoted strings
     which expand to the empty string are discarded.  The single
     exception is the case of expanding "$@" when there are no
     positional parameters.  In that case, we discard the expansion. */

  /* Because of how the code that handles "" and '' in partially
     quoted strings works, we need to make ISTRING into a QUOTED_NULL
     if we saw quoting characters, but the expansion was empty.
     "" and '' are tossed away before we get to this point when
     processing partially quoted strings.  This makes "" and $xxx""
     equivalent when xxx is unset.  We also look to see whether we
     saw a quoted null from a ${} expansion and add one back if we
     need to. */

  /* If we expand to nothing and there were no single or double quotes
     in the word, we throw it away.  Otherwise, we return a NULL word.
     The single exception is for $@ surrounded by double quotes when
     there are no positional parameters.  In that case, we also throw
     the word away. */

  if (*istring == '\0')
    {
      if (quoted_dollar_at == 0 && (had_quoted_null || quoted_state == PARTIALLY_QUOTED))
	{
	  istring[0] = CTLNUL;
	  istring[1] = '\0';
	  tword = make_bare_word (istring);
	  tword->flags |= W_HASQUOTEDNULL;		/* XXX */
	  list = make_word_list (tword, (WORD_LIST *)NULL);
	  if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
	    tword->flags |= W_QUOTED;
	}
      /* According to sh, ksh, and Posix.2, if a word expands into nothing
	 and a double-quoted "$@" appears anywhere in it, then the entire
	 word is removed. */
      else  if (quoted_state == UNQUOTED || quoted_dollar_at)
	list = (WORD_LIST *)NULL;
#if 0
      else
	{
	  tword = make_bare_word (istring);
	  if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
	    tword->flags |= W_QUOTED;
	  list = make_word_list (tword, (WORD_LIST *)NULL);
	}
#else
      else
	list = (WORD_LIST *)NULL;
#endif
    }
  else if (word->flags & W_NOSPLIT)
    {
      tword = make_bare_word (istring);
      if (word->flags & W_ASSIGNMENT)
	tword->flags |= W_ASSIGNMENT;	/* XXX */
      if (word->flags & W_COMPASSIGN)
	tword->flags |= W_COMPASSIGN;	/* XXX */
      if (word->flags & W_NOGLOB)
	tword->flags |= W_NOGLOB;	/* XXX */
      if (word->flags & W_NOEXPAND)
	tword->flags |= W_NOEXPAND;	/* XXX */
      if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
	tword->flags |= W_QUOTED;
      if (had_quoted_null)
	tword->flags |= W_HASQUOTEDNULL;
      list = make_word_list (tword, (WORD_LIST *)NULL);
    }
  else
    {
      char *ifs_chars;

      ifs_chars = (quoted_dollar_at || has_dollar_at) ? ifs_value : (char *)NULL;

      /* If we have $@, we need to split the results no matter what.  If
	 IFS is unset or NULL, string_list_dollar_at has separated the
	 positional parameters with a space, so we split on space (we have
	 set ifs_chars to " \t\n" above if ifs is unset).  If IFS is set,
	 string_list_dollar_at has separated the positional parameters
	 with the first character of $IFS, so we split on $IFS. */
      if (has_dollar_at && ifs_chars)
	list = list_string (istring, *ifs_chars ? ifs_chars : " ", 1);
      else
	{
	  tword = make_bare_word (istring);
	  if ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) || (quoted_state == WHOLLY_QUOTED))
	    tword->flags |= W_QUOTED;
	  if (word->flags & W_ASSIGNMENT)
	    tword->flags |= W_ASSIGNMENT;
	  if (word->flags & W_COMPASSIGN)
	    tword->flags |= W_COMPASSIGN;
	  if (word->flags & W_NOGLOB)
	    tword->flags |= W_NOGLOB;
	  if (word->flags & W_NOEXPAND)
	    tword->flags |= W_NOEXPAND;
	  if (had_quoted_null)
	    tword->flags |= W_HASQUOTEDNULL;	/* XXX */
	  list = make_word_list (tword, (WORD_LIST *)NULL);
	}
    }

  free (istring);
  return (list);
}

/* **************************************************************** */
/*								    */
/*		   Functions for Quote Removal			    */
/*								    */
/* **************************************************************** */

/* Perform quote removal on STRING.  If QUOTED > 0, assume we are obeying the
   backslash quoting rules for within double quotes or a here document. */
char *
string_quote_removal (string, quoted)
     char *string;
     int quoted;
{
  size_t slen;
  char *r, *result_string, *temp, *send;
  int sindex, tindex, dquote;
  unsigned char c;
  DECLARE_MBSTATE;

  /* The result can be no longer than the original string. */
  slen = strlen (string);
  send = string + slen;

  r = result_string = (char *)xmalloc (slen + 1);

  for (dquote = sindex = 0; c = string[sindex];)
    {
      switch (c)
	{
	case '\\':
	  c = string[++sindex];
	  if (c == 0)
	    {
	      *r++ = '\\';
	      break;
	    }
	  if (((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) || dquote) && (sh_syntaxtab[c] & CBSDQUOTE) == 0)
	    *r++ = '\\';
	  /* FALLTHROUGH */

	default:
	  SCOPY_CHAR_M (r, string, send, sindex);
	  break;

	case '\'':
	  if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) || dquote)
	    {
	      *r++ = c;
	      sindex++;
	      break;
	    }
	  tindex = sindex + 1;
	  temp = string_extract_single_quoted (string, &tindex);
	  if (temp)
	    {
	      strcpy (r, temp);
	      r += strlen (r);
	      free (temp);
	    }
	  sindex = tindex;
	  break;

	case '"':
	  dquote = 1 - dquote;
	  sindex++;
	  break;
	}
    }
    *r = '\0';
    return (result_string);
}

#if 0
/* UNUSED */
/* Perform quote removal on word WORD.  This allocates and returns a new
   WORD_DESC *. */
WORD_DESC *
word_quote_removal (word, quoted)
     WORD_DESC *word;
     int quoted;
{
  WORD_DESC *w;
  char *t;

  t = string_quote_removal (word->word, quoted);
  w = alloc_word_desc ();
  w->word = t ? t : savestring ("");
  return (w);
}

/* Perform quote removal on all words in LIST.  If QUOTED is non-zero,
   the members of the list are treated as if they are surrounded by
   double quotes.  Return a new list, or NULL if LIST is NULL. */
WORD_LIST *
word_list_quote_removal (list, quoted)
     WORD_LIST *list;
     int quoted;
{
  WORD_LIST *result, *t, *tresult, *e;

  for (t = list, result = (WORD_LIST *)NULL; t; t = t->next)
    {
      tresult = make_word_list (word_quote_removal (t->word, quoted), (WORD_LIST *)NULL);
#if 0
      result = (WORD_LIST *) list_append (result, tresult);
#else
      if (result == 0)
	result = e = tresult;
      else
	{
	  e->next = tresult;
	  while (e->next)
	    e = e->next;
	}
#endif
    }
  return (result);
}
#endif

/*******************************************
 *					   *
 *    Functions to perform word splitting  *
 *					   *
 *******************************************/

void
setifs (v)
     SHELL_VAR *v;
{
  char *t;
  unsigned char uc;

  ifs_var = v;
  ifs_value = (v && value_cell (v)) ? value_cell (v) : " \t\n";

  /* Should really merge ifs_cmap with sh_syntaxtab.  XXX - doesn't yet
     handle multibyte chars in IFS */
  memset (ifs_cmap, '\0', sizeof (ifs_cmap));
  for (t = ifs_value ; t && *t; t++)
    {
      uc = *t;
      ifs_cmap[uc] = 1;
    }

#if defined (HANDLE_MULTIBYTE)
  if (ifs_value == 0)
    {
      ifs_firstc[0] = '\0';
      ifs_firstc_len = 1;
    }
  else
    {
      size_t ifs_len;
      ifs_len = strnlen (ifs_value, MB_CUR_MAX);
      ifs_firstc_len = MBLEN (ifs_value, ifs_len);
      if (ifs_firstc_len == 1 || ifs_firstc_len == 0 || MB_INVALIDCH (ifs_firstc_len))
	{
	  ifs_firstc[0] = ifs_value[0];
	  ifs_firstc[1] = '\0';
	  ifs_firstc_len = 1;
	}
      else
	memcpy (ifs_firstc, ifs_value, ifs_firstc_len);
    }
#else
  ifs_firstc = ifs_value ? *ifs_value : 0;
#endif
}

char *
getifs ()
{
  return ifs_value;
}

/* This splits a single word into a WORD LIST on $IFS, but only if the word
   is not quoted.  list_string () performs quote removal for us, even if we
   don't do any splitting. */
WORD_LIST *
word_split (w, ifs_chars)
     WORD_DESC *w;
     char *ifs_chars;
{
  WORD_LIST *result;

  if (w)
    {
      char *xifs;

      xifs = ((w->flags & W_QUOTED) || ifs_chars == 0) ? "" : ifs_chars;
      result = list_string (w->word, xifs, w->flags & W_QUOTED);
    }
  else
    result = (WORD_LIST *)NULL;

  return (result);
}

/* Perform word splitting on LIST and return the RESULT.  It is possible
   to return (WORD_LIST *)NULL. */
static WORD_LIST *
word_list_split (list)
     WORD_LIST *list;
{
  WORD_LIST *result, *t, *tresult, *e;

  for (t = list, result = (WORD_LIST *)NULL; t; t = t->next)
    {
      tresult = word_split (t->word, ifs_value);
      if (result == 0)
        result = e = tresult;
      else
	{
	  e->next = tresult;
	  while (e->next)
	    e = e->next;
	}
    }
  return (result);
}

/**************************************************
 * 						  *
 *    Functions to expand an entire WORD_LIST	  *
 *						  *
 **************************************************/

/* Do any word-expansion-specific cleanup and jump to top_level */
static void
exp_jump_to_top_level (v)
     int v;
{
  set_pipestatus_from_exit (last_command_exit_value);

  /* Cleanup code goes here. */
  expand_no_split_dollar_star = 0;	/* XXX */
  expanding_redir = 0;
  assigning_in_environment = 0;

  if (parse_and_execute_level == 0)
    top_level_cleanup ();			/* from sig.c */

  jump_to_top_level (v);
}

/* Put NLIST (which is a WORD_LIST * of only one element) at the front of
   ELIST, and set ELIST to the new list. */
#define PREPEND_LIST(nlist, elist) \
	do { nlist->next = elist; elist = nlist; } while (0)

/* Separate out any initial variable assignments from TLIST.  If set -k has
   been executed, remove all assignment statements from TLIST.  Initial
   variable assignments and other environment assignments are placed
   on SUBST_ASSIGN_VARLIST. */
static WORD_LIST *
separate_out_assignments (tlist)
     WORD_LIST *tlist;
{
  register WORD_LIST *vp, *lp;

  if (tlist == 0)
    return ((WORD_LIST *)NULL);

  if (subst_assign_varlist)
    dispose_words (subst_assign_varlist);	/* Clean up after previous error */

  subst_assign_varlist = (WORD_LIST *)NULL;
  vp = lp = tlist;

  /* Separate out variable assignments at the start of the command.
     Loop invariant: vp->next == lp
     Loop postcondition:
	lp = list of words left after assignment statements skipped
	tlist = original list of words
  */
  while (lp && (lp->word->flags & W_ASSIGNMENT))
    {
      vp = lp;
      lp = lp->next;
    }

  /* If lp != tlist, we have some initial assignment statements.
     We make SUBST_ASSIGN_VARLIST point to the list of assignment
     words and TLIST point to the remaining words.  */
  if (lp != tlist)
    {
      subst_assign_varlist = tlist;
      /* ASSERT(vp->next == lp); */
      vp->next = (WORD_LIST *)NULL;	/* terminate variable list */
      tlist = lp;			/* remainder of word list */
    }

  /* vp == end of variable list */
  /* tlist == remainder of original word list without variable assignments */
  if (!tlist)
    /* All the words in tlist were assignment statements */
    return ((WORD_LIST *)NULL);

  /* ASSERT(tlist != NULL); */
  /* ASSERT((tlist->word->flags & W_ASSIGNMENT) == 0); */

  /* If the -k option is in effect, we need to go through the remaining
     words, separate out the assignment words, and place them on
     SUBST_ASSIGN_VARLIST. */
  if (place_keywords_in_env)
    {
      WORD_LIST *tp;	/* tp == running pointer into tlist */

      tp = tlist;
      lp = tlist->next;

      /* Loop Invariant: tp->next == lp */
      /* Loop postcondition: tlist == word list without assignment statements */
      while (lp)
	{
	  if (lp->word->flags & W_ASSIGNMENT)
	    {
	      /* Found an assignment statement, add this word to end of
		 subst_assign_varlist (vp). */
	      if (!subst_assign_varlist)
		subst_assign_varlist = vp = lp;
	      else
		{
		  vp->next = lp;
		  vp = lp;
		}

	      /* Remove the word pointed to by LP from TLIST. */
	      tp->next = lp->next;
	      /* ASSERT(vp == lp); */
	      lp->next = (WORD_LIST *)NULL;
	      lp = tp->next;
	    }
	  else
	    {
	      tp = lp;
	      lp = lp->next;
	    }
	}
    }
  return (tlist);
}

#define WEXP_VARASSIGN	0x001
#define WEXP_BRACEEXP	0x002
#define WEXP_TILDEEXP	0x004
#define WEXP_PARAMEXP	0x008
#define WEXP_PATHEXP	0x010

/* All of the expansions, including variable assignments at the start of
   the list. */
#define WEXP_ALL	(WEXP_VARASSIGN|WEXP_BRACEEXP|WEXP_TILDEEXP|WEXP_PARAMEXP|WEXP_PATHEXP)

/* All of the expansions except variable assignments at the start of
   the list. */
#define WEXP_NOVARS	(WEXP_BRACEEXP|WEXP_TILDEEXP|WEXP_PARAMEXP|WEXP_PATHEXP)

/* All of the `shell expansions': brace expansion, tilde expansion, parameter
   expansion, command substitution, arithmetic expansion, word splitting, and
   quote removal. */
#define WEXP_SHELLEXP	(WEXP_BRACEEXP|WEXP_TILDEEXP|WEXP_PARAMEXP)

/* Take the list of words in LIST and do the various substitutions.  Return
   a new list of words which is the expanded list, and without things like
   variable assignments. */

WORD_LIST *
expand_words (list)
     WORD_LIST *list;
{
  return (expand_word_list_internal (list, WEXP_ALL));
}

/* Same as expand_words (), but doesn't hack variable or environment
   variables. */
WORD_LIST *
expand_words_no_vars (list)
     WORD_LIST *list;
{
  return (expand_word_list_internal (list, WEXP_NOVARS));
}

WORD_LIST *
expand_words_shellexp (list)
     WORD_LIST *list;
{
  return (expand_word_list_internal (list, WEXP_SHELLEXP));
}

static WORD_LIST *
glob_expand_word_list (tlist, eflags)
     WORD_LIST *tlist;
     int eflags;
{
  char **glob_array, *temp_string;
  register int glob_index;
  WORD_LIST *glob_list, *output_list, *disposables, *next;
  WORD_DESC *tword;

  output_list = disposables = (WORD_LIST *)NULL;
  glob_array = (char **)NULL;
  while (tlist)
    {
      /* For each word, either globbing is attempted or the word is
	 added to orig_list.  If globbing succeeds, the results are
	 added to orig_list and the word (tlist) is added to the list
	 of disposable words.  If globbing fails and failed glob
	 expansions are left unchanged (the shell default), the
	 original word is added to orig_list.  If globbing fails and
	 failed glob expansions are removed, the original word is
	 added to the list of disposable words.  orig_list ends up
	 in reverse order and requires a call to REVERSE_LIST to
	 be set right.  After all words are examined, the disposable
	 words are freed. */
      next = tlist->next;

      /* If the word isn't an assignment and contains an unquoted
	 pattern matching character, then glob it. */
      if ((tlist->word->flags & W_NOGLOB) == 0 &&
	  unquoted_glob_pattern_p (tlist->word->word))
	{
	  glob_array = shell_glob_filename (tlist->word->word);

	  /* Handle error cases.
	     I don't think we should report errors like "No such file
	     or directory".  However, I would like to report errors
	     like "Read failed". */

	  if (glob_array == 0 || GLOB_FAILED (glob_array))
	    {
	      glob_array = (char **)xmalloc (sizeof (char *));
	      glob_array[0] = (char *)NULL;
	    }

	  /* Dequote the current word in case we have to use it. */
	  if (glob_array[0] == NULL)
	    {
	      temp_string = dequote_string (tlist->word->word);
	      free (tlist->word->word);
	      tlist->word->word = temp_string;
	    }

	  /* Make the array into a word list. */
	  glob_list = (WORD_LIST *)NULL;
	  for (glob_index = 0; glob_array[glob_index]; glob_index++)
	    {
	      tword = make_bare_word (glob_array[glob_index]);
	      tword->flags |= W_GLOBEXP;	/* XXX */
	      glob_list = make_word_list (tword, glob_list);
	    }

	  if (glob_list)
	    {
	      output_list = (WORD_LIST *)list_append (glob_list, output_list);
	      PREPEND_LIST (tlist, disposables);
	    }
	  else if (fail_glob_expansion != 0)
	    {
	      report_error (_("no match: %s"), tlist->word->word);
	      exp_jump_to_top_level (DISCARD);
	    }
	  else if (allow_null_glob_expansion == 0)
	    {
	      /* Failed glob expressions are left unchanged. */
	      PREPEND_LIST (tlist, output_list);
	    }
	  else
	    {
	      /* Failed glob expressions are removed. */
	      PREPEND_LIST (tlist, disposables);
	    }
	}
      else
	{
	  /* Dequote the string. */
	  temp_string = dequote_string (tlist->word->word);
	  free (tlist->word->word);
	  tlist->word->word = temp_string;
	  PREPEND_LIST (tlist, output_list);
	}

      strvec_dispose (glob_array);
      glob_array = (char **)NULL;

      tlist = next;
    }

  if (disposables)
    dispose_words (disposables);

  if (output_list)
    output_list = REVERSE_LIST (output_list, WORD_LIST *);

  return (output_list);
}

#if defined (BRACE_EXPANSION)
static WORD_LIST *
brace_expand_word_list (tlist, eflags)
     WORD_LIST *tlist;
     int eflags;
{
  register char **expansions;
  char *temp_string;
  WORD_LIST *disposables, *output_list, *next;
  WORD_DESC *w;
  int eindex;

  for (disposables = output_list = (WORD_LIST *)NULL; tlist; tlist = next)
    {
      next = tlist->next;

      if ((tlist->word->flags & (W_COMPASSIGN|W_ASSIGNARG)) == (W_COMPASSIGN|W_ASSIGNARG))
        {
/*itrace("brace_expand_word_list: %s: W_COMPASSIGN|W_ASSIGNARG", tlist->word->word);*/
	  PREPEND_LIST (tlist, output_list);
	  continue;
        }
          
      /* Only do brace expansion if the word has a brace character.  If
	 not, just add the word list element to BRACES and continue.  In
	 the common case, at least when running shell scripts, this will
	 degenerate to a bunch of calls to `mbschr', and then what is
	 basically a reversal of TLIST into BRACES, which is corrected
	 by a call to REVERSE_LIST () on BRACES when the end of TLIST
	 is reached. */
      if (mbschr (tlist->word->word, LBRACE))
	{
	  expansions = brace_expand (tlist->word->word);

	  for (eindex = 0; temp_string = expansions[eindex]; eindex++)
	    {
	      w = make_word (temp_string);
	      /* If brace expansion didn't change the word, preserve
		 the flags.  We may want to preserve the flags
		 unconditionally someday -- XXX */
	      if (STREQ (temp_string, tlist->word->word))
		w->flags = tlist->word->flags;
	      output_list = make_word_list (w, output_list);
	      free (expansions[eindex]);
	    }
	  free (expansions);

	  /* Add TLIST to the list of words to be freed after brace
	     expansion has been performed. */
	  PREPEND_LIST (tlist, disposables);
	}
      else
	PREPEND_LIST (tlist, output_list);
    }

  if (disposables)
    dispose_words (disposables);

  if (output_list)
    output_list = REVERSE_LIST (output_list, WORD_LIST *);

  return (output_list);
}
#endif

#if defined (ARRAY_VARS)
/* Take WORD, a compound associative array assignment, and internally run
   'declare -A w', where W is the variable name portion of WORD. */
static int
make_internal_declare (word, option)
     char *word;
     char *option;
{
  int t;
  WORD_LIST *wl;
  WORD_DESC *w;

  w = make_word (word);

  t = assignment (w->word, 0);
  w->word[t] = '\0';

  wl = make_word_list (w, (WORD_LIST *)NULL);
  wl = make_word_list (make_word (option), wl);

  return (declare_builtin (wl));  
}  
#endif

static WORD_LIST *
shell_expand_word_list (tlist, eflags)
     WORD_LIST *tlist;
     int eflags;
{
  WORD_LIST *expanded, *orig_list, *new_list, *next, *temp_list;
  int expanded_something, has_dollar_at;
  char *temp_string;

  /* We do tilde expansion all the time.  This is what 1003.2 says. */
  new_list = (WORD_LIST *)NULL;
  for (orig_list = tlist; tlist; tlist = next)
    {
      temp_string = tlist->word->word;

      next = tlist->next;

#if defined (ARRAY_VARS)
      /* If this is a compound array assignment to a builtin that accepts
         such assignments (e.g., `declare'), take the assignment and perform
         it separately, handling the semantics of declarations inside shell
         functions.  This avoids the double-evaluation of such arguments,
         because `declare' does some evaluation of compound assignments on
         its own. */
      if ((tlist->word->flags & (W_COMPASSIGN|W_ASSIGNARG)) == (W_COMPASSIGN|W_ASSIGNARG))
	{
	  int t;

	  if (tlist->word->flags & W_ASSIGNASSOC)
	    make_internal_declare (tlist->word->word, "-A");

	  t = do_word_assignment (tlist->word);
	  if (t == 0)
	    {
	      last_command_exit_value = EXECUTION_FAILURE;
	      exp_jump_to_top_level (DISCARD);
	    }

	  /* Now transform the word as ksh93 appears to do and go on */
	  t = assignment (tlist->word->word, 0);
	  tlist->word->word[t] = '\0';
	  tlist->word->flags &= ~(W_ASSIGNMENT|W_NOSPLIT|W_COMPASSIGN|W_ASSIGNARG|W_ASSIGNASSOC);
	}
#endif

      expanded_something = 0;
      expanded = expand_word_internal
	(tlist->word, 0, 0, &has_dollar_at, &expanded_something);

      if (expanded == &expand_word_error || expanded == &expand_word_fatal)
	{
	  /* By convention, each time this error is returned,
	     tlist->word->word has already been freed. */
	  tlist->word->word = (char *)NULL;

	  /* Dispose our copy of the original list. */
	  dispose_words (orig_list);
	  /* Dispose the new list we're building. */
	  dispose_words (new_list);

	  last_command_exit_value = EXECUTION_FAILURE;
	  if (expanded == &expand_word_error)
	    exp_jump_to_top_level (DISCARD);
	  else
	    exp_jump_to_top_level (FORCE_EOF);
	}

      /* Don't split words marked W_NOSPLIT. */
      if (expanded_something && (tlist->word->flags & W_NOSPLIT) == 0)
	{
	  temp_list = word_list_split (expanded);
	  dispose_words (expanded);
	}
      else
	{
	  /* If no parameter expansion, command substitution, process
	     substitution, or arithmetic substitution took place, then
	     do not do word splitting.  We still have to remove quoted
	     null characters from the result. */
	  word_list_remove_quoted_nulls (expanded);
	  temp_list = expanded;
	}

      expanded = REVERSE_LIST (temp_list, WORD_LIST *);
      new_list = (WORD_LIST *)list_append (expanded, new_list);
    }

  if (orig_list)  
    dispose_words (orig_list);

  if (new_list)
    new_list = REVERSE_LIST (new_list, WORD_LIST *);

  return (new_list);
}

/* The workhorse for expand_words () and expand_words_no_vars ().
   First arg is LIST, a WORD_LIST of words.
   Second arg EFLAGS is a flags word controlling which expansions are
   performed.

   This does all of the substitutions: brace expansion, tilde expansion,
   parameter expansion, command substitution, arithmetic expansion,
   process substitution, word splitting, and pathname expansion, according
   to the bits set in EFLAGS.  Words with the W_QUOTED or W_NOSPLIT bits
   set, or for which no expansion is done, do not undergo word splitting.
   Words with the W_NOGLOB bit set do not undergo pathname expansion. */
static WORD_LIST *
expand_word_list_internal (list, eflags)
     WORD_LIST *list;
     int eflags;
{
  WORD_LIST *new_list, *temp_list;
  int tint;

  if (list == 0)
    return ((WORD_LIST *)NULL);

  garglist = new_list = copy_word_list (list);
  if (eflags & WEXP_VARASSIGN)
    {
      garglist = new_list = separate_out_assignments (new_list);
      if (new_list == 0)
	{
	  if (subst_assign_varlist)
	    {
	      /* All the words were variable assignments, so they are placed
		 into the shell's environment. */
	      for (temp_list = subst_assign_varlist; temp_list; temp_list = temp_list->next)
		{
		  this_command_name = (char *)NULL;	/* no arithmetic errors */
		  tint = do_word_assignment (temp_list->word);
		  /* Variable assignment errors in non-interactive shells
		     running in Posix.2 mode cause the shell to exit. */
		  if (tint == 0)
		    {
		      last_command_exit_value = EXECUTION_FAILURE;
		      if (interactive_shell == 0 && posixly_correct)
			exp_jump_to_top_level (FORCE_EOF);
		      else
			exp_jump_to_top_level (DISCARD);
		    }
		}
	      dispose_words (subst_assign_varlist);
	      subst_assign_varlist = (WORD_LIST *)NULL;
	    }
	  return ((WORD_LIST *)NULL);
	}
    }

  /* Begin expanding the words that remain.  The expansions take place on
     things that aren't really variable assignments. */

#if defined (BRACE_EXPANSION)
  /* Do brace expansion on this word if there are any brace characters
     in the string. */
  if ((eflags & WEXP_BRACEEXP) && brace_expansion && new_list)
    new_list = brace_expand_word_list (new_list, eflags);
#endif /* BRACE_EXPANSION */

  /* Perform the `normal' shell expansions: tilde expansion, parameter and
     variable substitution, command substitution, arithmetic expansion,
     and word splitting. */
  new_list = shell_expand_word_list (new_list, eflags);

  /* Okay, we're almost done.  Now let's just do some filename
     globbing. */
  if (new_list)
    {
      if ((eflags & WEXP_PATHEXP) && disallow_filename_globbing == 0)
	/* Glob expand the word list unless globbing has been disabled. */
	new_list = glob_expand_word_list (new_list, eflags);
      else
	/* Dequote the words, because we're not performing globbing. */
	new_list = dequote_list (new_list);
    }

  if ((eflags & WEXP_VARASSIGN) && subst_assign_varlist)
    {
      sh_wassign_func_t *assign_func;

      /* If the remainder of the words expand to nothing, Posix.2 requires
	 that the variable and environment assignments affect the shell's
	 environment. */
      assign_func = new_list ? assign_in_env : do_word_assignment;
      tempenv_assign_error = 0;

      for (temp_list = subst_assign_varlist; temp_list; temp_list = temp_list->next)
	{
	  this_command_name = (char *)NULL;
	  assigning_in_environment = (assign_func == assign_in_env);
	  tint = (*assign_func) (temp_list->word);
	  assigning_in_environment = 0;
	  /* Variable assignment errors in non-interactive shells running
	     in Posix.2 mode cause the shell to exit. */
	  if (tint == 0)
	    {
	      if (assign_func == do_word_assignment)
		{
		  last_command_exit_value = EXECUTION_FAILURE;
		  if (interactive_shell == 0 && posixly_correct)
		    exp_jump_to_top_level (FORCE_EOF);
		  else
		    exp_jump_to_top_level (DISCARD);
		}
	      else
		tempenv_assign_error++;
	    }
	}

      dispose_words (subst_assign_varlist);
      subst_assign_varlist = (WORD_LIST *)NULL;
    }

#if 0
  tint = list_length (new_list) + 1;
  RESIZE_MALLOCED_BUFFER (glob_argv_flags, 0, tint, glob_argv_flags_size, 16);
  for (tint = 0, temp_list = new_list; temp_list; temp_list = temp_list->next)
    glob_argv_flags[tint++] = (temp_list->word->flags & W_GLOBEXP) ? '1' : '0';
  glob_argv_flags[tint] = '\0';
#endif

  return (new_list);
}
