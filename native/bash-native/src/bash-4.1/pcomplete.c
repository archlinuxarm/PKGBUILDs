/* pcomplete.c - functions to generate lists of matches for programmable completion. */

/* Copyright (C) 1999-2009 Free Software Foundation, Inc.

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

#include <config.h>

#if defined (PROGRAMMABLE_COMPLETION)

#include "bashtypes.h"
#include "posixstat.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <signal.h>

#if defined (PREFER_STDARG)
#  include <stdarg.h>
#else
#  include <varargs.h>
#endif

#include <stdio.h>
#include "bashansi.h"
#include "bashintl.h"

#include "shell.h"
#include "pcomplete.h"
#include "alias.h"
#include "bashline.h"
#include "execute_cmd.h"
#include "pathexp.h"

#if defined (JOB_CONTROL)
#  include "jobs.h"
#endif

#if !defined (NSIG)
#  include "trap.h"
#endif

#include "builtins.h"
#include "builtins/common.h"

#include <glob/glob.h>
#include <glob/strmatch.h>

#include <readline/rlconf.h>
#include <readline/readline.h>
#include <readline/history.h>

#define PCOMP_RETRYFAIL	256

#ifdef STRDUP
#  undef STRDUP
#endif
#define STRDUP(x)	((x) ? savestring (x) : (char *)NULL)

typedef SHELL_VAR **SVFUNC ();

#ifndef HAVE_STRPBRK
extern char *strpbrk __P((char *, char *));
#endif

extern int array_needs_making;
extern STRING_INT_ALIST word_token_alist[];
extern char *signal_names[];

#if defined (DEBUG)
#if defined (PREFER_STDARG)
static void debug_printf (const char *, ...)  __attribute__((__format__ (printf, 1, 2)));
#endif
#endif /* DEBUG */

static int it_init_joblist __P((ITEMLIST *, int));

static int it_init_aliases __P((ITEMLIST *));
static int it_init_arrayvars __P((ITEMLIST *));
static int it_init_bindings __P((ITEMLIST *));
static int it_init_builtins __P((ITEMLIST *));
static int it_init_disabled __P((ITEMLIST *));
static int it_init_enabled __P((ITEMLIST *));
static int it_init_exported __P((ITEMLIST *));
static int it_init_functions __P((ITEMLIST *));
static int it_init_hostnames __P((ITEMLIST *));
static int it_init_jobs __P((ITEMLIST *));
static int it_init_running __P((ITEMLIST *));
static int it_init_stopped __P((ITEMLIST *));
static int it_init_keywords __P((ITEMLIST *));
static int it_init_signals __P((ITEMLIST *));
static int it_init_variables __P((ITEMLIST *));
static int it_init_setopts __P((ITEMLIST *));
static int it_init_shopts __P((ITEMLIST *));

static int shouldexp_filterpat __P((char *));
static char *preproc_filterpat __P((char *, char *));

static void init_itemlist_from_varlist __P((ITEMLIST *, SVFUNC *));

static STRINGLIST *gen_matches_from_itemlist __P((ITEMLIST *, const char *));
static STRINGLIST *gen_action_completions __P((COMPSPEC *, const char *));
static STRINGLIST *gen_globpat_matches __P((COMPSPEC *, const char *));
static STRINGLIST *gen_wordlist_matches __P((COMPSPEC *, const char *));
static STRINGLIST *gen_shell_function_matches __P((COMPSPEC *, const char *,
						   char *, int, WORD_LIST *,
						   int, int, int *));
static STRINGLIST *gen_command_matches __P((COMPSPEC *, const char *, char *,
					    int, WORD_LIST *, int, int));

static STRINGLIST *gen_progcomp_completions __P((const char *, const char *,
						 const char *,
						 int, int, int *, int *,
						 COMPSPEC **));

static char *pcomp_filename_completion_function __P((const char *, int));

#if defined (ARRAY_VARS)
static SHELL_VAR *bind_comp_words __P((WORD_LIST *));
#endif
static void bind_compfunc_variables __P((char *, int, WORD_LIST *, int, int));
static void unbind_compfunc_variables __P((int));
static WORD_LIST *build_arg_list __P((char *, const char *, WORD_LIST *, int));
static WORD_LIST *command_line_to_word_list __P((char *, int, int, int *, int *));

#ifdef DEBUG
static int progcomp_debug = 0;
#endif

int prog_completion_enabled = 1;

/* These are used to manage the arrays of strings for possible completions. */
ITEMLIST it_aliases = { 0, it_init_aliases, (STRINGLIST *)0 };
ITEMLIST it_arrayvars  = { LIST_DYNAMIC, it_init_arrayvars, (STRINGLIST *)0 };
ITEMLIST it_bindings  = { 0, it_init_bindings, (STRINGLIST *)0 };
ITEMLIST it_builtins  = { 0, it_init_builtins, (STRINGLIST *)0 };
ITEMLIST it_commands = { LIST_DYNAMIC };	/* unused */
ITEMLIST it_directories = { LIST_DYNAMIC };	/* unused */
ITEMLIST it_disabled = { 0, it_init_disabled, (STRINGLIST *)0 };
ITEMLIST it_enabled = { 0, it_init_enabled, (STRINGLIST *)0 };
ITEMLIST it_exports  = { LIST_DYNAMIC, it_init_exported, (STRINGLIST *)0 };
ITEMLIST it_files = { LIST_DYNAMIC };		/* unused */
ITEMLIST it_functions  = { 0, it_init_functions, (STRINGLIST *)0 };
ITEMLIST it_hostnames  = { LIST_DYNAMIC, it_init_hostnames, (STRINGLIST *)0 };
ITEMLIST it_groups = { LIST_DYNAMIC };		/* unused */
ITEMLIST it_jobs = { LIST_DYNAMIC, it_init_jobs, (STRINGLIST *)0 };
ITEMLIST it_keywords = { 0, it_init_keywords, (STRINGLIST *)0 };
ITEMLIST it_running = { LIST_DYNAMIC, it_init_running, (STRINGLIST *)0 };
ITEMLIST it_services = { LIST_DYNAMIC };	/* unused */
ITEMLIST it_setopts = { 0, it_init_setopts, (STRINGLIST *)0 };
ITEMLIST it_shopts = { 0, it_init_shopts, (STRINGLIST *)0 };
ITEMLIST it_signals = { 0, it_init_signals, (STRINGLIST *)0 };
ITEMLIST it_stopped = { LIST_DYNAMIC, it_init_stopped, (STRINGLIST *)0 };
ITEMLIST it_users = { LIST_DYNAMIC };		/* unused */
ITEMLIST it_variables = { LIST_DYNAMIC, it_init_variables, (STRINGLIST *)0 };

