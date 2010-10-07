/* subst.h -- Names of externally visible functions in subst.c. */

/* Copyright (C) 1993-2009 Free Software Foundation, Inc.

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

#if !defined (_SUBST_H_)
#define _SUBST_H_

#include "stdc.h"

/* Constants which specify how to handle backslashes and quoting in
   expand_word_internal ().  Q_DOUBLE_QUOTES means to use the function
   slashify_in_quotes () to decide whether the backslash should be
   retained.  Q_HERE_DOCUMENT means slashify_in_here_document () to
   decide whether to retain the backslash.  Q_KEEP_BACKSLASH means
   to unconditionally retain the backslash.  Q_PATQUOTE means that we're
   expanding a pattern ${var%#[#%]pattern} in an expansion surrounded
   by double quotes. */
#define Q_DOUBLE_QUOTES  0x01
#define Q_HERE_DOCUMENT  0x02
#define Q_KEEP_BACKSLASH 0x04
#define Q_PATQUOTE	 0x08
#define Q_QUOTED	 0x10
#define Q_ADDEDQUOTES	 0x20
#define Q_QUOTEDNULL	 0x40

/* Flag values controlling how assignment statements are treated. */
#define ASS_APPEND	0x01
#define ASS_MKLOCAL	0x02
#define ASS_MKASSOC	0x04

/* Flags for the string extraction functions. */
#define SX_NOALLOC	0x01	/* just skip; don't return substring */
#define SX_VARNAME	0x02	/* variable name; for string_extract () */
#define SX_REQMATCH	0x04	/* closing/matching delimiter required */
#define SX_COMMAND	0x08	/* extracting a shell script/command */
#define SX_NOCTLESC	0x10	/* don't honor CTLESC quoting */
#define SX_NOESCCTLNUL	0x20	/* don't let CTLESC quote CTLNUL */
#define SX_NOLONGJMP	0x40	/* don't longjmp on fatal error */
#define SX_ARITHSUB	0x80	/* extracting $(( ... )) (currently unused) */

/* Remove backslashes which are quoting backquotes from STRING.  Modifies
   STRING, and returns a pointer to it. */
extern char * de_backslash __P((char *));

/* Replace instances of \! in a string with !. */
extern void unquote_bang __P((char *));

/* Extract the $( construct in STRING, and return a new string.
   Start extracting at (SINDEX) as if we had just seen "$(".
   Make (SINDEX) get the position just after the matching ")".
   XFLAGS is additional flags to pass to other extraction functions, */
extern char *extract_command_subst __P((char *, int *, int));

/* Extract the $[ construct in STRING, and return a new string.
   Start extracting at (SINDEX) as if we had just seen "$[".
   Make (SINDEX) get the position just after the matching "]". */
extern char *extract_arithmetic_subst __P((char *, int *));

#if defined (PROCESS_SUBSTITUTION)
/* Extract the <( or >( construct in STRING, and return a new string.
   Start extracting at (SINDEX) as if we had just seen "<(".
   Make (SINDEX) get the position just after the matching ")". */
extern char *extract_process_subst __P((char *, char *, int *));
#endif /* PROCESS_SUBSTITUTION */

/* Extract the name of the variable to bind to from the assignment string. */
extern char *assignment_name __P((char *));

/* Return a single string of all the words present in LIST, separating
   each word with SEP. */
extern char *string_list_internal __P((WORD_LIST *, char *));

/* Return a single string of all the words present in LIST, separating
   each word with a space. */
extern char *string_list __P((WORD_LIST *));

/* Turn $* into a single string, obeying POSIX rules. */
extern char *string_list_dollar_star __P((WORD_LIST *));

/* Expand $@ into a single string, obeying POSIX rules. */
extern char *string_list_dollar_at __P((WORD_LIST *, int));

/* Turn the positional paramters into a string, understanding quoting and
   the various subtleties of using the first character of $IFS as the
   separator.  Calls string_list_dollar_at, string_list_dollar_star, and
   string_list as appropriate. */
extern char *string_list_pos_params __P((int, WORD_LIST *, int));

/* Perform quoted null character removal on each element of LIST.
   This modifies LIST. */
