/* bashhist.c -- bash interface to the GNU history library. */

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

#include "config.h"

#if defined (HISTORY)

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include "bashtypes.h"
#include <stdio.h>
#include <errno.h>
#include "bashansi.h"
#include "posixstat.h"
#include "filecntl.h"

#include "bashintl.h"

#if defined (SYSLOG_HISTORY)
#  include <syslog.h>
#endif

#include "shell.h"
#include "flags.h"
#include "input.h"
#include "parser.h"	/* for the struct dstack stuff. */
#include "pathexp.h"	/* for the struct ignorevar stuff */
#include "bashhist.h"	/* matching prototypes and declarations */
#include "builtins/common.h"

#include <readline/history.h>
#include <glob/glob.h>
#include <glob/strmatch.h>

#if defined (READLINE)
#  include "bashline.h"
extern int rl_done, rl_dispatching;	/* should really include readline.h */
#endif

#if !defined (errno)
extern int errno;
#endif

static int histignore_item_func __P((struct ign *));
static int check_history_control __P((char *));
static void hc_erasedups __P((char *));
static void really_add_history __P((char *));

static struct ignorevar histignore =
{
  "HISTIGNORE",
  (struct ign *)0,
  0,
  (char *)0,
  (sh_iv_item_func_t *)histignore_item_func,
};

#define HIGN_EXPAND 0x01

/* Declarations of bash history variables. */
/* Non-zero means to remember lines typed to the shell on the history
   list.  This is different than the user-controlled behaviour; this
   becomes zero when we read lines from a file, for example. */
int remember_on_history = 1;
int enable_history_list = 1;	/* value for `set -o history' */

/* The number of lines that Bash has added to this history session.  The
   difference between the number of the top element in the history list
   (offset from history_base) and the number of lines in the history file.
   Appending this session's history to the history file resets this to 0. */
int history_lines_this_session;

/* The number of lines that Bash has read from the history file. */
int history_lines_in_file;

#if defined (BANG_HISTORY)
/* Non-zero means do no history expansion on this line, regardless
   of what history_expansion says. */
int history_expansion_inhibited;
#endif

/* With the old default, every line was saved in the history individually.
   I.e., if the user enters:
	bash$ for i in a b c
	> do
	> echo $i
	> done
   Each line will be individually saved in the history.
	bash$ history
	10  for i in a b c
	11  do
	12  echo $i
	13  done
	14  history
   If the variable command_oriented_history is set, multiple lines
   which form one command will be saved as one history entry.
	bash$ for i in a b c
	> do
	> echo $i
	> done
	bash$ history
	10  for i in a b c
    do
    echo $i
    done
	11  history
   The user can then recall the whole command all at once instead
   of just being able to recall one line at a time.

   This is now enabled by default.
   */
int command_oriented_history = 1;

/* Set to 1 if the first line of a possibly-multi-line command was saved
   in the history list.  Managed by maybe_add_history(), but global so
   the history-manipluating builtins can see it. */
int current_command_first_line_saved = 0;

/* Non-zero means to store newlines in the history list when using
   command_oriented_history rather than trying to use semicolons. */
int literal_history;

/* Non-zero means to append the history to the history file at shell
   exit, even if the history has been stifled. */
int force_append_history;

/* A nit for picking at history saving.  Flags have the following values:

   Value == 0 means save all lines parsed by the shell on the history.
   Value & HC_IGNSPACE means save all lines that do not start with a space.
   Value & HC_IGNDUPS means save all lines that do not match the last
   line saved.
   Value & HC_ERASEDUPS means to remove all other matching lines from the
   history list before saving the latest line. */
int history_control;

/* Set to 1 if the last command was added to the history list successfully
   as a separate history entry; set to 0 if the line was ignored or added
   to a previous entry as part of command-oriented-history processing. */
int hist_last_line_added;

/* Set to 1 if builtins/history.def:push_history added the last history
   entry. */
int hist_last_line_pushed;

#if defined (READLINE)
/* If non-zero, and readline is being used, the user is offered the
   chance to re-edit a failed history expansion. */
int history_reediting;

/* If non-zero, and readline is being used, don't directly execute a
   line with history substitution.  Reload it into the editing buffer
   instead and let the user further edit and confirm with a newline. */
int hist_verify;

#endif /* READLINE */

/* Non-zero means to not save function definitions in the history list. */
int dont_save_function_defs;

/* Variables declared in other files used here. */
extern int current_command_line_count;