COMPSPEC *pcomp_curcs;
const char *pcomp_curcmd;

#ifdef DEBUG
/* Debugging code */
static void
#if defined (PREFER_STDARG)
debug_printf (const char *format, ...)
#else
debug_printf (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  va_list args;

  if (progcomp_debug == 0)
    return;

  SH_VA_START (args, format);

  fprintf (stdout, "DEBUG: ");
  vfprintf (stdout, format, args);
  fprintf (stdout, "\n");

  rl_on_new_line ();

  va_end (args);
}
#endif

/* Functions to manage the item lists */

void
set_itemlist_dirty (it)
     ITEMLIST *it;
{
  it->flags |= LIST_DIRTY;
}

void
initialize_itemlist (itp)
     ITEMLIST *itp;
{
  (*itp->list_getter) (itp);
  itp->flags |= LIST_INITIALIZED;
  itp->flags &= ~LIST_DIRTY;
}

void
clean_itemlist (itp)
     ITEMLIST *itp;
{
  STRINGLIST *sl;

  sl = itp->slist;
  if (sl)
    {
      if ((itp->flags & (LIST_DONTFREEMEMBERS|LIST_DONTFREE)) == 0)
	strvec_flush (sl->list);
      if ((itp->flags & LIST_DONTFREE) == 0)
	free (sl->list);
      free (sl);
    }
  itp->slist = (STRINGLIST *)NULL;
  itp->flags &= ~(LIST_DONTFREE|LIST_DONTFREEMEMBERS|LIST_INITIALIZED|LIST_DIRTY);
}


static int
shouldexp_filterpat (s)
     char *s;
{
  register char *p;

  for (p = s; p && *p; p++)
    {
      if (*p == '\\')
	p++;
      else if (*p == '&')
	return 1;
    }
  return 0;
}

/* Replace any instance of `&' in PAT with TEXT.  Backslash may be used to
   quote a `&' and inhibit substitution.  Returns a new string.  This just
   calls stringlib.c:strcreplace(). */
static char *
preproc_filterpat (pat, text)
     char *pat;
     char *text;
{
  char *ret;

  ret = strcreplace (pat, '&', text, 1);
  return ret;
}
	
/* Remove any match of FILTERPAT from SL.  A `&' in FILTERPAT is replaced by
   TEXT.  A leading `!' in FILTERPAT negates the pattern; in this case
   any member of SL->list that does *not* match will be removed.  This returns
   a new STRINGLIST with the matching members of SL *copied*.  Any
   non-matching members of SL->list are *freed*. */   
STRINGLIST *
filter_stringlist (sl, filterpat, text)
     STRINGLIST *sl;
     char *filterpat, *text;
{
  int i, m, not;
  STRINGLIST *ret;
  char *npat, *t;

  if (sl == 0 || sl->list == 0 || sl->list_len == 0)
    return sl;

  npat = shouldexp_filterpat (filterpat) ? preproc_filterpat (filterpat, text) : filterpat;

  not = (npat[0] == '!');
  t = not ? npat + 1 : npat;

  ret = strlist_create (sl->list_size);
  for (i = 0; i < sl->list_len; i++)
    {
      m = strmatch (t, sl->list[i], FNMATCH_EXTFLAG);
      if ((not && m == FNM_NOMATCH) || (not == 0 && m != FNM_NOMATCH))
	free (sl->list[i]);
      else
	ret->list[ret->list_len++] = sl->list[i];
    }

  ret->list[ret->list_len] = (char *)NULL;
  if (npat != filterpat)
    free (npat);

  return ret;
}

/* Turn an array of strings returned by rl_completion_matches into a STRINGLIST.
   This understands how rl_completion_matches sets matches[0] (the lcd of the
   strings in the list, unless it's the only match). */
STRINGLIST *
completions_to_stringlist (matches)
     char **matches;
{
  STRINGLIST *sl;
  int mlen, i, n;

  mlen = (matches == 0) ? 0 : strvec_len (matches);
  sl = strlist_create (mlen + 1);

  if (matches == 0 || matches[0] == 0)
    return sl;

  if (matches[1] == 0)
    {
      sl->list[0] = STRDUP (matches[0]);
      sl->list[sl->list_len = 1] = (char *)NULL;
      return sl;
    }

  for (i = 1, n = 0; i < mlen; i++, n++)
    sl->list[n] = STRDUP (matches[i]);
  sl->list_len = n;
  sl->list[n] = (char *)NULL;

  return sl;
}

/* Functions to manage the various ITEMLISTs that we populate internally.
   The caller is responsible for setting ITP->flags correctly. */

static int
it_init_aliases (itp)
     ITEMLIST *itp;
{
#ifdef ALIAS
  alias_t **alias_list;
  register int i, n;
  STRINGLIST *sl;

  alias_list = all_aliases ();
  if (alias_list == 0)
    {
      itp->slist = (STRINGLIST *)NULL;
      return 0;
    }
  for (n = 0; alias_list[n]; n++)
    ;
  sl = strlist_create (n+1);
  for (i = 0; i < n; i++)
    sl->list[i] = STRDUP (alias_list[i]->name);
  sl->list[n] = (char *)NULL;
  sl->list_size = sl->list_len = n;
  itp->slist = sl;
#else
  itp->slist = (STRINGLIST *)NULL;
#endif
  return 1;
}