extern void word_list_remove_quoted_nulls __P((WORD_LIST *));

/* This performs word splitting and quoted null character removal on
   STRING. */
extern WORD_LIST *list_string __P((char *, char *, int));

extern char *ifs_firstchar  __P((int *));
extern char *get_word_from_string __P((char **, char *, char **));
extern char *strip_trailing_ifs_whitespace __P((char *, char *, int));

/* Given STRING, an assignment string, get the value of the right side
   of the `=', and bind it to the left side.  If EXPAND is true, then
   perform tilde expansion, parameter expansion, command substitution,
   and arithmetic expansion on the right-hand side.  Do not perform word
   splitting on the result of expansion. */
extern int do_assignment __P((char *));
extern int do_assignment_no_expand __P((char *));
extern int do_word_assignment __P((WORD_DESC *));

/* Append SOURCE to TARGET at INDEX.  SIZE is the current amount
   of space allocated to TARGET.  SOURCE can be NULL, in which
   case nothing happens.  Gets rid of SOURCE by free ()ing it.
   Returns TARGET in case the location has changed. */
extern char *sub_append_string __P((char *, char *, int *, int *));

/* Append the textual representation of NUMBER to TARGET.
   INDEX and SIZE are as in SUB_APPEND_STRING. */
extern char *sub_append_number __P((intmax_t, char *, int *, int *));

/* Return the word list that corresponds to `$*'. */
extern WORD_LIST *list_rest_of_args __P((void));

/* Make a single large string out of the dollar digit variables,
   and the rest_of_args.  If DOLLAR_STAR is 1, then obey the special
   case of "$*" with respect to IFS. */
extern char *string_rest_of_args __P((int));

extern int number_of_args __P((void));

/* Expand STRING by performing parameter expansion, command substitution,
   and arithmetic expansion.  Dequote the resulting WORD_LIST before
   returning it, but do not perform word splitting.  The call to
   remove_quoted_nulls () is made here because word splitting normally
   takes care of quote removal. */
extern WORD_LIST *expand_string_unsplit __P((char *, int));

/* Expand the rhs of an assignment statement. */
extern WORD_LIST *expand_string_assignment __P((char *, int));

/* Expand a prompt string. */
extern WORD_LIST *expand_prompt_string __P((char *, int, int));

/* Expand STRING just as if you were expanding a word.  This also returns
   a list of words.  Note that filename globbing is *NOT* done for word
   or string expansion, just when the shell is expanding a command.  This
   does parameter expansion, command substitution, arithmetic expansion,
   and word splitting.  Dequote the resultant WORD_LIST before returning. */
extern WORD_LIST *expand_string __P((char *, int));

/* Convenience functions that expand strings to strings, taking care of
   converting the WORD_LIST * returned by the expand_string* functions
   to a string and deallocating the WORD_LIST *. */
extern char *expand_string_to_string __P((char *, int));
extern char *expand_string_unsplit_to_string __P((char *, int));
extern char *expand_assignment_string_to_string __P((char *, int));

/* Expand an arithmetic expression string */
extern char *expand_arith_string __P((char *, int));

/* De-quote quoted characters in STRING. */
extern char *dequote_string __P((char *));

/* De-quote CTLESC-escaped CTLESC or CTLNUL characters in STRING. */
extern char *dequote_escapes __P((char *));

/* De-quote quoted characters in each word in LIST. */
extern WORD_LIST *dequote_list __P((WORD_LIST *));

/* Expand WORD, performing word splitting on the result.  This does
   parameter expansion, command substitution, arithmetic expansion,
   word splitting, and quote removal. */
extern WORD_LIST *expand_word __P((WORD_DESC *, int));

/* Expand WORD, but do not perform word splitting on the result.  This
   does parameter expansion, command substitution, arithmetic expansion,
   and quote removal. */
extern WORD_LIST *expand_word_unsplit __P((WORD_DESC *, int));
extern WORD_LIST *expand_word_leave_quoted __P((WORD_DESC *, int));

/* Return the value of a positional parameter.  This handles values > 10. */
extern char *get_dollar_var_value __P((intmax_t));

/* Quote a string to protect it from word splitting. */
extern char *quote_string __P((char *));