extern struct dstack dstack;

static int bash_history_inhibit_expansion __P((char *, int));
#if defined (READLINE)
static void re_edit __P((char *));
#endif
static int history_expansion_p __P((char *));
static int shell_comment __P((char *));
static int should_expand __P((char *));
static HIST_ENTRY *last_history_entry __P((void));
static char *expand_histignore_pattern __P((char *));
static int history_should_ignore __P((char *));

/* Is the history expansion starting at string[i] one that should not
   be expanded? */
static int
bash_history_inhibit_expansion (string, i)
     char *string;
     int i;
{
  /* The shell uses ! as a pattern negation character in globbing [...]
     expressions, so let those pass without expansion. */
  if (i > 0 && (string[i - 1] == '[') && member (']', string + i + 1))
    return (1);
  /* The shell uses ! as the indirect expansion character, so let those
     expansions pass as well. */
  else if (i > 1 && string[i - 1] == '{' && string[i - 2] == '$' &&
	     member ('}', string + i + 1))
    return (1);
#if defined (EXTENDED_GLOB)
  else if (extended_glob && i > 1 && string[i+1] == '(' && member (')', string + i + 2))
    return (1);
#endif
  else
    return (0);
}

void
bash_initialize_history ()
{
  history_quotes_inhibit_expansion = 1;
  history_search_delimiter_chars = ";&()|<>";
  history_inhibit_expansion_function = bash_history_inhibit_expansion;
#if defined (BANG_HISTORY)
  sv_histchars ("histchars");
#endif
}

void
bash_history_reinit (interact)
     int interact;
{
#if defined (BANG_HISTORY)
  history_expansion = interact != 0;
  history_expansion_inhibited = 1;
#endif
  remember_on_history = enable_history_list = interact != 0;
  history_inhibit_expansion_function = bash_history_inhibit_expansion;
}

void
bash_history_disable ()
{
  remember_on_history = 0;
#if defined (BANG_HISTORY)
  history_expansion_inhibited = 1;
#endif
}

void
bash_history_enable ()
{
  remember_on_history = 1;
#if defined (BANG_HISTORY)
  history_expansion_inhibited = 0;
#endif
  history_inhibit_expansion_function = bash_history_inhibit_expansion;
  sv_history_control ("HISTCONTROL");
  sv_histignore ("HISTIGNORE");
}

/* Load the history list from the history file. */
void
load_history ()
{
  char *hf;

  /* Truncate history file for interactive shells which desire it.
     Note that the history file is automatically truncated to the
     size of HISTSIZE if the user does not explicitly set the size
     differently. */
  set_if_not ("HISTSIZE", "500");
  sv_histsize ("HISTSIZE");

  set_if_not ("HISTFILESIZE", get_string_value ("HISTSIZE"));
  sv_histsize ("HISTFILESIZE");

  /* Read the history in HISTFILE into the history list. */
  hf = get_string_value ("HISTFILE");

  if (hf && *hf && file_exists (hf))
    {
      read_history (hf);
      using_history ();
      history_lines_in_file = where_history ();
    }
}

void
bash_clear_history ()
{
  clear_history ();
  history_lines_this_session = 0;
}

/* Delete and free the history list entry at offset I. */
int
bash_delete_histent (i)
     int i;
{
  HIST_ENTRY *discard;

  discard = remove_history (i);
  if (discard)
    free_history_entry (discard);
  history_lines_this_session--;

  return 1;
}

int
bash_delete_last_history ()
{
  register int i;
  HIST_ENTRY **hlist, *histent;
  int r;

  hlist = history_list ();
  if (hlist == NULL)
    return 0;

  for (i = 0; hlist[i]; i++)
    ;
  i--;

  /* History_get () takes a parameter that must be offset by history_base. */
  histent = history_get (history_base + i);	/* Don't free this */
  if (histent == NULL)
    return 0;

  r = bash_delete_histent (i);

  if (where_history () > history_length)
    history_set_pos (history_length);

  return r;
}

#ifdef INCLUDE_UNUSED
/* Write the existing history out to the history file. */
void
save_history ()
{
  char *hf;

  hf = get_string_value ("HISTFILE");
  if (hf && *hf && file_exists (hf))
    {
      /* Append only the lines that occurred this session to
	 the history file. */
      using_history ();

      if (history_lines_this_session < where_history () || force_append_history)
	append_history (history_lines_this_session, hf);
      else
	write_history (hf);
      sv_histsize ("HISTFILESIZE");
    }
}
#endif