static void
init_itemlist_from_varlist (itp, svfunc)
     ITEMLIST *itp;
     SVFUNC *svfunc;
{
  SHELL_VAR **vlist;
  STRINGLIST *sl;
  register int i, n;

  vlist = (*svfunc) ();
  if (vlist == 0)
    {
      itp->slist = (STRINGLIST *)NULL;
      return;
    }    
  for (n = 0; vlist[n]; n++)
    ;
  sl = strlist_create (n+1);
  for (i = 0; i < n; i++)
    sl->list[i] = savestring (vlist[i]->name);
  sl->list[sl->list_len = n] = (char *)NULL;
  itp->slist = sl;
}

static int
it_init_arrayvars (itp)
     ITEMLIST *itp;
{
#if defined (ARRAY_VARS)
  init_itemlist_from_varlist (itp, all_array_variables);
  return 1;
#else
  return 0;
#endif
}

static int
it_init_bindings (itp)
     ITEMLIST *itp;
{
  char **blist;
  STRINGLIST *sl;

  /* rl_funmap_names allocates blist, but not its members */
  blist = (char **)rl_funmap_names ();	/* XXX fix const later */
  sl = strlist_create (0);
  sl->list = blist;
  sl->list_size = 0;
  sl->list_len = strvec_len (sl->list);
  itp->flags |= LIST_DONTFREEMEMBERS;
  itp->slist = sl;

  return 0;
}

static int
it_init_builtins (itp)
     ITEMLIST *itp;
{
  STRINGLIST *sl;
  register int i, n;

  sl = strlist_create (num_shell_builtins);
  for (i = n = 0; i < num_shell_builtins; i++)
    if (shell_builtins[i].function)
      sl->list[n++] = shell_builtins[i].name;
  sl->list[sl->list_len = n] = (char *)NULL;
  itp->flags |= LIST_DONTFREEMEMBERS;
  itp->slist = sl;
  return 0;
}

static int
it_init_enabled (itp)
     ITEMLIST *itp;
{
  STRINGLIST *sl;
  register int i, n;

  sl = strlist_create (num_shell_builtins);
  for (i = n = 0; i < num_shell_builtins; i++)
    {
      if (shell_builtins[i].function && (shell_builtins[i].flags & BUILTIN_ENABLED))
	sl->list[n++] = shell_builtins[i].name;
    }
  sl->list[sl->list_len = n] = (char *)NULL;
  itp->flags |= LIST_DONTFREEMEMBERS;
  itp->slist = sl;
  return 0;
}

static int
it_init_disabled (itp)
     ITEMLIST *itp;
{
  STRINGLIST *sl;
  register int i, n;

  sl = strlist_create (num_shell_builtins);
  for (i = n = 0; i < num_shell_builtins; i++)
    {
      if (shell_builtins[i].function && ((shell_builtins[i].flags & BUILTIN_ENABLED) == 0))
	sl->list[n++] = shell_builtins[i].name;
    }
  sl->list[sl->list_len = n] = (char *)NULL;
  itp->flags |= LIST_DONTFREEMEMBERS;
  itp->slist = sl;
  return 0;
}

static int
it_init_exported (itp)
     ITEMLIST *itp;
{
  init_itemlist_from_varlist (itp, all_exported_variables);
  return 0;
}

static int
it_init_functions (itp)
     ITEMLIST *itp;
{
  init_itemlist_from_varlist (itp, all_visible_functions);
  return 0;
}

static int
it_init_hostnames (itp)
     ITEMLIST *itp;
{
  STRINGLIST *sl;

  sl = strlist_create (0);
  sl->list = get_hostname_list ();
  sl->list_len = sl->list ? strvec_len (sl->list) : 0;
  sl->list_size = sl->list_len;
  itp->slist = sl;
  itp->flags |= LIST_DONTFREEMEMBERS|LIST_DONTFREE;
  return 0;
}

static int
it_init_joblist (itp, jstate)
     ITEMLIST *itp;
     int jstate;
{
#if defined (JOB_CONTROL)
  STRINGLIST *sl;
  register int i;
  register PROCESS *p;
  char *s, *t;
  JOB *j;
  JOB_STATE ws;		/* wanted state */

  ws = JNONE;
  if (jstate == 0)
    ws = JRUNNING;
  else if (jstate == 1)
    ws = JSTOPPED;

  sl = strlist_create (js.j_jobslots);
  for (i = js.j_jobslots - 1; i >= 0; i--)
    {
      j = get_job_by_jid (i);
      if (j == 0)
	continue;
      p = j->pipe;
      if (jstate == -1 || JOBSTATE(i) == ws)
	{
	  s = savestring (p->command);
	  t = strpbrk (s, " \t\n");
	  if (t)
	    *t = '\0';
	  sl->list[sl->list_len++] = s;
	}
    }
  itp->slist = sl;
#else
  itp->slist = (STRINGLIST *)NULL;
#endif
  return 0;
}

static int
it_init_jobs (itp)
     ITEMLIST *itp;
{
  return (it_init_joblist (itp, -1));
}

static int
it_init_running (itp)
     ITEMLIST *itp;
{
  return (it_init_joblist (itp, 0));
}

static int
it_init_stopped (itp)
     ITEMLIST *itp;
{
  return (it_init_joblist (itp, 1));
}

static int
it_init_keywords (itp)
     ITEMLIST *itp;
{
  STRINGLIST *sl;
  register int i, n;

  for (n = 0; word_token_alist[n].word; n++)
    ;
  sl = strlist_create (n);
  for (i = 0; i < n; i++)
    sl->list[i] = word_token_alist[i].word;
  sl->list[sl->list_len = i] = (char *)NULL;
  itp->flags |= LIST_DONTFREEMEMBERS;
  itp->slist = sl;
  return 0;
}

static int
it_init_signals (itp)
     ITEMLIST *itp;
{
  STRINGLIST *sl;

  sl = strlist_create (0);
  sl->list = signal_names;
  sl->list_len = strvec_len (sl->list);
  itp->flags |= LIST_DONTFREE;
  itp->slist = sl;
  return 0;
}