/* Quote escape characters (characters special to interals of expansion)
   in a string. */
extern char *quote_escapes __P((char *));

/* And remove such quoted special characters. */
extern char *remove_quoted_escapes __P((char *));

/* Remove CTLNUL characters from STRING unless they are quoted with CTLESC. */
extern char *remove_quoted_nulls __P((char *));

/* Perform quote removal on STRING.  If QUOTED > 0, assume we are obeying the
   backslash quoting rules for within double quotes. */
extern char *string_quote_removal __P((char *, int));

/* Perform quote removal on word WORD.  This allocates and returns a new
   WORD_DESC *. */
extern WORD_DESC *word_quote_removal __P((WORD_DESC *, int));

/* Perform quote removal on all words in LIST.  If QUOTED is non-zero,
   the members of the list are treated as if they are surrounded by
   double quotes.  Return a new list, or NULL if LIST is NULL. */
extern WORD_LIST *word_list_quote_removal __P((WORD_LIST *, int));

/* Called when IFS is changed to maintain some private variables. */
extern void setifs __P((SHELL_VAR *));

/* Return the value of $IFS, or " \t\n" if IFS is unset. */
extern char *getifs __P((void));

/* This splits a single word into a WORD LIST on $IFS, but only if the word
   is not quoted.  list_string () performs quote removal for us, even if we
   don't do any splitting. */
extern WORD_LIST *word_split __P((WORD_DESC *, char *));

/* Take the list of words in LIST and do the various substitutions.  Return
   a new list of words which is the expanded list, and without things like
   variable assignments. */
extern WORD_LIST *expand_words __P((WORD_LIST *));

/* Same as expand_words (), but doesn't hack variable or environment
   variables. */
extern WORD_LIST *expand_words_no_vars __P((WORD_LIST *));

/* Perform the `normal shell expansions' on a WORD_LIST.  These are
   brace expansion, tilde expansion, parameter and variable substitution,
   command substitution, arithmetic expansion, and word splitting. */
extern WORD_LIST *expand_words_shellexp __P((WORD_LIST *));

extern WORD_DESC *command_substitute __P((char *, int));
extern char *pat_subst __P((char *, char *, char *, int));

extern int fifos_pending __P((void));
extern void unlink_fifo_list __P((void));

extern WORD_LIST *list_string_with_quotes __P((char *));

#if defined (ARRAY_VARS)
extern char *extract_array_assignment_list __P((char *, int *));
#endif

#if defined (COND_COMMAND)
extern char *remove_backslashes __P((char *));
extern char *cond_expand_word __P((WORD_DESC *, int));
#endif

/* Flags for skip_to_delim */
#define SD_NOJMP	0x01	/* don't longjmp on fatal error. */
#define SD_INVERT	0x02	/* look for chars NOT in passed set */
#define SD_NOQUOTEDELIM	0x04	/* don't let single or double quotes act as delimiters */
#define SD_NOSKIPCMD	0x08	/* don't skip over $(, <(, or >( command/process substitution */

extern int skip_to_delim __P((char *, int, char *, int));

#if defined (READLINE)
extern int char_is_quoted __P((char *, int));
extern int unclosed_pair __P((char *, int, char *));
extern WORD_LIST *split_at_delims __P((char *, int, char *, int, int, int *, int *));
#endif

/* Variables used to keep track of the characters in IFS. */
extern SHELL_VAR *ifs_var;
extern char *ifs_value;
extern unsigned char ifs_cmap[];

#if defined (HANDLE_MULTIBYTE)
extern unsigned char ifs_firstc[];
extern size_t ifs_firstc_len;
#else
extern unsigned char ifs_firstc;
#endif

/* Evaluates to 1 if C is a character in $IFS. */
#define isifs(c)	(ifs_cmap[(unsigned char)(c)] != 0)

/* How to determine the quoted state of the character C. */
#define QUOTED_CHAR(c)  ((c) == CTLESC)

/* Is the first character of STRING a quoted NULL character? */
#define QUOTED_NULL(string) ((string)[0] == CTLNUL && (string)[1] == '\0')

#endif /* !_SUBST_H_ */