int
maybe_append_history (filename)
     char *filename;
{
  int fd, result;
  struct stat buf;

  result = EXECUTION_SUCCESS;
  if (history_lines_this_session && (history_lines_this_session < where_history ()))
    {
      /* If the filename was supplied, then create it if necessary. */
      if (stat (filename, &buf) == -1 && errno == ENOENT)
	{
	  fd = open (filename, O_WRONLY|O_CREAT, 0600);
	  if (fd < 0)
	    {
	      builtin_error (_("%s: cannot create: %s"), filename, strerror (errno));
	      return (EXECUTION_FAILURE);
	    }
	  close (fd);
	}
      result = append_history (history_lines_this_session, filename);
      history_lines_in_file += history_lines_this_session;
      history_lines_this_session = 0;
    }
  return (result);
}

/* If this is an interactive shell, then append the lines executed
   this session to the history file. */
int
maybe_save_shell_history ()
{
  int result;
  char *hf;

  result = 0;
  if (history_lines_this_session)
    {
      hf = get_string_value ("HISTFILE");

      if (hf && *hf)
	{
	  /* If the file doesn't exist, then create it. */
	  if (file_exists (hf) == 0)
	    {
	      int file;
	      file = open (hf, O_CREAT | O_TRUNC | O_WRONLY, 0600);
	      if (file != -1)
		close (file);
	    }

	  /* Now actually append the lines if the history hasn't been
	     stifled.  If the history has been stifled, rewrite the
	     history file. */
	  using_history ();
	  if (history_lines_this_session <= where_history () || force_append_history)
	    {
	      result = append_history (history_lines_this_session, hf);
	      history_lines_in_file += history_lines_this_session;
	    }
	  else
	    {
	      result = write_history (hf);
	      history_lines_in_file = history_lines_this_session;
	    }
	  history_lines_this_session = 0;

	  sv_histsize ("HISTFILESIZE");
	}
    }
  return (result);
}

#if defined (READLINE)
/* Tell readline () that we have some text for it to edit. */
static void
re_edit (text)
     char *text;
{
  if (bash_input.type == st_stdin)
    bash_re_edit (text);
}
#endif /* READLINE */

/* Return 1 if this line needs history expansion. */
static int
history_expansion_p (line)
     char *line;
{
  register char *s;

  for (s = line; *s; s++)
    if (*s == history_expansion_char || *s == history_subst_char)
      return 1;
  return 0;
}

/* Do pre-processing on LINE.  If PRINT_CHANGES is non-zero, then
   print the results of expanding the line if there were any changes.
   If there is an error, return NULL, otherwise the expanded line is
   returned.  If ADDIT is non-zero the line is added to the history
   list after history expansion.  ADDIT is just a suggestion;
   REMEMBER_ON_HISTORY can veto, and does.
   Right now this does history expansion. */
char *
pre_process_line (line, print_changes, addit)
     char *line;
     int print_changes, addit;
{
  char *history_value;
  char *return_value;
  int expanded;

  return_value = line;
  expanded = 0;

#  if defined (BANG_HISTORY)
  /* History expand the line.  If this results in no errors, then
     add that line to the history if ADDIT is non-zero. */
  if (!history_expansion_inhibited && history_expansion && history_expansion_p (line))
    {
      expanded = history_expand (line, &history_value);

      if (expanded)
	{
	  if (print_changes)
	    {
	      if (expanded < 0)
		internal_error ("%s", history_value);
#if defined (READLINE)
	      else if (hist_verify == 0 || expanded == 2)
#else
	      else
#endif
		fprintf (stderr, "%s\n", history_value);
	    }

	  /* If there was an error, return NULL. */
	  if (expanded < 0 || expanded == 2)	/* 2 == print only */
	    {
#    if defined (READLINE)
	      if (expanded == 2 && rl_dispatching == 0 && *history_value)
#    else	      
	      if (expanded == 2 && *history_value)
#    endif /* !READLINE */
		maybe_add_history (history_value);

	      free (history_value);

#    if defined (READLINE)
	      /* New hack.  We can allow the user to edit the
		 failed history expansion. */
	      if (history_reediting && expanded < 0 && rl_done)
		re_edit (line);
#    endif /* READLINE */
	      return ((char *)NULL);
	    }

#    if defined (READLINE)
	  if (hist_verify && expanded == 1)
	    {
	      re_edit (history_value);
	      return ((char *)NULL);
	    }
#    endif
	}

      /* Let other expansions know that return_value can be free'ed,
	 and that a line has been added to the history list.  Note
	 that we only add lines that have something in them. */
      expanded = 1;
      return_value = history_value;
    }
#  endif /* BANG_HISTORY */

  if (addit && remember_on_history && *return_value)
    maybe_add_history (return_value);

#if 0
  if (expanded == 0)
    return_value = savestring (line);
#endif

  return (return_value);
}