static int
it_init_variables (itp)
     ITEMLIST *itp;
{
  init_itemlist_from_varlist (itp, all_visible_variables);
  return 0;
}

static int
it_init_setopts (itp)
     ITEMLIST *itp;
{
  STRINGLIST *sl;

  sl = strlist_create (0);
  sl->list = get_minus_o_opts ();
  sl->list_len = strvec_len (sl->list);
  itp->slist = sl;
  itp->flags |= LIST_DONTFREEMEMBERS;
  return 0;
}

static int
it_init_shopts (itp)
     ITEMLIST *itp;
{
  STRINGLIST *sl;

  sl = strlist_create (0);
  sl->list = get_shopt_options ();
  sl->list_len = strvec_len (sl->list);
  itp->slist = sl;
  itp->flags |= LIST_DONTFREEMEMBERS;
  return 0;
}

/* Generate a list of all matches for TEXT using the STRINGLIST in itp->slist
   as the list of possibilities.  If the itemlist has been marked dirty or
   it should be regenerated every time, destroy the old STRINGLIST and make a
   new one before trying the match.  TEXT is dequoted before attempting a
   match. */
static STRINGLIST *
gen_matches_from_itemlist (itp, text)
     ITEMLIST *itp;
     const char *text;
{
  STRINGLIST *ret, *sl;
  int tlen, i, n;
  char *ntxt;

  if ((itp->flags & (LIST_DIRTY|LIST_DYNAMIC)) ||
      (itp->flags & LIST_INITIALIZED) == 0)
    {
      if (itp->flags & (LIST_DIRTY|LIST_DYNAMIC))
	clean_itemlist (itp);
      if ((itp->flags & LIST_INITIALIZED) == 0)
	initialize_itemlist (itp);
    }
  if (itp->slist == 0)
    return ((STRINGLIST *)NULL);
  ret = strlist_create (itp->slist->list_len+1);
  sl = itp->slist;

  ntxt = bash_dequote_text (text);
  tlen = STRLEN (ntxt);

  for (i = n = 0; i < sl->list_len; i++)
    {
      if (tlen == 0 || STREQN (sl->list[i], ntxt, tlen))
	ret->list[n++] = STRDUP (sl->list[i]);
    }
  ret->list[ret->list_len = n] = (char *)NULL;

  FREE (ntxt);
  return ret;
}

/* A wrapper for rl_filename_completion_function that dequotes the filename
   before attempting completions. */
static char *
pcomp_filename_completion_function (text, state)
     const char *text;
     int state;
{
  static char *dfn;	/* dequoted filename */
  int qc;

  if (state == 0)
    {
      FREE (dfn);
      /* remove backslashes quoting special characters in filenames. */
#if 1
      if (RL_ISSTATE (RL_STATE_COMPLETING) && rl_filename_dequoting_function)
#else
      if (rl_filename_dequoting_function)
#endif
	{
	  /* Use rl_completion_quote_character because any single or
	     double quotes have been removed by the time TEXT makes it
	     here, and we don't want to remove backslashes inside
	     quoted strings. */
	  dfn = (*rl_filename_dequoting_function) ((char *)text, rl_completion_quote_character);
	}
      else
	dfn = savestring (text);
    }

  return (rl_filename_completion_function (dfn, state));
}

#define GEN_COMPS(bmap, flag, it, text, glist, tlist) \
  do { \
    if (bmap & flag) \
      { \
	tlist = gen_matches_from_itemlist (it, text); \
	if (tlist) \
	  { \
	    glist = strlist_append (glist, tlist); \
	    strlist_dispose (tlist); \
	  } \
      } \
  } while (0)

#define GEN_XCOMPS(bmap, flag, text, func, cmatches, glist, tlist) \
  do { \
    if (bmap & flag) \
      { \
	cmatches = rl_completion_matches (text, func); \
	tlist = completions_to_stringlist (cmatches); \
	glist = strlist_append (glist, tlist); \
	strvec_dispose (cmatches); \
	strlist_dispose (tlist); \
      } \
  } while (0)

/* Functions to generate lists of matches from the actions member of CS. */

static STRINGLIST *
gen_action_completions (cs, text)
     COMPSPEC *cs;
     const char *text;
{
  STRINGLIST *ret, *tmatches;
  char **cmatches;	/* from rl_completion_matches ... */
  unsigned long flags;

  ret = tmatches = (STRINGLIST *)NULL;
  flags = cs->actions;

  GEN_COMPS (flags, CA_ALIAS, &it_aliases, text, ret, tmatches);
  GEN_COMPS (flags, CA_ARRAYVAR, &it_arrayvars, text, ret, tmatches);
  GEN_COMPS (flags, CA_BINDING, &it_bindings, text, ret, tmatches);
  GEN_COMPS (flags, CA_BUILTIN, &it_builtins, text, ret, tmatches);
  GEN_COMPS (flags, CA_DISABLED, &it_disabled, text, ret, tmatches);
  GEN_COMPS (flags, CA_ENABLED, &it_enabled, text, ret, tmatches);
  GEN_COMPS (flags, CA_EXPORT, &it_exports, text, ret, tmatches);
  GEN_COMPS (flags, CA_FUNCTION, &it_functions, text, ret, tmatches);
  GEN_COMPS (flags, CA_HOSTNAME, &it_hostnames, text, ret, tmatches);
  GEN_COMPS (flags, CA_JOB, &it_jobs, text, ret, tmatches);
  GEN_COMPS (flags, CA_KEYWORD, &it_keywords, text, ret, tmatches);
  GEN_COMPS (flags, CA_RUNNING, &it_running, text, ret, tmatches);
  GEN_COMPS (flags, CA_SETOPT, &it_setopts, text, ret, tmatches);
  GEN_COMPS (flags, CA_SHOPT, &it_shopts, text, ret, tmatches);
  GEN_COMPS (flags, CA_SIGNAL, &it_signals, text, ret, tmatches);
  GEN_COMPS (flags, CA_STOPPED, &it_stopped, text, ret, tmatches);
  GEN_COMPS (flags, CA_VARIABLE, &it_variables, text, ret, tmatches);

  GEN_XCOMPS(flags, CA_COMMAND, text, command_word_completion_function, cmatches, ret, tmatches);
  GEN_XCOMPS(flags, CA_FILE, text, pcomp_filename_completion_function, cmatches, ret, tmatches);
  GEN_XCOMPS(flags, CA_USER, text, rl_username_completion_function, cmatches, ret, tmatches);
  GEN_XCOMPS(flags, CA_GROUP, text, bash_groupname_completion_function, cmatches, ret, tmatches);
  GEN_XCOMPS(flags, CA_SERVICE, text, bash_servicename_completion_function, cmatches, ret, tmatches);

  /* And lastly, the special case for directories */
  if (flags & CA_DIRECTORY)
    {
      rl_completion_mark_symlink_dirs = 1;	/* override user preference */
      cmatches = bash_directory_completion_matches (text);
      tmatches = completions_to_stringlist (cmatches);
      ret = strlist_append (ret, tmatches);
      strvec_dispose (cmatches);
      strlist_dispose (tmatches);
    }

  return ret;
}

/* Generate a list of matches for CS->globpat.  Unresolved: should this use
   TEXT as a match prefix, or just go without?  Currently, the code does not
   use TEXT, just globs CS->globpat and returns the results.  If we do decide
   to use TEXT, we should call quote_string_for_globbing before the call to
   glob_filename. */
static STRINGLIST *
gen_globpat_matches (cs, text)
      COMPSPEC *cs;
      const char *text;
{
  STRINGLIST *sl;

  sl = strlist_create (0);
  sl->list = glob_filename (cs->globpat, 0);
  if (GLOB_FAILED (sl->list))
    sl->list = (char **)NULL;
  if (sl->list)
    sl->list_len = sl->list_size = strvec_len (sl->list);
  return sl;
}

/* Perform the shell word expansions on CS->words and return the results.
   Again, this ignores TEXT. */
static STRINGLIST *
gen_wordlist_matches (cs, text)
     COMPSPEC *cs;
     const char *text;
{
  WORD_LIST *l, *l2;
  STRINGLIST *sl;
  int nw, tlen;
  char *ntxt;		/* dequoted TEXT to use in comparisons */

  if (cs->words == 0 || cs->words[0] == '\0')
    return ((STRINGLIST *)NULL);

  /* This used to be a simple expand_string(cs->words, 0), but that won't
     do -- there's no way to split a simple list into individual words
     that way, since the shell semantics say that word splitting is done
     only on the results of expansion. */
  l = split_at_delims (cs->words, strlen (cs->words), (char *)NULL, -1, 0, (int *)NULL, (int *)NULL);
  if (l == 0)
    return ((STRINGLIST *)NULL);
  /* This will jump back to the top level if the expansion fails... */
  l2 = expand_words_shellexp (l);
  dispose_words (l);

  nw = list_length (l2);
  sl = strlist_create (nw + 1);

  ntxt = bash_dequote_text (text);
  tlen = STRLEN (ntxt);

  for (nw = 0, l = l2; l; l = l->next)
    {
      if (tlen == 0 || STREQN (l->word->word, ntxt, tlen))
	sl->list[nw++] = STRDUP (l->word->word);
    }
  sl->list[sl->list_len = nw] = (char *)NULL;

  dispose_words (l2);
  FREE (ntxt);
  return sl;
}

#ifdef ARRAY_VARS

static SHELL_VAR *
bind_comp_words (lwords)
     WORD_LIST *lwords;
{
  SHELL_VAR *v;

  v = find_variable ("COMP_WORDS");
  if (v == 0)
    v = make_new_array_variable ("COMP_WORDS");
  if (readonly_p (v))
    VUNSETATTR (v, att_readonly);
  if (array_p (v) == 0)
    v = convert_var_to_array (v);
  v = assign_array_var_from_word_list (v, lwords, 0);

  VUNSETATTR (v, att_invisible);
  return v;
}
#endif /* ARRAY_VARS */

static void
bind_compfunc_variables (line, ind, lwords, cw, exported)
     char *line;
     int ind;
     WORD_LIST *lwords;
     int cw, exported;
{
  char ibuf[INT_STRLEN_BOUND(int) + 1];
  char *value;
  SHELL_VAR *v;

  /* Set the variables that the function expects while it executes.  Maybe
     these should be in the function environment (temporary_env). */
  v = bind_variable ("COMP_LINE", line, 0);
  if (v && exported)
    VSETATTR(v, att_exported);

  value = inttostr (ind, ibuf, sizeof(ibuf));
  v = bind_int_variable ("COMP_POINT", value);
  if (v && exported)
    VSETATTR(v, att_exported);

  value = inttostr (rl_completion_type, ibuf, sizeof (ibuf));
  v = bind_int_variable ("COMP_TYPE", value);
  if (v && exported)
    VSETATTR(v, att_exported);

  value = inttostr (rl_completion_invoking_key, ibuf, sizeof (ibuf));
  v = bind_int_variable ("COMP_KEY", value);
  if (v && exported)
    VSETATTR(v, att_exported);

  /* Since array variables can't be exported, we don't bother making the
     array of words. */
  if (exported == 0)
    {
#ifdef ARRAY_VARS
      v = bind_comp_words (lwords);
      value = inttostr (cw, ibuf, sizeof(ibuf));
      bind_int_variable ("COMP_CWORD", value);
#endif
    }
  else
    array_needs_making = 1;
}

static void
unbind_compfunc_variables (exported)
     int exported;
{
  unbind_variable ("COMP_LINE");
  unbind_variable ("COMP_POINT");
  unbind_variable ("COMP_TYPE");
  unbind_variable ("COMP_KEY");
#ifdef ARRAY_VARS
  unbind_variable ("COMP_WORDS");
  unbind_variable ("COMP_CWORD");
#endif
  if (exported)
    array_needs_making = 1;
}