/* Return 1 if the first non-whitespace character in LINE is a `#', indicating
 * that the line is a shell comment. */
static int
shell_comment (line)
     char *line;
{
  char *p;

  for (p = line; p && *p && whitespace (*p); p++)
    ;
  return (p && *p == '#');
}

#ifdef INCLUDE_UNUSED
/* Remove shell comments from LINE.  A `#' and anything after it is a comment.
   This isn't really useful yet, since it doesn't handle quoting. */
static char *
filter_comments (line)
     char *line;
{
  char *p;

  for (p = line; p && *p && *p != '#'; p++)
    ;
  if (p && *p == '#')
    *p = '\0';
  return (line);
}
#endif

/* Check LINE against what HISTCONTROL says to do.  Returns 1 if the line
   should be saved; 0 if it should be discarded. */
static int
check_history_control (line)
     char *line;
{
  HIST_ENTRY *temp;
  int r;

  if (history_control == 0)
    return 1;

  /* ignorespace or ignoreboth */
  if ((history_control & HC_IGNSPACE) && *line == ' ')
    return 0;

  /* ignoredups or ignoreboth */
  if (history_control & HC_IGNDUPS)
    {
      using_history ();
      temp = previous_history ();

      r = (temp == 0 || STREQ (temp->line, line) == 0);

      using_history ();

      if (r == 0)
	return r;
    }

  return 1;
}

/* Remove all entries matching LINE from the history list.  Triggered when
   HISTCONTROL includes `erasedups'. */
static void
hc_erasedups (line)
     char *line;
{
  HIST_ENTRY *temp;
  int r;

  using_history ();
  while (temp = previous_history ())
    {
      if (STREQ (temp->line, line))
	{
	  r = where_history ();
	  remove_history (r);
	}
    }
  using_history ();
}

/* Add LINE to the history list, handling possibly multi-line compound
   commands.  We note whether or not we save the first line of each command
   (which is usually the entire command and history entry), and don't add
   the second and subsequent lines of a multi-line compound command if we
   didn't save the first line.  We don't usually save shell comment lines in
   compound commands in the history, because they could have the effect of
   commenting out the rest of the command when the entire command is saved as
   a single history entry (when COMMAND_ORIENTED_HISTORY is enabled).  If
   LITERAL_HISTORY is set, we're saving lines in the history with embedded
   newlines, so it's OK to save comment lines.  We also make sure to save
   multiple-line quoted strings or other constructs. */
void
maybe_add_history (line)
     char *line;
{
  hist_last_line_added = 0;

  /* Don't use the value of history_control to affect the second
     and subsequent lines of a multi-line command (old code did
     this only when command_oriented_history is enabled). */
  if (current_command_line_count > 1)
    {
      if (current_command_first_line_saved &&
	  (literal_history || dstack.delimiter_depth != 0 || shell_comment (line) == 0))
	bash_add_history (line);
      return;
    }

  /* This is the first line of a (possible multi-line) command.  Note whether
     or not we should save the first line and remember it. */
  current_command_first_line_saved = check_add_history (line, 0);
}

/* Just check LINE against HISTCONTROL and HISTIGNORE and add it to the
   history if it's OK.  Used by `history -s' as well as maybe_add_history().
   Returns 1 if the line was saved in the history, 0 otherwise. */
int
check_add_history (line, force)
     char *line;
     int force;
{
  if (check_history_control (line) && history_should_ignore (line) == 0)
    {
      /* We're committed to saving the line.  If the user has requested it,
	 remove other matching lines from the history. */
      if (history_control & HC_ERASEDUPS)
	hc_erasedups (line);
        
      if (force)
	{
	  really_add_history (line);
	  using_history ();
	}
      else
	bash_add_history (line);
      return 1;
    }
  return 0;
}

#if defined (SYSLOG_HISTORY)
#define SYSLOG_MAXLEN 600