/* Build the list of words to pass to a function or external command
   as arguments.  When the function or command is invoked,

	$0 == function or command being invoked
   	$1 == command name
   	$2 == word to be completed (possibly null)
   	$3 == previous word

   Functions can access all of the words in the current command line
   with the COMP_WORDS array.  External commands cannot; they have to
   make do  with the COMP_LINE and COMP_POINT variables. */

static WORD_LIST *
build_arg_list (cmd, text, lwords, ind)
     char *cmd;
     const char *text;
     WORD_LIST *lwords;
     int ind;
{
  WORD_LIST *ret, *cl, *l;
  WORD_DESC *w;
  int i;

  ret = (WORD_LIST *)NULL;
  w = make_word (cmd);
  ret = make_word_list (w, (WORD_LIST *)NULL);

  w = (lwords && lwords->word) ? copy_word (lwords->word) : make_word ("");
  cl = ret->next = make_word_list (w, (WORD_LIST *)NULL);

  w = make_word (text);
  cl->next = make_word_list (w, (WORD_LIST *)NULL);
  cl = cl->next;

  /* Search lwords for current word */
  for (l = lwords, i = 1; l && i < ind-1; l = l->next, i++)
    ;
  w = (l && l->word) ? copy_word (l->word) : make_word ("");
  cl->next = make_word_list (w, (WORD_LIST *)NULL);

  return ret;
}

/* Build a command string with
	$0 == cs->funcname	(function to execute for completion list)
   	$1 == command name	(command being completed)
   	$2 = word to be completed (possibly null)
   	$3 = previous word
   and run in the current shell.  The function should put its completion
   list into the array variable COMPREPLY.  We build a STRINGLIST
   from the results and return it.

   Since the shell function should return its list of matches in an array
   variable, this does nothing if arrays are not compiled into the shell. */

static STRINGLIST *
gen_shell_function_matches (cs, text, line, ind, lwords, nw, cw, foundp)
     COMPSPEC *cs;
     const char *text;
     char *line;
     int ind;
     WORD_LIST *lwords;
     int nw, cw;
     int *foundp;
{
  char *funcname;
  STRINGLIST *sl;
  SHELL_VAR *f, *v;
  WORD_LIST *cmdlist;
  int fval, found;
  sh_parser_state_t ps;
  sh_parser_state_t * restrict pps;
#if defined (ARRAY_VARS)
  ARRAY *a;
#endif

  found = 0;
  if (foundp)
    *foundp = found;

  funcname = cs->funcname;
  f = find_function (funcname);
  if (f == 0)
    {
      internal_error (_("completion: function `%s' not found"), funcname);
      rl_ding ();
      rl_on_new_line ();
      return ((STRINGLIST *)NULL);
    }

#if !defined (ARRAY_VARS)
  return ((STRINGLIST *)NULL);
#else

  /* We pass cw - 1 because command_line_to_word_list returns indices that are
     1-based, while bash arrays are 0-based. */
  bind_compfunc_variables (line, ind, lwords, cw - 1, 0);

  cmdlist = build_arg_list (funcname, text, lwords, cw);

  pps = &ps;
  save_parser_state (pps);
  begin_unwind_frame ("gen-shell-function-matches");
  add_unwind_protect (restore_parser_state, (char *)pps);
  add_unwind_protect (dispose_words, (char *)cmdlist);
  add_unwind_protect (unbind_compfunc_variables, (char *)0);

  fval = execute_shell_function (f, cmdlist);  

  discard_unwind_frame ("gen-shell-function-matches");
  restore_parser_state (pps);

  found = fval != EX_NOTFOUND;
  if (fval == EX_RETRYFAIL)
    found |= PCOMP_RETRYFAIL;
  if (foundp)
    *foundp = found;

  /* Now clean up and destroy everything. */
  dispose_words (cmdlist);
  unbind_compfunc_variables (0);

  /* The list of completions is returned in the array variable COMPREPLY. */
  v = find_variable ("COMPREPLY");
  if (v == 0)
    return ((STRINGLIST *)NULL);
  if (array_p (v) == 0)
    v = convert_var_to_array (v);

  VUNSETATTR (v, att_invisible);

  a = array_cell (v);
  if (found == 0 || (found & PCOMP_RETRYFAIL) || a == 0 || array_empty (a))
    sl = (STRINGLIST *)NULL;
  else
    {
      /* XXX - should we filter the list of completions so only those matching
	 TEXT are returned?  Right now, we do not. */
      sl = strlist_create (0);
      sl->list = array_to_argv (a);
      sl->list_len = sl->list_size = array_num_elements (a);
    }

  /* XXX - should we unbind COMPREPLY here? */
  unbind_variable ("COMPREPLY");

  return (sl);
#endif
}

/* Build a command string with
	$0 == cs->command	(command to execute for completion list)
   	$1 == command name	(command being completed)
   	$2 = word to be completed (possibly null)
   	$3 = previous word
   and run in with command substitution.  Parse the results, one word
   per line, with backslashes allowed to escape newlines.  Build a
   STRINGLIST from the results and return it. */

static STRINGLIST *
gen_command_matches (cs, text, line, ind, lwords, nw, cw)
     COMPSPEC *cs;
     const char *text;
     char *line;
     int ind;
     WORD_LIST *lwords;
     int nw, cw;
{
  char *csbuf, *cscmd, *t;
  int cmdlen, cmdsize, n, ws, we;
  WORD_LIST *cmdlist, *cl;
  WORD_DESC *tw;
  STRINGLIST *sl;

  bind_compfunc_variables (line, ind, lwords, cw, 1);
  cmdlist = build_arg_list (cs->command, text, lwords, cw);

  /* Estimate the size needed for the buffer. */
  n = strlen (cs->command);
  cmdsize = n + 1;
  for (cl = cmdlist->next; cl; cl = cl->next)
    cmdsize += STRLEN (cl->word->word) + 3;
  cmdsize += 2;

  /* allocate the string for the command and fill it in. */
  cscmd = (char *)xmalloc (cmdsize + 1);

  strcpy (cscmd, cs->command);			/* $0 */
  cmdlen = n;
  cscmd[cmdlen++] = ' ';
  for (cl = cmdlist->next; cl; cl = cl->next)   /* $1, $2, $3, ... */
    {
      t = sh_single_quote (cl->word->word ? cl->word->word : "");
      n = strlen (t);
      RESIZE_MALLOCED_BUFFER (cscmd, cmdlen, n + 2, cmdsize, 64);
      strcpy (cscmd + cmdlen, t);
      cmdlen += n;
      if (cl->next)
	cscmd[cmdlen++] = ' ';
      free (t);
    }
  cscmd[cmdlen] = '\0';

  tw = command_substitute (cscmd, 0);
  csbuf = tw ? tw->word : (char *)NULL;
  dispose_word_desc (tw);

  /* Now clean up and destroy everything. */
  dispose_words (cmdlist);
  free (cscmd);
  unbind_compfunc_variables (1);

  if (csbuf == 0 || *csbuf == '\0')
    {
      FREE (csbuf);
      return ((STRINGLIST *)NULL);
    }

  /* Now break CSBUF up at newlines, with backslash allowed to escape a
     newline, and put the individual words into a STRINGLIST. */
  sl = strlist_create (16);
  for (ws = 0; csbuf[ws]; )
    {
      we = ws;
      while (csbuf[we] && csbuf[we] != '\n')
	{
	  if (csbuf[we] == '\\' && csbuf[we+1] == '\n')
	    we++;
	  we++;
	}
      t = substring (csbuf, ws, we);
      if (sl->list_len >= sl->list_size - 1)
	strlist_resize (sl, sl->list_size + 16);
      sl->list[sl->list_len++] = t;
      while (csbuf[we] == '\n') we++;
      ws = we;
    }
  sl->list[sl->list_len] = (char *)NULL;

  free (csbuf);
  return (sl);
}

static WORD_LIST *
command_line_to_word_list (line, llen, sentinel, nwp, cwp)
     char *line;
     int llen, sentinel, *nwp, *cwp;
{
  WORD_LIST *ret;
  char *delims;

#if 0
  delims = "()<>;&| \t\n";	/* shell metacharacters break words */
#else
  delims = rl_completer_word_break_characters;
#endif
  ret = split_at_delims (line, llen, delims, sentinel, SD_NOQUOTEDELIM, nwp, cwp);
  return (ret);
}

/* Evaluate COMPSPEC *cs and return all matches for WORD. */

STRINGLIST *
gen_compspec_completions (cs, cmd, word, start, end, foundp)
     COMPSPEC *cs;
     const char *cmd;
     const char *word;
     int start, end;
     int *foundp;
{
  STRINGLIST *ret, *tmatches;
  char *line;
  int llen, nw, cw, found, foundf;
  WORD_LIST *lwords;
  COMPSPEC *tcs;

  found = 1;

#ifdef DEBUG
  debug_printf ("gen_compspec_completions (%s, %s, %d, %d)", cmd, word, start, end);
  debug_printf ("gen_compspec_completions: %s -> %p", cmd, cs);
#endif
  ret = gen_action_completions (cs, word);
#ifdef DEBUG
  if (ret && progcomp_debug)
    {
      debug_printf ("gen_action_completions (%p, %s) -->", cs, word);
      strlist_print (ret, "\t");
      rl_on_new_line ();
    }
#endif

  /* Now we start generating completions based on the other members of CS. */
  if (cs->globpat)
    {
      tmatches = gen_globpat_matches (cs, word);
      if (tmatches)
	{
#ifdef DEBUG
	  if (progcomp_debug)
	    {
	      debug_printf ("gen_globpat_matches (%p, %s) -->", cs, word);
	      strlist_print (tmatches, "\t");
	      rl_on_new_line ();
	    }
#endif
	  ret = strlist_append (ret, tmatches);
	  strlist_dispose (tmatches);
	  rl_filename_completion_desired = 1;
	}
    }

  if (cs->words)
    {
      tmatches = gen_wordlist_matches (cs, word);
      if (tmatches)
	{
#ifdef DEBUG
	  if (progcomp_debug)
	    {
	      debug_printf ("gen_wordlist_matches (%p, %s) -->", cs, word);
	      strlist_print (tmatches, "\t");
	      rl_on_new_line ();
	    }
#endif
	  ret = strlist_append (ret, tmatches);
	  strlist_dispose (tmatches);
	}
    }

  lwords = (WORD_LIST *)NULL;
  line = (char *)NULL;
  if (cs->command || cs->funcname)
    {
      /* If we have a command or function to execute, we need to first break
	 the command line into individual words, find the number of words,
	 and find the word in the list containing the word to be completed. */
      line = substring (rl_line_buffer, start, end);
      llen = end - start;

#ifdef DEBUG
      debug_printf ("command_line_to_word_list (%s, %d, %d, %p, %p)",
		line, llen, rl_point - start, &nw, &cw);
#endif
      lwords = command_line_to_word_list (line, llen, rl_point - start, &nw, &cw);
#ifdef DEBUG
      if (lwords == 0 && llen > 0)
	debug_printf ("ERROR: command_line_to_word_list returns NULL");
      else if (progcomp_debug)
	{
	  debug_printf ("command_line_to_word_list -->");
	  printf ("\t");
	  print_word_list (lwords, "!");
	  printf ("\n");
	  fflush(stdout);
	  rl_on_new_line ();
	}
#endif
    }

  if (cs->funcname)
    {
      foundf = 0;
      tmatches = gen_shell_function_matches (cs, word, line, rl_point - start, lwords, nw, cw, &foundf);
      if (foundf != 0)
	found = foundf;
      if (tmatches)
	{
#ifdef DEBUG
	  if (progcomp_debug)
	    {
	      debug_printf ("gen_shell_function_matches (%p, %s, %p, %d, %d) -->", cs, word, lwords, nw, cw);
	      strlist_print (tmatches, "\t");
	      rl_on_new_line ();
	    }
#endif
	  ret = strlist_append (ret, tmatches);
	  strlist_dispose (tmatches);
	}
    }

  if (cs->command)
    {
      tmatches = gen_command_matches (cs, word, line, rl_point - start, lwords, nw, cw);
      if (tmatches)
	{
#ifdef DEBUG
	  if (progcomp_debug)
	    {
	      debug_printf ("gen_command_matches (%p, %s, %p, %d, %d) -->", cs, word, lwords, nw, cw);
	      strlist_print (tmatches, "\t");
	      rl_on_new_line ();
	    }
#endif
	  ret = strlist_append (ret, tmatches);
	  strlist_dispose (tmatches);
	}
    }

  if (cs->command || cs->funcname)
    {
      if (lwords)
	dispose_words (lwords);
      FREE (line);
    }

  if (foundp)
    *foundp = found;

  if (found == 0 || (found & PCOMP_RETRYFAIL))
    {
      strlist_dispose (ret);
      return NULL;
    }

  if (cs->filterpat)
    {
      tmatches = filter_stringlist (ret, cs->filterpat, word);
#ifdef DEBUG
      if (progcomp_debug)
	{
	  debug_printf ("filter_stringlist (%p, %s, %s) -->", ret, cs->filterpat, word);
	  strlist_print (tmatches, "\t");
	  rl_on_new_line ();
	}
#endif
      if (ret && ret != tmatches)
	{
	  FREE (ret->list);
	  free (ret);
	}
      ret = tmatches;
    }

  if (cs->prefix || cs->suffix)
    ret = strlist_prefix_suffix (ret, cs->prefix, cs->suffix);

  /* If no matches have been generated and the user has specified that
      directory completion should be done as a default, call
      gen_action_completions again to generate a list of matching directory
      names. */
  if ((ret == 0 || ret->list_len == 0) && (cs->options & COPT_DIRNAMES))
    {
      tcs = compspec_create ();
      tcs->actions = CA_DIRECTORY;
      ret = gen_action_completions (tcs, word);
      compspec_dispose (tcs);
    }
  else if (cs->options & COPT_PLUSDIRS)
    {
      tcs = compspec_create ();
      tcs->actions = CA_DIRECTORY;
      tmatches = gen_action_completions (tcs, word);
      ret = strlist_append (ret, tmatches);
      strlist_dispose (tmatches);
      compspec_dispose (tcs);
    }

  return (ret);
}