void
bash_syslog_history (line)
     const char *line;
{
  char trunc[SYSLOG_MAXLEN];

  if (strlen(line) < SYSLOG_MAXLEN)
    syslog (SYSLOG_FACILITY|SYSLOG_LEVEL, "HISTORY: PID=%d UID=%d %s", getpid(), current_user.uid, line);
  else
    {
      strncpy (trunc, line, SYSLOG_MAXLEN);
      trunc[SYSLOG_MAXLEN - 1] = '\0';
      syslog (SYSLOG_FACILITY|SYSLOG_LEVEL, "HISTORY (TRUNCATED): PID=%d UID=%d %s", getpid(), current_user.uid, trunc);
    }
}
#endif
     	
/* Add a line to the history list.
   The variable COMMAND_ORIENTED_HISTORY controls the style of history
   remembering;  when non-zero, and LINE is not the first line of a
   complete parser construct, append LINE to the last history line instead
   of adding it as a new line. */
void
bash_add_history (line)
     char *line;
{
  int add_it, offset, curlen;
  HIST_ENTRY *current, *old;
  char *chars_to_add, *new_line;

  add_it = 1;
  if (command_oriented_history && current_command_line_count > 1)
    {
      chars_to_add = literal_history ? "\n" : history_delimiting_chars ();

      using_history ();
      current = previous_history ();

      if (current)
	{
	  /* If the previous line ended with an escaped newline (escaped
	     with backslash, but otherwise unquoted), then remove the quoted
	     newline, since that is what happens when the line is parsed. */
	  curlen = strlen (current->line);

	  if (dstack.delimiter_depth == 0 && current->line[curlen - 1] == '\\' &&
	      current->line[curlen - 2] != '\\')
	    {
	      current->line[curlen - 1] = '\0';
	      curlen--;
	      chars_to_add = "";
	    }

	  new_line = (char *)xmalloc (1
				      + curlen
				      + strlen (line)
				      + strlen (chars_to_add));
	  sprintf (new_line, "%s%s%s", current->line, chars_to_add, line);
	  offset = where_history ();
	  old = replace_history_entry (offset, new_line, current->data);
	  free (new_line);

	  if (old)
	    free_history_entry (old);

	  add_it = 0;
	}
    }

  if (add_it)
    really_add_history (line);

#if defined (SYSLOG_HISTORY)
  bash_syslog_history (line);
#endif

  using_history ();
}

static void
really_add_history (line)
     char *line;
{
  hist_last_line_added = 1;
  hist_last_line_pushed = 0;
  add_history (line);
  history_lines_this_session++;
}

int
history_number ()
{
  using_history ();
  return (remember_on_history ? history_base + where_history () : 1);
}

static int
should_expand (s)
     char *s;
{
  char *p;

  for (p = s; p && *p; p++)
    {
      if (*p == '\\')
	p++;
      else if (*p == '&')
	return 1;
    }
  return 0;
}

static int
histignore_item_func (ign)
     struct ign *ign;
{
  if (should_expand (ign->val))
    ign->flags |= HIGN_EXPAND;
  return (0);
}

void
setup_history_ignore (varname)
     char *varname;
{
  setup_ignore_patterns (&histignore);
}

static HIST_ENTRY *
last_history_entry ()
{
  HIST_ENTRY *he;

  using_history ();
  he = previous_history ();
  using_history ();
  return he;
}

char *
last_history_line ()
{
  HIST_ENTRY *he;

  he = last_history_entry ();
  if (he == 0)
    return ((char *)NULL);
  return he->line;
}

static char *
expand_histignore_pattern (pat)
     char *pat;
{
  HIST_ENTRY *phe;
  char *ret;

  phe = last_history_entry ();

  if (phe == (HIST_ENTRY *)0)
    return (savestring (pat));

  ret = strcreplace (pat, '&', phe->line, 1);

  return ret;
}

/* Return 1 if we should not put LINE into the history according to the
   patterns in HISTIGNORE. */
static int
history_should_ignore (line)
     char *line;
{
  register int i, match;
  char *npat;

  if (histignore.num_ignores == 0)
    return 0;

  for (i = match = 0; i < histignore.num_ignores; i++)
    {
      if (histignore.ignores[i].flags & HIGN_EXPAND)
	npat = expand_histignore_pattern (histignore.ignores[i].val);
      else
	npat = histignore.ignores[i].val;

      match = strmatch (npat, line, FNMATCH_EXTFLAG) != FNM_NOMATCH;

      if (histignore.ignores[i].flags & HIGN_EXPAND)
	free (npat);

      if (match)
	break;
    }

  return match;
}
#endif /* HISTORY */