void
pcomp_set_readline_variables (flags, nval)
     int flags, nval;
{
  /* If the user specified that the compspec returns filenames, make
     sure that readline knows it. */
  if (flags & COPT_FILENAMES)
    rl_filename_completion_desired = nval;
  /* If the user doesn't want a space appended, tell readline. */
  if (flags & COPT_NOSPACE)
    rl_completion_suppress_append = nval;
}

/* Set or unset FLAGS in the options word of the current compspec.
   SET_OR_UNSET is 1 for setting, 0 for unsetting. */
void
pcomp_set_compspec_options (cs, flags, set_or_unset)
     COMPSPEC *cs;
     int flags, set_or_unset;
{
  if (cs == 0 && ((cs = pcomp_curcs) == 0))
    return;
  if (set_or_unset)
    cs->options |= flags;
  else
    cs->options &= ~flags;
}

static STRINGLIST *
gen_progcomp_completions (ocmd, cmd, word, start, end, foundp, retryp, lastcs)
     const char *ocmd;
     const char *cmd;
     const char *word;
     int start, end;
     int *foundp, *retryp;
     COMPSPEC **lastcs;
{
  COMPSPEC *cs, *oldcs;
  const char *oldcmd;
  STRINGLIST *ret;

  cs = progcomp_search (ocmd);

  if (cs == 0 || cs == *lastcs)
    return (NULL);

  if (*lastcs)
    compspec_dispose (*lastcs);
  cs->refcount++;	/* XXX */
  *lastcs = cs;

  cs = compspec_copy (cs);

  oldcs = pcomp_curcs;
  oldcmd = pcomp_curcmd;

  pcomp_curcs = cs;
  pcomp_curcmd = cmd;

  ret = gen_compspec_completions (cs, cmd, word, start, end, foundp);

  pcomp_curcs = oldcs;
  pcomp_curcmd = oldcmd;

  /* We need to conditionally handle setting *retryp here */
  if (retryp)
    *retryp = foundp && (*foundp & PCOMP_RETRYFAIL);    	

  if (foundp)
    {
      *foundp &= ~PCOMP_RETRYFAIL;
      *foundp |= cs->options;
    }

  compspec_dispose (cs);
  return ret;  
}

/* The driver function for the programmable completion code.  Returns a list
   of matches for WORD, which is an argument to command CMD.  START and END
   bound the command currently being completed in rl_line_buffer. */
char **
programmable_completions (cmd, word, start, end, foundp)
     const char *cmd;
     const char *word;
     int start, end, *foundp;
{
  COMPSPEC *cs, *lastcs;
  STRINGLIST *ret;
  char **rmatches, *t;
  int found, retry, count;

  lastcs = 0;
  found = count = 0;

  do
    {
      retry = 0;

      /* We look at the basename of CMD if the full command does not have
	 an associated COMPSPEC. */
      ret = gen_progcomp_completions (cmd, cmd, word, start, end, &found, &retry, &lastcs);
      if (found == 0)
	{
	  t = strrchr (cmd, '/');
	  if (t && *(++t))
	    ret = gen_progcomp_completions (t, cmd, word, start, end, &found, &retry, &lastcs);
	}

      if (found == 0)
	ret = gen_progcomp_completions (DEFAULTCMD, cmd, word, start, end, &found, &retry, &lastcs);

      count++;

      if (count > 32)
	{
	  internal_warning ("programmable_completion: %s: possible retry loop", cmd);
	  break;
	}
    }
  while (retry);

  if (ret)
    {
      rmatches = ret->list;
      free (ret);
    }
  else
    rmatches = (char **)NULL;

  if (foundp)
    *foundp = found;

  if (lastcs)	/* XXX - should be while? */
    compspec_dispose (lastcs);

  return (rmatches);
}

#endif /* PROGRAMMABLE_COMPLETION */
