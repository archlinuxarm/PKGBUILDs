/* bashline.c -- Bash's interface to the readline library. */

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

#if defined (READLINE)

#include "bashtypes.h"
#include "posixstat.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#if defined (HAVE_GRP_H)
#  include <grp.h>
#endif

#if defined (HAVE_NETDB_H)
#  include <netdb.h>
#endif

#include <stdio.h>
#include "chartypes.h"
#include "bashansi.h"
#include "bashintl.h"

#include "shell.h"
#include "input.h"
#include "builtins.h"
#include "bashhist.h"
#include "bashline.h"
#include "execute_cmd.h"
#include "findcmd.h"
#include "pathexp.h"
#include "shmbutil.h"

#include "builtins/common.h"

#include <readline/rlconf.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <glob/glob.h>

#if defined (ALIAS)
#  include "alias.h"
#endif

#if defined (PROGRAMMABLE_COMPLETION)
#  include "pcomplete.h"
#endif

/* These should agree with the defines for emacs_mode and vi_mode in
   rldefs.h, even though that's not a public readline header file. */
#ifndef EMACS_EDITING_MODE
#  define NO_EDITING_MODE	-1
#  define EMACS_EDITING_MODE	 1
#  define VI_EDITING_MODE	 0
#endif

#define RL_BOOLEAN_VARIABLE_VALUE(s)	((s)[0] == 'o' && (s)[1] == 'n' && (s)[2] == '\0')

#if defined (BRACE_COMPLETION)
extern int bash_brace_completion __P((int, int));
#endif /* BRACE_COMPLETION */

/* To avoid including curses.h/term.h/termcap.h and that whole mess. */
extern int tputs __P((const char *string, int nlines, int (*outx)(int)));

/* Forward declarations */

/* Functions bound to keys in Readline for Bash users. */
static int shell_expand_line __P((int, int));
static int display_shell_version __P((int, int));
static int operate_and_get_next __P((int, int));

static int bash_ignore_filenames __P((char **));
static int bash_ignore_everything __P((char **));

#if defined (BANG_HISTORY)
static char *history_expand_line_internal __P((char *));
static int history_expand_line __P((int, int));
static int tcsh_magic_space __P((int, int));
#endif /* BANG_HISTORY */
#ifdef ALIAS
static int alias_expand_line __P((int, int));
#endif
#if defined (BANG_HISTORY) && defined (ALIAS)
static int history_and_alias_expand_line __P((int, int));
#endif

static int bash_forward_shellword __P((int, int));
static int bash_backward_shellword __P((int, int));
static int bash_kill_shellword __P((int, int));
static int bash_backward_kill_shellword __P((int, int));

/* Helper functions for Readline. */
static char *restore_tilde __P((char *, char *));

static char *bash_filename_rewrite_hook __P((char *, int));
static void bash_directory_expansion __P((char **));
static int bash_directory_completion_hook __P((char **));
static int filename_completion_ignore __P((char **));
static int bash_push_line __P((void));

static void cleanup_expansion_error __P((void));
static void maybe_make_readline_line __P((char *));
static void set_up_new_line __P((char *));

static int check_redir __P((int));
static char **attempt_shell_completion __P((const char *, int, int));
static char *variable_completion_function __P((const char *, int));
static char *hostname_completion_function __P((const char *, int));
static char *command_subst_completion_function __P((const char *, int));

static void build_history_completion_array __P((void));
static char *history_completion_generator __P((const char *, int));
static int dynamic_complete_history __P((int, int));
static int bash_dabbrev_expand __P((int, int));

static void initialize_hostname_list __P((void));
static void add_host_name __P((char *));
static void snarf_hosts_from_file __P((char *));
static char **hostnames_matching __P((char *));

static void _ignore_completion_names __P((char **, sh_ignore_func_t *));
static int name_is_acceptable __P((const char *));
static int test_for_directory __P((const char *));
static int return_zero __P((const char *));

static char *bash_dequote_filename __P((char *, int));
static char *quote_word_break_chars __P((char *));
static char *bash_quote_filename __P((char *, int, char *));

static int putx __P((int));
static int bash_execute_unix_command __P((int, int));
static void init_unix_command_map __P((void));
static int isolate_sequence __P((char *, int, int, int *));

static int set_saved_history __P((void));

#if defined (ALIAS)
static int posix_edit_macros __P((int, int));
#endif

#if defined (PROGRAMMABLE_COMPLETION)
static int find_cmd_start __P((int));
static int find_cmd_end __P((int));
static char *find_cmd_name __P((int));
static char *prog_complete_return __P((const char *, int));

static char **prog_complete_matches;
#endif

/* Variables used here but defined in other files. */
#if defined (BANG_HISTORY)
extern int hist_verify;
#endif

extern int current_command_line_count, last_command_exit_value;
extern int array_needs_making;
extern int posixly_correct, no_symbolic_links;
extern char *current_prompt_string, *ps1_prompt;
extern STRING_INT_ALIST word_token_alist[];
extern sh_builtin_func_t *last_shell_builtin, *this_shell_builtin;

/* SPECIFIC_COMPLETION_FUNCTIONS specifies that we have individual
   completion functions which indicate what type of completion should be
   done (at or before point) that can be bound to key sequences with
   the readline library. */
#define SPECIFIC_COMPLETION_FUNCTIONS

#if defined (SPECIFIC_COMPLETION_FUNCTIONS)
static int bash_specific_completion __P((int, rl_compentry_func_t *));

static int bash_complete_filename_internal __P((int));
static int bash_complete_username_internal __P((int));
static int bash_complete_hostname_internal __P((int));
static int bash_complete_variable_internal __P((int));
static int bash_complete_command_internal __P((int));

static int bash_complete_filename __P((int, int));
static int bash_possible_filename_completions __P((int, int));
static int bash_complete_username __P((int, int));
static int bash_possible_username_completions __P((int, int));
static int bash_complete_hostname __P((int, int));
static int bash_possible_hostname_completions __P((int, int));
static int bash_complete_variable __P((int, int));
static int bash_possible_variable_completions __P((int, int));
static int bash_complete_command __P((int, int));
static int bash_possible_command_completions __P((int, int));

static char *glob_complete_word __P((const char *, int));
static int bash_glob_completion_internal __P((int));
static int bash_glob_complete_word __P((int, int));
static int bash_glob_expand_word __P((int, int));
static int bash_glob_list_expansions __P((int, int));

#endif /* SPECIFIC_COMPLETION_FUNCTIONS */

static int edit_and_execute_command __P((int, int, int, char *));
#if defined (VI_MODE)
static int vi_edit_and_execute_command __P((int, int));
static int bash_vi_complete __P((int, int));
#endif
static int emacs_edit_and_execute_command __P((int, int));

/* Non-zero once initalize_readline () has been called. */
int bash_readline_initialized = 0;

/* If non-zero, we do hostname completion, breaking words at `@' and
   trying to complete the stuff after the `@' from our own internal
   host list. */
int perform_hostname_completion = 1;

/* If non-zero, we don't do command completion on an empty line. */
int no_empty_command_completion;

/* Set FORCE_FIGNORE if you want to honor FIGNORE even if it ignores the
   only possible matches.  Set to 0 if you want to match filenames if they
   are the only possible matches, even if FIGNORE says to. */
int force_fignore = 1;

/* Perform spelling correction on directory names during word completion */
int dircomplete_spelling = 0;

static char *bash_completer_word_break_characters = " \t\n\"'@><=;|&(:";
static char *bash_nohostname_word_break_characters = " \t\n\"'><=;|&(:";
/* )) */

static rl_hook_func_t *old_rl_startup_hook = (rl_hook_func_t *)NULL;

static int dot_in_path = 0;

/* Set to non-zero when dabbrev-expand is running */
static int dabbrev_expand_active = 0;

/* What kind of quoting is performed by bash_quote_filename:
	COMPLETE_DQUOTE = double-quoting the filename
	COMPLETE_SQUOTE = single_quoting the filename
	COMPLETE_BSQUOTE = backslash-quoting special chars in the filename
*/
#define COMPLETE_DQUOTE  1
#define COMPLETE_SQUOTE  2
#define COMPLETE_BSQUOTE 3
static int completion_quoting_style = COMPLETE_BSQUOTE;

/* Flag values for the final argument to bash_default_completion */
#define DEFCOMP_CMDPOS		1

/* Change the readline VI-mode keymaps into or out of Posix.2 compliance.
   Called when the shell is put into or out of `posix' mode. */
void
posix_readline_initialize (on_or_off)
     int on_or_off;
{
  if (on_or_off)
    rl_variable_bind ("comment-begin", "#");
#if defined (VI_MODE)
  rl_bind_key_in_map (CTRL ('I'), on_or_off ? rl_insert : rl_complete, vi_insertion_keymap);
#endif
}

void
reset_completer_word_break_chars ()
{
  rl_completer_word_break_characters = perform_hostname_completion ? savestring (bash_completer_word_break_characters) : savestring (bash_nohostname_word_break_characters);
}

/* When this function returns, rl_completer_word_break_characters points to
   dynamically allocated memory. */
int
enable_hostname_completion (on_or_off)
     int on_or_off;
{
  int old_value;
  char *at, *nv, *nval;

  old_value = perform_hostname_completion;

  if (on_or_off)
    {
      perform_hostname_completion = 1;
      rl_special_prefixes = "$@";
    }
  else
    {
      perform_hostname_completion = 0;
      rl_special_prefixes = "$";
    }

  /* Now we need to figure out how to appropriately modify and assign
     rl_completer_word_break_characters depending on whether we want
     hostname completion on or off. */

  /* If this is the first time this has been called
     (bash_readline_initialized == 0), use the sames values as before, but
     allocate new memory for rl_completer_word_break_characters. */

  if (bash_readline_initialized == 0 &&
      (rl_completer_word_break_characters == 0 || 
       rl_completer_word_break_characters == rl_basic_word_break_characters))
    {
      if (on_or_off)
	rl_completer_word_break_characters = savestring (bash_completer_word_break_characters);
      else
	rl_completer_word_break_characters = savestring (bash_nohostname_word_break_characters);
    }
  else
    {
      /* See if we have anything to do. */
      at = strchr (rl_completer_word_break_characters, '@');
      if ((at == 0 && on_or_off == 0) || (at != 0 && on_or_off != 0))
        return old_value;

      /* We have something to do.  Do it. */
      nval = (char *)xmalloc (strlen (rl_completer_word_break_characters) + 1 + on_or_off);

      if (on_or_off == 0)
	{
	  /* Turn it off -- just remove `@' from word break chars.  We want
	     to remove all occurrences of `@' from the char list, so we loop
	     rather than just copy the rest of the list over AT. */
	  for (nv = nval, at = rl_completer_word_break_characters; *at; )
	    if (*at != '@')
	      *nv++ = *at++;
	    else
	      at++;
	  *nv = '\0';
	}
      else
	{
	  nval[0] = '@';
	  strcpy (nval + 1, rl_completer_word_break_characters);
        }

      free (rl_completer_word_break_characters);
      rl_completer_word_break_characters = nval;
    }

  return (old_value);
}

/* Called once from parse.y if we are going to use readline. */
void
initialize_readline ()
{
  rl_command_func_t *func;
  char kseq[2];

  if (bash_readline_initialized)
    return;

  rl_terminal_name = get_string_value ("TERM");
  rl_instream = stdin;
  rl_outstream = stderr;

  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = "Bash";

  /* Add bindable names before calling rl_initialize so they may be
     referenced in the various inputrc files. */
  rl_add_defun ("shell-expand-line", shell_expand_line, -1);
#ifdef BANG_HISTORY
  rl_add_defun ("history-expand-line", history_expand_line, -1);
  rl_add_defun ("magic-space", tcsh_magic_space, -1);
#endif

  rl_add_defun ("shell-forward-word", bash_forward_shellword, -1);
  rl_add_defun ("shell-backward-word", bash_backward_shellword, -1);
  rl_add_defun ("shell-kill-word", bash_kill_shellword, -1);
  rl_add_defun ("shell-backward-kill-word", bash_backward_kill_shellword, -1);

#ifdef ALIAS
  rl_add_defun ("alias-expand-line", alias_expand_line, -1);
#  ifdef BANG_HISTORY
  rl_add_defun ("history-and-alias-expand-line", history_and_alias_expand_line, -1);
#  endif
#endif

  /* Backwards compatibility. */
  rl_add_defun ("insert-last-argument", rl_yank_last_arg, -1);

  rl_add_defun ("operate-and-get-next", operate_and_get_next, -1);
  rl_add_defun ("display-shell-version", display_shell_version, -1);
  rl_add_defun ("edit-and-execute-command", emacs_edit_and_execute_command, -1);

#if defined (BRACE_COMPLETION)
  rl_add_defun ("complete-into-braces", bash_brace_completion, -1);
#endif

#if defined (SPECIFIC_COMPLETION_FUNCTIONS)
  rl_add_defun ("complete-filename", bash_complete_filename, -1);
  rl_add_defun ("possible-filename-completions", bash_possible_filename_completions, -1);
  rl_add_defun ("complete-username", bash_complete_username, -1);
  rl_add_defun ("possible-username-completions", bash_possible_username_completions, -1);
  rl_add_defun ("complete-hostname", bash_complete_hostname, -1);
  rl_add_defun ("possible-hostname-completions", bash_possible_hostname_completions, -1);
  rl_add_defun ("complete-variable", bash_complete_variable, -1);
  rl_add_defun ("possible-variable-completions", bash_possible_variable_completions, -1);
  rl_add_defun ("complete-command", bash_complete_command, -1);
  rl_add_defun ("possible-command-completions", bash_possible_command_completions, -1);
  rl_add_defun ("glob-complete-word", bash_glob_complete_word, -1);
  rl_add_defun ("glob-expand-word", bash_glob_expand_word, -1);
  rl_add_defun ("glob-list-expansions", bash_glob_list_expansions, -1);
#endif

  rl_add_defun ("dynamic-complete-history", dynamic_complete_history, -1);
  rl_add_defun ("dabbrev-expand", bash_dabbrev_expand, -1);

  /* Bind defaults before binding our custom shell keybindings. */
  if (RL_ISSTATE(RL_STATE_INITIALIZED) == 0)
    rl_initialize ();

  /* Bind up our special shell functions. */
  rl_bind_key_if_unbound_in_map (CTRL('E'), shell_expand_line, emacs_meta_keymap);

#ifdef BANG_HISTORY
  rl_bind_key_if_unbound_in_map ('^', history_expand_line, emacs_meta_keymap);
#endif

  rl_bind_key_if_unbound_in_map (CTRL ('O'), operate_and_get_next, emacs_standard_keymap);
  rl_bind_key_if_unbound_in_map (CTRL ('V'), display_shell_version, emacs_ctlx_keymap);

  /* In Bash, the user can switch editing modes with "set -o [vi emacs]",
     so it is not necessary to allow C-M-j for context switching.  Turn
     off this occasionally confusing behaviour. */
  kseq[0] = CTRL('J');
  kseq[1] = '\0';
  func = rl_function_of_keyseq (kseq, emacs_meta_keymap, (int *)NULL);
  if (func == rl_vi_editing_mode)
    rl_unbind_key_in_map (CTRL('J'), emacs_meta_keymap);
  kseq[0] = CTRL('M');
  func = rl_function_of_keyseq (kseq, emacs_meta_keymap, (int *)NULL);
  if (func == rl_vi_editing_mode)
    rl_unbind_key_in_map (CTRL('M'), emacs_meta_keymap);
#if defined (VI_MODE)
  rl_unbind_key_in_map (CTRL('E'), vi_movement_keymap);
#endif

#if defined (BRACE_COMPLETION)
  rl_bind_key_if_unbound_in_map ('{', bash_brace_completion, emacs_meta_keymap); /*}*/
#endif /* BRACE_COMPLETION */

#if defined (SPECIFIC_COMPLETION_FUNCTIONS)
  rl_bind_key_if_unbound_in_map ('/', bash_complete_filename, emacs_meta_keymap);
  rl_bind_key_if_unbound_in_map ('/', bash_possible_filename_completions, emacs_ctlx_keymap);

  /* Have to jump through hoops here because there is a default binding for
     M-~ (rl_tilde_expand) */
  kseq[0] = '~';
  kseq[1] = '\0';
  func = rl_function_of_keyseq (kseq, emacs_meta_keymap, (int *)NULL);
  if (func == 0 || func == rl_tilde_expand)
    rl_bind_keyseq_in_map (kseq, bash_complete_username, emacs_meta_keymap);

  rl_bind_key_if_unbound_in_map ('~', bash_possible_username_completions, emacs_ctlx_keymap);

  rl_bind_key_if_unbound_in_map ('@', bash_complete_hostname, emacs_meta_keymap);
  rl_bind_key_if_unbound_in_map ('@', bash_possible_hostname_completions, emacs_ctlx_keymap);

  rl_bind_key_if_unbound_in_map ('$', bash_complete_variable, emacs_meta_keymap);
  rl_bind_key_if_unbound_in_map ('$', bash_possible_variable_completions, emacs_ctlx_keymap);

  rl_bind_key_if_unbound_in_map ('!', bash_complete_command, emacs_meta_keymap);
  rl_bind_key_if_unbound_in_map ('!', bash_possible_command_completions, emacs_ctlx_keymap);

  rl_bind_key_if_unbound_in_map ('g', bash_glob_complete_word, emacs_meta_keymap);
  rl_bind_key_if_unbound_in_map ('*', bash_glob_expand_word, emacs_ctlx_keymap);
  rl_bind_key_if_unbound_in_map ('g', bash_glob_list_expansions, emacs_ctlx_keymap);

#endif /* SPECIFIC_COMPLETION_FUNCTIONS */

  kseq[0] = TAB;
  kseq[1] = '\0';
  func = rl_function_of_keyseq (kseq, emacs_meta_keymap, (int *)NULL);
  if (func == 0 || func == rl_tab_insert)
    rl_bind_key_in_map (TAB, dynamic_complete_history, emacs_meta_keymap);

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = attempt_shell_completion;

  /* Tell the completer that we might want to follow symbolic links or
     do other expansion on directory names. */
  rl_directory_completion_hook = bash_directory_completion_hook;

  rl_filename_rewrite_hook = bash_filename_rewrite_hook;

  /* Tell the filename completer we want a chance to ignore some names. */
  rl_ignore_some_completions_function = filename_completion_ignore;

  /* Bind C-xC-e to invoke emacs and run result as commands. */
  rl_bind_key_if_unbound_in_map (CTRL ('E'), emacs_edit_and_execute_command, emacs_ctlx_keymap);
#if defined (VI_MODE)
  rl_bind_key_if_unbound_in_map ('v', vi_edit_and_execute_command, vi_movement_keymap);
#  if defined (ALIAS)
  rl_bind_key_if_unbound_in_map ('@', posix_edit_macros, vi_movement_keymap);
#  endif

  rl_bind_key_in_map ('\\', bash_vi_complete, vi_movement_keymap);
  rl_bind_key_in_map ('*', bash_vi_complete, vi_movement_keymap);
  rl_bind_key_in_map ('=', bash_vi_complete, vi_movement_keymap);
#endif

  rl_completer_quote_characters = "'\"";

  /* This sets rl_completer_word_break_characters and rl_special_prefixes
     to the appropriate values, depending on whether or not hostname
     completion is enabled. */
  enable_hostname_completion (perform_hostname_completion);

  /* characters that need to be quoted when appearing in filenames. */
#if 0
  rl_filename_quote_characters = " \t\n\\\"'@<>=;|&()#$`?*[!:{";	/*}*/
#else
  rl_filename_quote_characters = " \t\n\\\"'@<>=;|&()#$`?*[!:{~";	/*}*/
#endif
  rl_filename_quoting_function = bash_quote_filename;
  rl_filename_dequoting_function = bash_dequote_filename;
  rl_char_is_quoted_p = char_is_quoted;

#if 0
  /* This is superfluous and makes it impossible to use tab completion in
     vi mode even when explicitly binding it in ~/.inputrc.  sv_strict_posix()
     should already have called posix_readline_initialize() when
     posixly_correct was set. */
  if (posixly_correct)
    posix_readline_initialize (1);
#endif

  bash_readline_initialized = 1;
}

void
bashline_reinitialize ()
{
  bash_readline_initialized = 0;
}

/* On Sun systems at least, rl_attempted_completion_function can end up
   getting set to NULL, and rl_completion_entry_function set to do command
   word completion if Bash is interrupted while trying to complete a command
   word.  This just resets all the completion functions to the right thing.
   It's called from throw_to_top_level(). */
void
bashline_reset ()
{
  tilde_initialize ();
  rl_attempted_completion_function = attempt_shell_completion;
  rl_completion_entry_function = NULL;
  rl_directory_completion_hook = bash_directory_completion_hook;
  rl_ignore_some_completions_function = filename_completion_ignore;
}

/* Contains the line to push into readline. */
static char *push_to_readline = (char *)NULL;

/* Push the contents of push_to_readline into the
   readline buffer. */
static int
bash_push_line ()
{
  if (push_to_readline)
    {
      rl_insert_text (push_to_readline);
      free (push_to_readline);
      push_to_readline = (char *)NULL;
      rl_startup_hook = old_rl_startup_hook;
    }
  return 0;
}

/* Call this to set the initial text for the next line to read
   from readline. */
int
bash_re_edit (line)
     char *line;
{
  FREE (push_to_readline);

  push_to_readline = savestring (line);
  old_rl_startup_hook = rl_startup_hook;
  rl_startup_hook = bash_push_line;

  return (0);
}

static int
display_shell_version (count, c)
     int count, c;
{
  rl_crlf ();
  show_shell_version (0);
  putc ('\r', rl_outstream);
  fflush (rl_outstream);
  rl_on_new_line ();
  rl_redisplay ();
  return 0;
}

/* **************************************************************** */
/*								    */
/*			     Readline Stuff			    */
/*								    */
/* **************************************************************** */

/* If the user requests hostname completion, then simply build a list
   of hosts, and complete from that forever more, or at least until
   HOSTFILE is unset. */

/* THIS SHOULD BE A STRINGLIST. */
/* The kept list of hostnames. */
static char **hostname_list = (char **)NULL;

/* The physical size of the above list. */
static int hostname_list_size;

/* The number of hostnames in the above list. */
static int hostname_list_length;

/* Whether or not HOSTNAME_LIST has been initialized. */
int hostname_list_initialized = 0;

/* Initialize the hostname completion table. */
static void
initialize_hostname_list ()
{
  char *temp;

  temp = get_string_value ("HOSTFILE");
  if (temp == 0)
    temp = get_string_value ("hostname_completion_file");
  if (temp == 0)
    temp = DEFAULT_HOSTS_FILE;

  snarf_hosts_from_file (temp);

  if (hostname_list)
    hostname_list_initialized++;
}

/* Add NAME to the list of hosts. */
static void
add_host_name (name)
     char *name;
{
  if (hostname_list_length + 2 > hostname_list_size)
    {
      hostname_list_size = (hostname_list_size + 32) - (hostname_list_size % 32);
      hostname_list = strvec_resize (hostname_list, hostname_list_size);
    }

  hostname_list[hostname_list_length++] = savestring (name);
  hostname_list[hostname_list_length] = (char *)NULL;
}

#define cr_whitespace(c) ((c) == '\r' || (c) == '\n' || whitespace(c))

static void
snarf_hosts_from_file (filename)
     char *filename;
{
  FILE *file;
  char *temp, buffer[256], name[256];
  register int i, start;

  file = fopen (filename, "r");
  if (file == 0)
    return;

  while (temp = fgets (buffer, 255, file))
    {
      /* Skip to first character. */
      for (i = 0; buffer[i] && cr_whitespace (buffer[i]); i++)
	;

      /* If comment or blank line, ignore. */
      if (buffer[i] == '\0' || buffer[i] == '#')
	continue;

      /* If `preprocessor' directive, do the include. */
      if (strncmp (buffer + i, "$include ", 9) == 0)
	{
	  char *incfile, *t;

	  /* Find start of filename. */
	  for (incfile = buffer + i + 9; *incfile && whitespace (*incfile); incfile++)
	    ;

	  /* Find end of filename. */
	  for (t = incfile; *t && cr_whitespace (*t) == 0; t++)
	    ;

	  *t = '\0';

	  snarf_hosts_from_file (incfile);
	  continue;
	}

      /* Skip internet address if present. */
      if (DIGIT (buffer[i]))
	for (; buffer[i] && cr_whitespace (buffer[i]) == 0; i++);

      /* Gobble up names.  Each name is separated with whitespace. */
      while (buffer[i])
	{
	  for (; cr_whitespace (buffer[i]); i++)
	    ;
	  if (buffer[i] == '\0' || buffer[i] ==  '#')
	    break;

	  /* Isolate the current word. */
	  for (start = i; buffer[i] && cr_whitespace (buffer[i]) == 0; i++)
	    ;
	  if (i == start)
	    continue;
	  strncpy (name, buffer + start, i - start);
	  name[i - start] = '\0';
	  add_host_name (name);
	}
    }
  fclose (file);
}

/* Return the hostname list. */
char **
get_hostname_list ()
{
  if (hostname_list_initialized == 0)
    initialize_hostname_list ();
  return (hostname_list);
}

void
clear_hostname_list ()
{
  register int i;

  if (hostname_list_initialized == 0)
    return;
  for (i = 0; i < hostname_list_length; i++)
    free (hostname_list[i]);
  hostname_list_length = hostname_list_initialized = 0;
}

/* Return a NULL terminated list of hostnames which begin with TEXT.
   Initialize the hostname list the first time if neccessary.
   The array is malloc ()'ed, but not the individual strings. */
static char **
hostnames_matching (text)
     char *text;
{
  register int i, len, nmatch, rsize;
  char **result;

  if (hostname_list_initialized == 0)
    initialize_hostname_list ();

  if (hostname_list_initialized == 0)
    return ((char **)NULL);

  /* Special case.  If TEXT consists of nothing, then the whole list is
     what is desired. */
  if (*text == '\0')
    {
      result = strvec_create (1 + hostname_list_length);
      for (i = 0; i < hostname_list_length; i++)
	result[i] = hostname_list[i];
      result[i] = (char *)NULL;
      return (result);
    }

  /* Scan until found, or failure. */
  len = strlen (text);
  result = (char **)NULL;
  for (i = nmatch = rsize = 0; i < hostname_list_length; i++)
    {
      if (STREQN (text, hostname_list[i], len) == 0)
	continue;

      /* OK, it matches.  Add it to the list. */
      if (nmatch >= (rsize - 1))
	{
	  rsize = (rsize + 16) - (rsize % 16);
	  result = strvec_resize (result, rsize);
	}

      result[nmatch++] = hostname_list[i];
    }
  if (nmatch)
    result[nmatch] = (char *)NULL;
  return (result);
}

/* The equivalent of the Korn shell C-o operate-and-get-next-history-line
   editing command. */
static int saved_history_line_to_use = -1;

static int
set_saved_history ()
{
  if (saved_history_line_to_use >= 0)
    rl_get_previous_history (history_length - saved_history_line_to_use, 0);
  saved_history_line_to_use = -1;
  rl_startup_hook = old_rl_startup_hook;
  return (0);
}

static int
operate_and_get_next (count, c)
     int count, c;
{
  int where;

  /* Accept the current line. */
  rl_newline (1, c);

  /* Find the current line, and find the next line to use. */
  where = where_history ();

  if ((history_is_stifled () && (history_length >= history_max_entries)) ||
      (where >= history_length - 1))
    saved_history_line_to_use = where;
  else
    saved_history_line_to_use = where + 1;

  old_rl_startup_hook = rl_startup_hook;
  rl_startup_hook = set_saved_history;

  return 0;
}

/* This vi mode command causes VI_EDIT_COMMAND to be run on the current
   command being entered (if no explicit argument is given), otherwise on
   a command from the history file. */

#define VI_EDIT_COMMAND		"fc -e \"${VISUAL:-${EDITOR:-vi}}\""
#define EMACS_EDIT_COMMAND	"fc -e \"${VISUAL:-${EDITOR:-emacs}}\""
#define POSIX_VI_EDIT_COMMAND	"fc -e vi"

static int
edit_and_execute_command (count, c, editing_mode, edit_command)
     int count, c, editing_mode;
     char *edit_command;
{
  char *command, *metaval;
  int r, cclc, rrs, metaflag;

  rrs = rl_readline_state;
  cclc = current_command_line_count;

  /* Accept the current line. */
  rl_newline (1, c);

  if (rl_explicit_arg)
    {
      command = (char *)xmalloc (strlen (edit_command) + 8);
      sprintf (command, "%s %d", edit_command, count);
    }
  else
    {
      /* Take the command we were just editing, add it to the history file,
	 then call fc to operate on it.  We have to add a dummy command to
	 the end of the history because fc ignores the last command (assumes
	 it's supposed to deal with the command before the `fc'). */
      using_history ();
      bash_add_history (rl_line_buffer);
      bash_add_history ("");
      history_lines_this_session++;
      using_history ();
      command = savestring (edit_command);
    }

  metaval = rl_variable_value ("input-meta");
  metaflag = RL_BOOLEAN_VARIABLE_VALUE (metaval);
  
  /* Now, POSIX.1-2001 and SUSv3 say that the commands executed from the
     temporary file should be placed into the history.  We don't do that
     yet. */
  if (rl_deprep_term_function)
    (*rl_deprep_term_function) ();
  r = parse_and_execute (command, (editing_mode == VI_EDITING_MODE) ? "v" : "C-xC-e", SEVAL_NOHIST);
  if (rl_prep_term_function)
    (*rl_prep_term_function) (metaflag);

  current_command_line_count = cclc;

  /* Now erase the contents of the current line and undo the effects of the
     rl_accept_line() above.  We don't even want to make the text we just
     executed available for undoing. */
  rl_line_buffer[0] = '\0';	/* XXX */
  rl_point = rl_end = 0;
  rl_done = 0;
  rl_readline_state = rrs;

  rl_forced_update_display ();

  return r;
}

#if defined (VI_MODE)
static int
vi_edit_and_execute_command (count, c)
     int count, c;
{
  if (posixly_correct)
    return (edit_and_execute_command (count, c, VI_EDITING_MODE, POSIX_VI_EDIT_COMMAND));
  else
    return (edit_and_execute_command (count, c, VI_EDITING_MODE, VI_EDIT_COMMAND));
}
#endif /* VI_MODE */

static int
emacs_edit_and_execute_command (count, c)
     int count, c;
{
  return (edit_and_execute_command (count, c, EMACS_EDITING_MODE, EMACS_EDIT_COMMAND));
}

#if defined (ALIAS)
static int
posix_edit_macros (count, key)
     int count, key;
{
  int c;
  char alias_name[3], *alias_value, *macro;

  c = rl_read_key ();
  alias_name[0] = '_';
  alias_name[1] = c;
  alias_name[2] = '\0';

  alias_value = get_alias_value (alias_name);
  if (alias_value && *alias_value)
    {
      macro = savestring (alias_value);
      rl_push_macro_input (macro);
    }
  return 0;
}
#endif

/* Bindable commands that move `shell-words': that is, sequences of
   non-unquoted-metacharacters. */

#define WORDDELIM(c)	(shellmeta(c) || shellblank(c))

static int
bash_forward_shellword (count, key)
     int count, key;
{
  size_t slen;
  int sindex, c, p;
  DECLARE_MBSTATE;

  if (count < 0)
    return (bash_backward_shellword (-count, key));

  /* The tricky part of this is deciding whether or not the first character
     we're on is an unquoted metacharacter.  Not completely handled yet. */
  /* XXX - need to test this stuff with backslash-escaped shell
     metacharacters and unclosed single- and double-quoted strings. */

  p = rl_point;
  slen = rl_end;

  while (count)
    {
      if (p == rl_end)
	{
	  rl_point = rl_end;
	  return 0;
	}

      /* Move forward until we hit a non-metacharacter. */
      while (p < rl_end && (c = rl_line_buffer[p]) && WORDDELIM (c))
	{
	  switch (c)
	    {
	    default:
	      ADVANCE_CHAR (rl_line_buffer, slen, p);
	      continue;		/* straight back to loop, don't increment p */
	    case '\\':
	      if (p < rl_end && rl_line_buffer[p])
		ADVANCE_CHAR (rl_line_buffer, slen, p);
	      break;
	    case '\'':
	      p = skip_to_delim (rl_line_buffer, ++p, "'", SD_NOJMP);
	      break;
	    case '"':
	      p = skip_to_delim (rl_line_buffer, ++p, "\"", SD_NOJMP);
	      break;
	    }

	  if (p < rl_end)
	    p++;
	}

      if (rl_line_buffer[p] == 0 || p == rl_end)
        {
	  rl_point = rl_end;
	  rl_ding ();
	  return 0;
        }
	
      /* Now move forward until we hit a non-quoted metacharacter or EOL */
      while (p < rl_end && (c = rl_line_buffer[p]) && WORDDELIM (c) == 0)
	{
	  switch (c)
	    {
	    default:
	      ADVANCE_CHAR (rl_line_buffer, slen, p);
	      continue;		/* straight back to loop, don't increment p */
	    case '\\':
	      if (p < rl_end && rl_line_buffer[p])
		ADVANCE_CHAR (rl_line_buffer, slen, p);
	      break;
	    case '\'':
	      p = skip_to_delim (rl_line_buffer, ++p, "'", SD_NOJMP);
	      break;
	    case '"':
	      p = skip_to_delim (rl_line_buffer, ++p, "\"", SD_NOJMP);
	      break;
	    }

	  if (p < rl_end)
	    p++;
	}

      if (p == rl_end || rl_line_buffer[p] == 0)
	{
	  rl_point = rl_end;
	  return (0);
	}

      count--;      
    }

  rl_point = p;
  return (0);
}

static int
bash_backward_shellword (count, key)
     int count, key;
{
  size_t slen;
  int sindex, c, p;
  DECLARE_MBSTATE;
  
  if (count < 0)
    return (bash_forward_shellword (-count, key));

  p = rl_point;
  slen = rl_end;
  
  while (count)
    {
      if (p == 0)
	{
	  rl_point = 0;
	  return 0;
	}

      /* Move backward until we hit a non-metacharacter. */
      while (p > 0)
	{
	  c = rl_line_buffer[p];
	  if (WORDDELIM (c) && char_is_quoted (rl_line_buffer, p) == 0)
	    BACKUP_CHAR (rl_line_buffer, slen, p);
	  break;
	}

      if (p == 0)
	{
	  rl_point = 0;
	  return 0;
	}

      /* Now move backward until we hit a metacharacter or BOL. */
      while (p > 0)
	{
	  c = rl_line_buffer[p];
	  if (WORDDELIM (c) && char_is_quoted (rl_line_buffer, p) == 0)
	    break;
	  BACKUP_CHAR (rl_line_buffer, slen, p);
	}

      count--;
    }

  rl_point = p;
  return 0;
}

static int
bash_kill_shellword (count, key)
     int count, key;
{
  int p;

  if (count < 0)
    return (bash_backward_kill_shellword (-count, key));

  p = rl_point;
  bash_forward_shellword (count, key);

  if (rl_point != p)
    rl_kill_text (p, rl_point);

  rl_point = p;
  if (rl_editing_mode == 1)	/* 1 == emacs_mode */
    rl_mark = rl_point;

  return 0;
}

static int
bash_backward_kill_shellword (count, key)
     int count, key;
{
  int p;

  if (count < 0)
    return (bash_kill_shellword (-count, key));

  p = rl_point;
  bash_backward_shellword (count, key);

  if (rl_point != p)
    rl_kill_text (p, rl_point);

  if (rl_editing_mode == 1)	/* 1 == emacs_mode */
    rl_mark = rl_point;

  return 0;
}


/* **************************************************************** */
/*								    */
/*			How To Do Shell Completion		    */
/*								    */
/* **************************************************************** */

#define COMMAND_SEPARATORS ";|&{(`"
/* )} */ 

static int
check_redir (ti)
     int ti;
{
  register int this_char, prev_char;

  /* Handle the two character tokens `>&', `<&', and `>|'.
     We are not in a command position after one of these. */
  this_char = rl_line_buffer[ti];
  prev_char = rl_line_buffer[ti - 1];

  if ((this_char == '&' && (prev_char == '<' || prev_char == '>')) ||
      (this_char == '|' && prev_char == '>'))
    return (1);
  else if ((this_char == '{' && prev_char == '$') || /* } */
	   (char_is_quoted (rl_line_buffer, ti)))
    return (1);
  return (0);
}

#if defined (PROGRAMMABLE_COMPLETION)
/*
 * XXX - because of the <= start test, and setting os = s+1, this can
 * potentially return os > start.  This is probably not what we want to
 * happen, but fix later after 2.05a-release.
 */
static int
find_cmd_start (start)
     int start;
{
  register int s, os;

  os = 0;
  while (((s = skip_to_delim (rl_line_buffer, os, COMMAND_SEPARATORS, SD_NOJMP|SD_NOSKIPCMD)) <= start) &&
	 rl_line_buffer[s])
    os = s+1;
  return os;
}

static int
find_cmd_end (end)
     int end;
{
  register int e;

  e = skip_to_delim (rl_line_buffer, end, COMMAND_SEPARATORS, SD_NOJMP);
  return e;
}

static char *
find_cmd_name (start)
     int start;
{
  char *name;
  register int s, e;

  for (s = start; whitespace (rl_line_buffer[s]); s++)
    ;

  /* skip until a shell break character */
  e = skip_to_delim (rl_line_buffer, s, "()<>;&| \t\n", SD_NOJMP);

  name = substring (rl_line_buffer, s, e);

  return (name);
}

static char *
prog_complete_return (text, matchnum)
     const char *text;
     int matchnum;
{
  static int ind;

  if (matchnum == 0)
    ind = 0;

  if (prog_complete_matches == 0 || prog_complete_matches[ind] == 0)
    return (char *)NULL;
  return (prog_complete_matches[ind++]);
}

#endif /* PROGRAMMABLE_COMPLETION */

/* Do some completion on TEXT.  The indices of TEXT in RL_LINE_BUFFER are
   at START and END.  Return an array of matches, or NULL if none. */
static char **
attempt_shell_completion (text, start, end)
     const char *text;
     int start, end;
{
  int in_command_position, ti, saveti, qc, dflags;
  char **matches, *command_separator_chars;

  command_separator_chars = COMMAND_SEPARATORS;
  matches = (char **)NULL;
  rl_ignore_some_completions_function = filename_completion_ignore;

  /* Determine if this could be a command word.  It is if it appears at
     the start of the line (ignoring preceding whitespace), or if it
     appears after a character that separates commands.  It cannot be a
     command word if we aren't at the top-level prompt. */
  ti = start - 1;
  saveti = qc = -1;

  while ((ti > -1) && (whitespace (rl_line_buffer[ti])))
    ti--;

#if 1
  /* If this is an open quote, maybe we're trying to complete a quoted
     command name. */
  if (ti >= 0 && (rl_line_buffer[ti] == '"' || rl_line_buffer[ti] == '\''))
    {
      qc = rl_line_buffer[ti];
      saveti = ti--;
      while (ti > -1 && (whitespace (rl_line_buffer[ti])))
	ti--;
    }
#endif
      
  in_command_position = 0;
  if (ti < 0)
    {
      /* Only do command completion at the start of a line when we
	 are prompting at the top level. */
      if (current_prompt_string == ps1_prompt)
	in_command_position++;
    }
  else if (member (rl_line_buffer[ti], command_separator_chars))
    {
      in_command_position++;

      if (check_redir (ti) == 1)
	in_command_position = 0;
    }
  else
    {
      /* This still could be in command position.  It is possible
	 that all of the previous words on the line are variable
	 assignments. */
    }

  /* Check that we haven't incorrectly flagged a closed command substitution
     as indicating we're in a command position. */
  if (in_command_position && ti >= 0 && rl_line_buffer[ti] == '`' &&
	*text != '`' && unclosed_pair (rl_line_buffer, end, "`") == 0)
    in_command_position = 0;

  /* Special handling for command substitution.  If *TEXT is a backquote,
     it can be the start or end of an old-style command substitution, or
     unmatched.  If it's unmatched, both calls to unclosed_pair will
     succeed.  Don't bother if readline found a single quote and we are
     completing on the substring.  */
  if (*text == '`' && rl_completion_quote_character != '\'' &&
	(in_command_position || (unclosed_pair (rl_line_buffer, start, "`") &&
				 unclosed_pair (rl_line_buffer, end, "`"))))
    matches = rl_completion_matches (text, command_subst_completion_function);

#if defined (PROGRAMMABLE_COMPLETION)
  /* Attempt programmable completion. */
  if (matches == 0 && (in_command_position == 0 || text[0] == '\0') &&
      prog_completion_enabled && (progcomp_size () > 0) &&
      current_prompt_string == ps1_prompt)
    {
      int s, e, foundcs;
      char *n;

      /* XXX - don't free the members */
      if (prog_complete_matches)
	free (prog_complete_matches);
      prog_complete_matches = (char **)NULL;

      s = find_cmd_start (start);
      e = find_cmd_end (end);
      n = find_cmd_name (s);
      if (e == 0 && e == s && text[0] == '\0')
        prog_complete_matches = programmable_completions ("_EmptycmD_", text, s, e, &foundcs);
      else if (e > s && assignment (n, 0) == 0)
	prog_complete_matches = programmable_completions (n, text, s, e, &foundcs);
      else
	foundcs = 0;
      FREE (n);
      /* XXX - if we found a COMPSPEC for the command, just return whatever
	 the programmable completion code returns, and disable the default
	 filename completion that readline will do unless the COPT_DEFAULT
	 option has been set with the `-o default' option to complete or
	 compopt. */
      if (foundcs)
	{
	  pcomp_set_readline_variables (foundcs, 1);
	  /* Turn what the programmable completion code returns into what
	     readline wants.  I should have made compute_lcd_of_matches
	     external... */
	  matches = rl_completion_matches (text, prog_complete_return);
	  if ((foundcs & COPT_DEFAULT) == 0)
	    rl_attempted_completion_over = 1;	/* no default */
	  if (matches || ((foundcs & COPT_BASHDEFAULT) == 0))
	    return (matches);
	}
    }
#endif

  if (matches == 0)
    {
      dflags = 0;
      if (in_command_position)
	dflags |= DEFCOMP_CMDPOS;
      matches = bash_default_completion (text, start, end, qc, dflags);
    }

  return matches;
}

char **
bash_default_completion (text, start, end, qc, compflags)
     const char *text;
     int start, end, qc, compflags;
{
  char **matches;

  matches = (char **)NULL;

  /* New posix-style command substitution or variable name? */
  if (!matches && *text == '$')
    {
      if (qc != '\'' && text[1] == '(') /* ) */
	matches = rl_completion_matches (text, command_subst_completion_function);
      else
	matches = rl_completion_matches (text, variable_completion_function);
    }

  /* If the word starts in `~', and there is no slash in the word, then
     try completing this word as a username. */
  if (matches ==0 && *text == '~' && mbschr (text, '/') == 0)
    matches = rl_completion_matches (text, rl_username_completion_function);

  /* Another one.  Why not?  If the word starts in '@', then look through
     the world of known hostnames for completion first. */
  if (!matches && perform_hostname_completion && *text == '@')
    matches = rl_completion_matches (text, hostname_completion_function);

  /* And last, (but not least) if this word is in a command position, then
     complete over possible command names, including aliases, functions,
     and command names. */
  if (matches == 0 && (compflags & DEFCOMP_CMDPOS))
    {
      /* If END == START and text[0] == 0, we are trying to complete an empty
	 command word. */
      if (no_empty_command_completion && end == start && text[0] == '\0')
	{
	  matches = (char **)NULL;
	  rl_ignore_some_completions_function = bash_ignore_everything;
	}
      else
	{
#define CMD_IS_DIR(x)	(absolute_pathname(x) == 0 && absolute_program(x) == 0 && *(x) != '~' && test_for_directory (x))

	  dot_in_path = 0;
	  matches = rl_completion_matches (text, command_word_completion_function);

	  /* If we are attempting command completion and nothing matches, we
	     do not want readline to perform filename completion for us.  We
	     still want to be able to complete partial pathnames, so set the
	     completion ignore function to something which will remove
	     filenames and leave directories in the match list. */
	  if (matches == (char **)NULL)
	    rl_ignore_some_completions_function = bash_ignore_filenames;
	  else if (matches[1] == 0 && CMD_IS_DIR(matches[0]) && dot_in_path == 0)
	    /* If we found a single match, without looking in the current
	       directory (because it's not in $PATH), but the found name is
	       also a command in the current directory, suppress appending any
	       terminating character, since it's ambiguous. */
	    {
	      rl_completion_suppress_append = 1;
	      rl_filename_completion_desired = 0;
	    }
	  else if (matches[0] && matches[1] && STREQ (matches[0], matches[1]) && CMD_IS_DIR (matches[0]))
	    /* There are multiple instances of the same match (duplicate
	       completions haven't yet been removed).  In this case, all of
	       the matches will be the same, and the duplicate removal code
	       will distill them all down to one.  We turn on
	       rl_completion_suppress_append for the same reason as above.
	       Remember: we only care if there's eventually a single unique
	       completion.  If there are multiple completions this won't
	       make a difference and the problem won't occur. */
	    {
	      rl_completion_suppress_append = 1;
	      rl_filename_completion_desired = 0;
	    }
	}
    }

  /* This could be a globbing pattern, so try to expand it using pathname
     expansion. */
  if (!matches && glob_pattern_p (text))
    {
      matches = rl_completion_matches (text, glob_complete_word);
      /* A glob expression that matches more than one filename is problematic.
	 If we match more than one filename, punt. */
      if (matches && matches[1] && rl_completion_type == TAB)
	{
	  strvec_dispose (matches);
	  matches = (char **)0;
	}
    }

  return (matches);
}

/* This is the function to call when the word to complete is in a position
   where a command word can be found.  It grovels $PATH, looking for commands
   that match.  It also scans aliases, function names, and the shell_builtin
   table. */
char *
command_word_completion_function (hint_text, state)
     const char *hint_text;
     int state;
{
  static char *hint = (char *)NULL;
  static char *path = (char *)NULL;
  static char *val = (char *)NULL;
  static char *filename_hint = (char *)NULL;
  static char *dequoted_hint = (char *)NULL;
  static char *directory_part = (char *)NULL;
  static char **glob_matches = (char **)NULL;
  static int path_index, hint_len, dequoted_len, istate, igncase;
  static int mapping_over, local_index, searching_path, hint_is_dir;
  static int old_glob_ignore_case, globpat;
  static SHELL_VAR **varlist = (SHELL_VAR **)NULL;
#if defined (ALIAS)
  static alias_t **alias_list = (alias_t **)NULL;
#endif /* ALIAS */
  char *temp;

  /* We have to map over the possibilities for command words.  If we have
     no state, then make one just for that purpose. */
  if (state == 0)
    {
      if (dequoted_hint && dequoted_hint != hint)
	free (dequoted_hint);
      if (hint)
	free (hint);

      mapping_over = searching_path = 0;
      hint_is_dir = CMD_IS_DIR (hint_text);
      val = (char *)NULL;

      temp = rl_variable_value ("completion-ignore-case");
      igncase = RL_BOOLEAN_VARIABLE_VALUE (temp);

      if (glob_matches)
	{
	  free (glob_matches);
	  glob_matches = (char **)NULL;
	}

      globpat = glob_pattern_p (hint_text);

      /* If this is an absolute program name, do not check it against
	 aliases, reserved words, functions or builtins.  We must check
	 whether or not it is unique, and, if so, whether that filename
	 is executable. */
      if (globpat || absolute_program (hint_text))
	{
	  /* Perform tilde expansion on what's passed, so we don't end up
	     passing filenames with tildes directly to stat(). */
	  if (*hint_text == '~')
	    {
	      hint = bash_tilde_expand (hint_text, 0);
	      directory_part = savestring (hint_text);
	      temp = strchr (directory_part, '/');
	      if (temp)
		*temp = 0;
	      else
		{
		  free (directory_part);
		  directory_part = (char *)NULL;
		}
	    }
	  else
	    hint = savestring (hint_text);

	  dequoted_hint = hint;
	  /* If readline's completer found a quote character somewhere, but
	     didn't set the quote character, there must have been a quote
	     character embedded in the filename.  It can't be at the start of
	     the filename, so we need to dequote the filename before we look
	     in the file system for it. */
	  if (rl_completion_found_quote && rl_completion_quote_character == 0)
	    {
	      dequoted_hint = bash_dequote_filename (hint, 0);
	      free (hint);
	      hint = dequoted_hint;
	    }
	  dequoted_len = hint_len = strlen (hint);

	  if (filename_hint)
	    free (filename_hint);

	  filename_hint = savestring (hint);

	  istate = 0;

	  if (globpat)
	    {
	      mapping_over = 5;
	      goto globword;
	    }
	  else
	    {
	      mapping_over = 4;
	      goto inner;
	    }
	}

      dequoted_hint = hint = savestring (hint_text);
      dequoted_len = hint_len = strlen (hint);

      if (rl_completion_found_quote && rl_completion_quote_character == 0)
	{
	  dequoted_hint = bash_dequote_filename (hint, 0);
	  dequoted_len = strlen (dequoted_hint);
	}
      
      path = get_string_value ("PATH");
      path_index = dot_in_path = 0;

      /* Initialize the variables for each type of command word. */
      local_index = 0;

      if (varlist)
	free (varlist);

      varlist = all_visible_functions ();

#if defined (ALIAS)
      if (alias_list)
	free (alias_list);

      alias_list = all_aliases ();
#endif /* ALIAS */
    }

  /* mapping_over says what we are currently hacking.  Note that every case
     in this list must fall through when there are no more possibilities. */

  switch (mapping_over)
    {
    case 0:			/* Aliases come first. */
#if defined (ALIAS)
      while (alias_list && alias_list[local_index])
	{
	  register char *alias;

	  alias = alias_list[local_index++]->name;

	  if (STREQN (alias, hint, hint_len))
	    return (savestring (alias));
	}
#endif /* ALIAS */
      local_index = 0;
      mapping_over++;

    case 1:			/* Then shell reserved words. */
      {
	while (word_token_alist[local_index].word)
	  {
	    register char *reserved_word;

	    reserved_word = word_token_alist[local_index++].word;

	    if (STREQN (reserved_word, hint, hint_len))
	      return (savestring (reserved_word));
	  }
	local_index = 0;
	mapping_over++;
      }

    case 2:			/* Then function names. */
      while (varlist && varlist[local_index])
	{
	  register char *varname;

	  varname = varlist[local_index++]->name;

	  if (STREQN (varname, hint, hint_len))
	    return (savestring (varname));
	}
      local_index = 0;
      mapping_over++;

    case 3:			/* Then shell builtins. */
      for (; local_index < num_shell_builtins; local_index++)
	{
	  /* Ignore it if it doesn't have a function pointer or if it
	     is not currently enabled. */
	  if (!shell_builtins[local_index].function ||
	      (shell_builtins[local_index].flags & BUILTIN_ENABLED) == 0)
	    continue;

	  if (STREQN (shell_builtins[local_index].name, hint, hint_len))
	    {
	      int i = local_index++;

	      return (savestring (shell_builtins[i].name));
	    }
	}
      local_index = 0;
      mapping_over++;
    }

globword:
  /* Limited support for completing command words with globbing chars.  Only
     a single match (multiple matches that end up reducing the number of
     characters in the common prefix are bad) will ever be returned on
     regular completion. */
  if (globpat)
    {
      if (state == 0)
	{
	  glob_ignore_case = igncase;
	  glob_matches = shell_glob_filename (hint);
	  glob_ignore_case = old_glob_ignore_case;

	  if (GLOB_FAILED (glob_matches) || glob_matches == 0)
	    {
	      glob_matches = (char **)NULL;
	      return ((char *)NULL);
	    }

	  local_index = 0;
		
	  if (glob_matches[1] && rl_completion_type == TAB)	/* multiple matches are bad */
	    return ((char *)NULL);
	}

      while (val = glob_matches[local_index++])
        {
	  if (executable_or_directory (val))
	    {
	      if (*hint_text == '~')
		{
		  temp = restore_tilde (val, directory_part);
		  free (val);
		  val = temp;
		}
	      return (val);
	    }
	  free (val);
        }

      glob_ignore_case = old_glob_ignore_case;
      return ((char *)NULL);
    }

  /* If the text passed is a directory in the current directory, return it
     as a possible match.  Executables in directories in the current
     directory can be specified using relative pathnames and successfully
     executed even when `.' is not in $PATH. */
  if (hint_is_dir)
    {
      hint_is_dir = 0;	/* only return the hint text once */
      return (savestring (hint_text));
    }
    
  /* Repeatedly call filename_completion_function while we have
     members of PATH left.  Question:  should we stat each file?
     Answer: we call executable_file () on each file. */
 outer:

  istate = (val != (char *)NULL);

  if (istate == 0)
    {
      char *current_path;

      /* Get the next directory from the path.  If there is none, then we
	 are all done. */
      if (path == 0 || path[path_index] == 0 ||
	  (current_path = extract_colon_unit (path, &path_index)) == 0)
	return ((char *)NULL);

      searching_path = 1;
      if (*current_path == 0)
	{
	  free (current_path);
	  current_path = savestring (".");
	}

      if (*current_path == '~')
	{
	  char *t;

	  t = bash_tilde_expand (current_path, 0);
	  free (current_path);
	  current_path = t;
	}

      if (current_path[0] == '.' && current_path[1] == '\0')
	dot_in_path = 1;

      if (filename_hint)
	free (filename_hint);

      filename_hint = sh_makepath (current_path, hint, 0);
      free (current_path);		/* XXX */
    }

 inner:
  val = rl_filename_completion_function (filename_hint, istate);
  istate = 1;

  if (val == 0)
    {
      /* If the hint text is an absolute program, then don't bother
	 searching through PATH. */
      if (absolute_program (hint))
	return ((char *)NULL);

      goto outer;
    }
  else
    {
      int match, freetemp;

      if (absolute_program (hint))
	{
	  if (igncase == 0)
	    match = strncmp (val, hint, hint_len) == 0;
	  else
	    match = strncasecmp (val, hint, hint_len) == 0;

	  /* If we performed tilde expansion, restore the original
	     filename. */
	  if (*hint_text == '~')
	    temp = restore_tilde (val, directory_part);
	  else
	    temp = savestring (val);
	  freetemp = 1;
	}
      else
	{
	  temp = strrchr (val, '/');

	  if (temp)
	    {
	      temp++;
	      if (igncase == 0)
		freetemp = match = strncmp (temp, hint, hint_len) == 0;
	      else
		freetemp = match = strncasecmp (temp, hint, hint_len) == 0;
	      if (match)
		temp = savestring (temp);
	    }
	  else
	    freetemp = match = 0;
	}

#if 0
      /* If we have found a match, and it is an executable file or a
	 directory name, return it. */
      if (match && executable_or_directory (val))
#else
      /* If we have found a match, and it is an executable file, return it.
	 We don't return directory names when searching $PATH, since the
	 bash execution code won't find executables in directories which
	 appear in directories in $PATH when they're specified using
	 relative pathnames. */
      if (match && (searching_path ? executable_file (val) : executable_or_directory (val)))
#endif
	{
	  free (val);
	  val = "";		/* So it won't be NULL. */
	  return (temp);
	}
      else
	{
	  if (freetemp)
	    free (temp);
	  free (val);
	  goto inner;
	}
    }
}

/* Completion inside an unterminated command substitution. */
static char *
command_subst_completion_function (text, state)
     const char *text;
     int state;
{
  static char **matches = (char **)NULL;
  static const char *orig_start;
  static char *filename_text = (char *)NULL;
  static int cmd_index, start_len;
  char *value;

  if (state == 0)
    {
      if (filename_text)
	free (filename_text);
      orig_start = text;
      if (*text == '`')
	text++;
      else if (*text == '$' && text[1] == '(')	/* ) */
	text += 2;
      /* If the text was quoted, suppress any quote character that the
	 readline completion code would insert. */
      rl_completion_suppress_quote = 1;
      start_len = text - orig_start;
      filename_text = savestring (text);
      if (matches)
	free (matches);

      /*
       * At this point we can entertain the idea of re-parsing
       * `filename_text' into a (possibly incomplete) command name and
       * arguments, and doing completion based on that.  This is
       * currently very rudimentary, but it is a small improvement.
       */
      for (value = filename_text + strlen (filename_text) - 1; value > filename_text; value--)
        if (whitespace (*value) || member (*value, COMMAND_SEPARATORS))
          break;
      if (value <= filename_text)
	matches = rl_completion_matches (filename_text, command_word_completion_function);
      else
	{
	  value++;
	  start_len += value - filename_text;
	  if (whitespace (value[-1]))
	    matches = rl_completion_matches (value, rl_filename_completion_function);
	  else
	    matches = rl_completion_matches (value, command_word_completion_function);
	}

      /* If there is more than one match, rl_completion_matches has already
	 put the lcd in matches[0].  Skip over it. */
      cmd_index = matches && matches[0] && matches[1];

      /* If there's a single match and it's a directory, set the append char
	 to the expected `/'.  Otherwise, don't append anything. */
      if (matches && matches[0] && matches[1] == 0 && test_for_directory (matches[0]))
	rl_completion_append_character = '/';
      else
	rl_completion_suppress_append = 1;
    }

  if (!matches || !matches[cmd_index])
    {
      rl_filename_quoting_desired = 0;	/* disable quoting */
      return ((char *)NULL);
    }
  else
    {
      value = (char *)xmalloc (1 + start_len + strlen (matches[cmd_index]));

      if (start_len == 1)
	value[0] = *orig_start;
      else
	strncpy (value, orig_start, start_len);

      strcpy (value + start_len, matches[cmd_index]);

      cmd_index++;
      return (value);
    }
}

/* Okay, now we write the entry_function for variable completion. */
static char *
variable_completion_function (text, state)
     const char *text;
     int state;
{
  static char **varlist = (char **)NULL;
  static int varlist_index;
  static char *varname = (char *)NULL;
  static int namelen;
  static int first_char, first_char_loc;

  if (!state)
    {
      if (varname)
	free (varname);

      first_char_loc = 0;
      first_char = text[0];

      if (first_char == '$')
	first_char_loc++;

      if (text[first_char_loc] == '{')
	first_char_loc++;

      varname = savestring (text + first_char_loc);

      namelen = strlen (varname);
      if (varlist)
	strvec_dispose (varlist);

      varlist = all_variables_matching_prefix (varname);
      varlist_index = 0;
    }

  if (!varlist || !varlist[varlist_index])
    {
      return ((char *)NULL);
    }
  else
    {
      char *value;

      value = (char *)xmalloc (4 + strlen (varlist[varlist_index]));

      if (first_char_loc)
	{
	  value[0] = first_char;
	  if (first_char_loc == 2)
	    value[1] = '{';
	}

      strcpy (value + first_char_loc, varlist[varlist_index]);
      if (first_char_loc == 2)
	strcat (value, "}");

      varlist_index++;
      return (value);
    }
}

/* How about a completion function for hostnames? */
static char *
hostname_completion_function (text, state)
     const char *text;
     int state;
{
  static char **list = (char **)NULL;
  static int list_index = 0;
  static int first_char, first_char_loc;

  /* If we don't have any state, make some. */
  if (state == 0)
    {
      FREE (list);

      list = (char **)NULL;

      first_char_loc = 0;
      first_char = *text;

      if (first_char == '@')
	first_char_loc++;

      list = hostnames_matching ((char *)text+first_char_loc);
      list_index = 0;
    }

  if (list && list[list_index])
    {
      char *t;

      t = (char *)xmalloc (2 + strlen (list[list_index]));
      *t = first_char;
      strcpy (t + first_char_loc, list[list_index]);
      list_index++;
      return (t);
    }

  return ((char *)NULL);
}

/*
 * A completion function for service names from /etc/services (or wherever).
 */
char *
bash_servicename_completion_function (text, state)
     const char *text;
     int state;
{
#if defined (__WIN32__) || defined (__OPENNT) || !defined (HAVE_GETSERVENT)
  return ((char *)NULL);
#else
  static char *sname = (char *)NULL;
  static struct servent *srvent;
  static int snamelen, firstc;
  char *value;
  char **alist, *aentry;
  int afound;

  if (state == 0)
    {
      FREE (sname);
      firstc = *text;

      sname = savestring (text);
      snamelen = strlen (sname);
      setservent (0);
    }

  while (srvent = getservent ())
    {
      afound = 0;
      if (snamelen == 0 || (STREQN (sname, srvent->s_name, snamelen)))
	break;
      /* Not primary, check aliases */
      for (alist = srvent->s_aliases; *alist; alist++)
	{
	  aentry = *alist;
	  if (STREQN (sname, aentry, snamelen))
	    {
	      afound = 1;
	      break;
	    }
	}

      if (afound)
	break;
    }

  if (srvent == 0)
    {
      endservent ();
      return ((char *)NULL);
    }

  value = afound ? savestring (aentry) : savestring (srvent->s_name);
  return value;
#endif
}

/*
 * A completion function for group names from /etc/group (or wherever).
 */
char *
bash_groupname_completion_function (text, state)
     const char *text;
     int state;
{
#if defined (__WIN32__) || defined (__OPENNT) || !defined (HAVE_GRP_H)
  return ((char *)NULL);
#else
  static char *gname = (char *)NULL;
  static struct group *grent;
  static int gnamelen;
  char *value;

  if (state == 0)
    {
      FREE (gname);
      gname = savestring (text);
      gnamelen = strlen (gname);

      setgrent ();
    }

  while (grent = getgrent ())
    {
      if (gnamelen == 0 || (STREQN (gname, grent->gr_name, gnamelen)))
        break;
    }

  if (grent == 0)
    {
      endgrent ();
      return ((char *)NULL);
    }

  value = savestring (grent->gr_name);
  return (value);
#endif
}

/* Functions to perform history and alias expansions on the current line. */

#if defined (BANG_HISTORY)
/* Perform history expansion on the current line.  If no history expansion
   is done, pre_process_line() returns what it was passed, so we need to
   allocate a new line here. */
static char *
history_expand_line_internal (line)
     char *line;
{
  char *new_line;
  int old_verify;

  old_verify = hist_verify;
  hist_verify = 0;
  new_line = pre_process_line (line, 0, 0);
  hist_verify = old_verify;

  return (new_line == line) ? savestring (line) : new_line;
}
#endif

/* There was an error in expansion.  Let the preprocessor print
   the error here. */
static void
cleanup_expansion_error ()
{
  char *to_free;
#if defined (BANG_HISTORY)
  int old_verify;

  old_verify = hist_verify;
  hist_verify = 0;
#endif

  fprintf (rl_outstream, "\r\n");
  to_free = pre_process_line (rl_line_buffer, 1, 0);
#if defined (BANG_HISTORY)
  hist_verify = old_verify;
#endif
  if (to_free != rl_line_buffer)
    FREE (to_free);
  putc ('\r', rl_outstream);
  rl_forced_update_display ();
}

/* If NEW_LINE differs from what is in the readline line buffer, add an
   undo record to get from the readline line buffer contents to the new
   line and make NEW_LINE the current readline line. */
static void
maybe_make_readline_line (new_line)
     char *new_line;
{
  if (strcmp (new_line, rl_line_buffer) != 0)
    {
      rl_point = rl_end;

      rl_add_undo (UNDO_BEGIN, 0, 0, 0);
      rl_delete_text (0, rl_point);
      rl_point = rl_end = rl_mark = 0;
      rl_insert_text (new_line);
      rl_add_undo (UNDO_END, 0, 0, 0);
    }
}

/* Make NEW_LINE be the current readline line.  This frees NEW_LINE. */
static void
set_up_new_line (new_line)
     char *new_line;
{
  int old_point, at_end;

  old_point = rl_point;
  at_end = rl_point == rl_end;

  /* If the line was history and alias expanded, then make that
     be one thing to undo. */
  maybe_make_readline_line (new_line);
  free (new_line);

  /* Place rl_point where we think it should go. */
  if (at_end)
    rl_point = rl_end;
  else if (old_point < rl_end)
    {
      rl_point = old_point;
      if (!whitespace (rl_line_buffer[rl_point]))
	rl_forward_word (1, 0);
    }
}

#if defined (ALIAS)
/* Expand aliases in the current readline line. */
static int
alias_expand_line (count, ignore)
     int count, ignore;
{
  char *new_line;

  new_line = alias_expand (rl_line_buffer);

  if (new_line)
    {
      set_up_new_line (new_line);
      return (0);
    }
  else
    {
      cleanup_expansion_error ();
      return (1);
    }
}
#endif

#if defined (BANG_HISTORY)
/* History expand the line. */
static int
history_expand_line (count, ignore)
     int count, ignore;
{
  char *new_line;

  new_line = history_expand_line_internal (rl_line_buffer);

  if (new_line)
    {
      set_up_new_line (new_line);
      return (0);
    }
  else
    {
      cleanup_expansion_error ();
      return (1);
    }
}

/* Expand history substitutions in the current line and then insert a
   space (hopefully close to where we were before). */
static int
tcsh_magic_space (count, ignore)
     int count, ignore;
{
  int dist_from_end, old_point;

  old_point = rl_point;
  dist_from_end = rl_end - rl_point;
  if (history_expand_line (count, ignore) == 0)
    {
      /* Try a simple heuristic from Stephen Gildea <gildea@intouchsys.com>.
	 This works if all expansions were before rl_point or if no expansions
	 were performed. */
      rl_point = (old_point == 0) ? old_point : rl_end - dist_from_end;
      rl_insert (1, ' ');
      return (0);
    }
  else
    return (1);
}
#endif /* BANG_HISTORY */

/* History and alias expand the line. */
static int
history_and_alias_expand_line (count, ignore)
     int count, ignore;
{
  char *new_line;

  new_line = 0;
#if defined (BANG_HISTORY)
  new_line = history_expand_line_internal (rl_line_buffer);
#endif

#if defined (ALIAS)
  if (new_line)
    {
      char *alias_line;

      alias_line = alias_expand (new_line);
      free (new_line);
      new_line = alias_line;
    }
#endif /* ALIAS */

  if (new_line)
    {
      set_up_new_line (new_line);
      return (0);
    }
  else
    {
      cleanup_expansion_error ();
      return (1);
    }
}

/* History and alias expand the line, then perform the shell word
   expansions by calling expand_string.  This can't use set_up_new_line()
   because we want the variable expansions as a separate undo'able
   set of operations. */
static int
shell_expand_line (count, ignore)
     int count, ignore;
{
  char *new_line;
  WORD_LIST *expanded_string;

  new_line = 0;
#if defined (BANG_HISTORY)
  new_line = history_expand_line_internal (rl_line_buffer);
#endif

#if defined (ALIAS)
  if (new_line)
    {
      char *alias_line;

      alias_line = alias_expand (new_line);
      free (new_line);
      new_line = alias_line;
    }
#endif /* ALIAS */

  if (new_line)
    {
      int old_point = rl_point;
      int at_end = rl_point == rl_end;

      /* If the line was history and alias expanded, then make that
	 be one thing to undo. */
      maybe_make_readline_line (new_line);
      free (new_line);

      /* If there is variable expansion to perform, do that as a separate
	 operation to be undone. */
      new_line = savestring (rl_line_buffer);
      expanded_string = expand_string (new_line, 0);
      FREE (new_line);
      if (expanded_string == 0)
	{
	  new_line = (char *)xmalloc (1);
	  new_line[0] = '\0';
	}
      else
	{
	  new_line = string_list (expanded_string);
	  dispose_words (expanded_string);
	}

      maybe_make_readline_line (new_line);
      free (new_line);

      /* Place rl_point where we think it should go. */
      if (at_end)
	rl_point = rl_end;
      else if (old_point < rl_end)
	{
	  rl_point = old_point;
	  if (!whitespace (rl_line_buffer[rl_point]))
	    rl_forward_word (1, 0);
	}
      return 0;
    }
  else
    {
      cleanup_expansion_error ();
      return 1;
    }
}

/* If FIGNORE is set, then don't match files with the given suffixes when
   completing filenames.  If only one of the possibilities has an acceptable
   suffix, delete the others, else just return and let the completer
   signal an error.  It is called by the completer when real
   completions are done on filenames by the completer's internal
   function, not for completion lists (M-?) and not on "other"
   completion types, such as hostnames or commands. */

static struct ignorevar fignore =
{
  "FIGNORE",
  (struct ign *)0,
  0,
  (char *)0,
  (sh_iv_item_func_t *) 0,
};

static void
_ignore_completion_names (names, name_func)
     char **names;
     sh_ignore_func_t *name_func;
{
  char **newnames;
  int idx, nidx;
  char **oldnames;
  int oidx;

  /* If there is only one completion, see if it is acceptable.  If it is
     not, free it up.  In any case, short-circuit and return.  This is a
     special case because names[0] is not the prefix of the list of names
     if there is only one completion; it is the completion itself. */
  if (names[1] == (char *)0)
    {
      if (force_fignore)
	if ((*name_func) (names[0]) == 0)
	  {
	    free (names[0]);
	    names[0] = (char *)NULL;
	  }

      return;
    }

  /* Allocate space for array to hold list of pointers to matching
     filenames.  The pointers are copied back to NAMES when done. */
  for (nidx = 1; names[nidx]; nidx++)
    ;
  newnames = strvec_create (nidx + 1);

  if (force_fignore == 0)
    {
      oldnames = strvec_create (nidx - 1);
      oidx = 0;
    }

  newnames[0] = names[0];
  for (idx = nidx = 1; names[idx]; idx++)
    {
      if ((*name_func) (names[idx]))
	newnames[nidx++] = names[idx];
      else if (force_fignore == 0)
	oldnames[oidx++] = names[idx];
      else
	free (names[idx]);
    }

  newnames[nidx] = (char *)NULL;

  /* If none are acceptable then let the completer handle it. */
  if (nidx == 1)
    {
      if (force_fignore)
	{
	  free (names[0]);
	  names[0] = (char *)NULL;
	}
      else
	free (oldnames);

      free (newnames);
      return;
    }

  if (force_fignore == 0)
    {
      while (oidx)
	free (oldnames[--oidx]);
      free (oldnames);
    }

  /* If only one is acceptable, copy it to names[0] and return. */
  if (nidx == 2)
    {
      free (names[0]);
      names[0] = newnames[1];
      names[1] = (char *)NULL;
      free (newnames);
      return;
    }

  /* Copy the acceptable names back to NAMES, set the new array end,
     and return. */
  for (nidx = 1; newnames[nidx]; nidx++)
    names[nidx] = newnames[nidx];
  names[nidx] = (char *)NULL;
  free (newnames);
}

static int
name_is_acceptable (name)
     const char *name;
{
  struct ign *p;
  int nlen;

  for (nlen = strlen (name), p = fignore.ignores; p->val; p++)
    {
      if (nlen > p->len && p->len > 0 && STREQ (p->val, &name[nlen - p->len]))
	return (0);
    }

  return (1);
}

#if 0
static int
ignore_dot_names (name)
     char *name;
{
  return (name[0] != '.');
}
#endif

static int
filename_completion_ignore (names)
     char **names;
{
#if 0
  if (glob_dot_filenames == 0)
    _ignore_completion_names (names, ignore_dot_names);
#endif

  setup_ignore_patterns (&fignore);

  if (fignore.num_ignores == 0)
    return 0;

  _ignore_completion_names (names, name_is_acceptable);

  return 0;
}

/* Return 1 if NAME is a directory.  NAME undergoes tilde expansion. */
static int
test_for_directory (name)
     const char *name;
{
  char *fn;
  int r;

  fn = bash_tilde_expand (name, 0);
  r = file_isdir (fn);
  free (fn);

  return (r);
}

/* Remove files from NAMES, leaving directories. */
static int
bash_ignore_filenames (names)
     char **names;
{
  _ignore_completion_names (names, test_for_directory);
  return 0;
}

static int
return_zero (name)
     const char *name;
{
  return 0;
}

static int
bash_ignore_everything (names)
     char **names;
{
  _ignore_completion_names (names, return_zero);
  return 0;
}

/* Replace a tilde-prefix in VAL with a `~', assuming the user typed it.  VAL
   is an expanded filename.  DIRECTORY_PART is the tilde-prefix portion
   of the un-tilde-expanded version of VAL (what the user typed). */
static char *
restore_tilde (val, directory_part)
     char *val, *directory_part;
{
  int l, vl, dl2, xl;
  char *dh2, *expdir, *ret;

  vl = strlen (val);

  /* We need to duplicate the expansions readline performs on the directory
     portion before passing it to our completion function. */
  dh2 = directory_part ? bash_dequote_filename (directory_part, 0) : 0;
  bash_directory_expansion (&dh2);
  dl2 = strlen (dh2);

  expdir = bash_tilde_expand (directory_part, 0);
  xl = strlen (expdir);
  free (expdir);

  /*
     dh2 = unexpanded but dequoted tilde-prefix
     dl2 = length of tilde-prefix
     expdir = tilde-expanded tilde-prefix
     xl = length of expanded tilde-prefix
     l = length of remainder after tilde-prefix
  */
  l = (vl - xl) + 1;

  ret = (char *)xmalloc (dl2 + 2 + l);
  strcpy (ret, dh2);
  strcpy (ret + dl2, val + xl);

  free (dh2);
  return (ret);
}

/* Simulate the expansions that will be performed by
   rl_filename_completion_function.  This must be called with the address of
   a pointer to malloc'd memory. */
static void
bash_directory_expansion (dirname)
     char **dirname;
{
  char *d, *nd;

  d = savestring (*dirname);

  if (rl_directory_rewrite_hook)
    (*rl_directory_rewrite_hook) (&d);

  if (rl_directory_completion_hook && (*rl_directory_completion_hook) (&d))
    {
      free (*dirname);
      *dirname = d;
    }
  else if (rl_completion_found_quote)
    {
      nd = bash_dequote_filename (d, rl_completion_quote_character);
      free (*dirname);
      free (d);
      *dirname = nd;
    }
}

/* If necessary, rewrite directory entry */
static char *
bash_filename_rewrite_hook (fname, fnlen)
     char *fname;
     int fnlen;
{
  char *conv;

  conv = fnx_fromfs (fname, fnlen);
  if (conv != fname)
    conv = savestring (conv);
  return conv;
}

/* Handle symbolic link references and other directory name
   expansions while hacking completion. */
static int
bash_directory_completion_hook (dirname)
     char **dirname;
{
  char *local_dirname, *new_dirname, *t;
  int return_value, should_expand_dirname;
  WORD_LIST *wl;
  struct stat sb;

  return_value = should_expand_dirname = 0;
  local_dirname = *dirname;

  if (mbschr (local_dirname, '$'))
    should_expand_dirname = 1;
  else
    {
      t = mbschr (local_dirname, '`');
      if (t && unclosed_pair (local_dirname, strlen (local_dirname), "`") == 0)
	should_expand_dirname = 1;
    }

#if defined (HAVE_LSTAT)
  if (should_expand_dirname && lstat (local_dirname, &sb) == 0)
#else
  if (should_expand_dirname && stat (local_dirname, &sb) == 0)
#endif
    should_expand_dirname = 0;

  if (should_expand_dirname)  
    {
      new_dirname = savestring (local_dirname);
      wl = expand_prompt_string (new_dirname, 0, W_NOCOMSUB);	/* does the right thing */
      if (wl)
	{
	  *dirname = string_list (wl);
	  /* Tell the completer to replace the directory name only if we
	     actually expanded something. */
	  return_value = STREQ (local_dirname, *dirname) == 0;
	  free (local_dirname);
	  free (new_dirname);
	  dispose_words (wl);
	  local_dirname = *dirname;
	}
      else
	{
	  free (new_dirname);
	  free (local_dirname);
	  *dirname = (char *)xmalloc (1);
	  **dirname = '\0';
	  return 1;
	}
    }
  else 
    {
      /* Dequote the filename even if we don't expand it. */
      new_dirname = bash_dequote_filename (local_dirname, rl_completion_quote_character);
      free (local_dirname);
      local_dirname = *dirname = new_dirname;
    }

  if (no_symbolic_links == 0 && (local_dirname[0] != '.' || local_dirname[1]))
    {
      char *temp1, *temp2;
      int len1, len2;

      t = get_working_directory ("symlink-hook");
      temp1 = make_absolute (local_dirname, t);
      free (t);
      temp2 = sh_canonpath (temp1, PATH_CHECKDOTDOT|PATH_CHECKEXISTS);

      /* Try spelling correction if initial canonicalization fails. */
      if (temp2 == 0 && dircomplete_spelling)
	{
	  temp2 = dirspell (temp1);
	  if (temp2)
	    {
	      free (temp1);
	      temp1 = temp2;
	      temp2 = sh_canonpath (temp1, PATH_CHECKDOTDOT|PATH_CHECKEXISTS);
	      return_value = temp2 != 0;
	    }
	}
      /* If we can't canonicalize, bail. */
      if (temp2 == 0)
	{
	  free (temp1);
	  return 1;
	}
      len1 = strlen (temp1);
      if (temp1[len1 - 1] == '/')
	{
	  len2 = strlen (temp2);
	  if (len2 > 2)		/* don't append `/' to `/' or `//' */
	    {
	      temp2 = (char *)xrealloc (temp2, len2 + 2);
	      temp2[len2] = '/';
	      temp2[len2 + 1] = '\0';
	    }
	}
      free (local_dirname);
      *dirname = temp2;
      free (temp1);
    }
  return (return_value);
}

static char **history_completion_array = (char **)NULL;
static int harry_size;
static int harry_len;

static void
build_history_completion_array ()
{
  register int i, j;
  HIST_ENTRY **hlist;
  char **tokens;

  /* First, clear out the current dynamic history completion list. */
  if (harry_size)
    {
      strvec_dispose (history_completion_array);
      history_completion_array = (char **)NULL;
      harry_size = 0;
      harry_len = 0;
    }

  /* Next, grovel each line of history, making each shell-sized token
     a separate entry in the history_completion_array. */
  hlist = history_list ();

  if (hlist)
    {
      for (i = 0; hlist[i]; i++)
	;
      for ( --i; i >= 0; i--)
	{
	  /* Separate each token, and place into an array. */
	  tokens = history_tokenize (hlist[i]->line);

	  for (j = 0; tokens && tokens[j]; j++)
	    {
	      if (harry_len + 2 > harry_size)
	        history_completion_array = strvec_resize (history_completion_array, harry_size += 10);

	      history_completion_array[harry_len++] = tokens[j];
	      history_completion_array[harry_len] = (char *)NULL;
	    }
	  free (tokens);
	}

      /* Sort the complete list of tokens. */
      if (dabbrev_expand_active == 0)
        qsort (history_completion_array, harry_len, sizeof (char *), (QSFUNC *)strvec_strcmp);
    }
}

static char *
history_completion_generator (hint_text, state)
     const char *hint_text;
     int state;
{
  static int local_index, len;
  static const char *text;

  /* If this is the first call to the generator, then initialize the
     list of strings to complete over. */
  if (state == 0)
    {
      if (dabbrev_expand_active)	/* This is kind of messy */
	rl_completion_suppress_append = 1;
      local_index = 0;
      build_history_completion_array ();
      text = hint_text;
      len = strlen (text);
    }

  while (history_completion_array && history_completion_array[local_index])
    {
      if (strncmp (text, history_completion_array[local_index++], len) == 0)
	return (savestring (history_completion_array[local_index - 1]));
    }
  return ((char *)NULL);
}

static int
dynamic_complete_history (count, key)
     int count, key;
{
  int r;
  rl_compentry_func_t *orig_func;
  rl_completion_func_t *orig_attempt_func;

  orig_func = rl_completion_entry_function;
  orig_attempt_func = rl_attempted_completion_function;

  rl_completion_entry_function = history_completion_generator;
  rl_attempted_completion_function = (rl_completion_func_t *)NULL;

  /* XXX - use rl_completion_mode here? */
  if (rl_last_func == dynamic_complete_history)
    r = rl_complete_internal ('?');
  else
    r = rl_complete_internal (TAB);

  rl_completion_entry_function = orig_func;
  rl_attempted_completion_function = orig_attempt_func;
  return r;
}

static int
bash_dabbrev_expand (count, key)
     int count, key;
{
  int r, orig_suppress, orig_sort;
  rl_compentry_func_t *orig_func;
  rl_completion_func_t *orig_attempt_func;

  orig_func = rl_menu_completion_entry_function;
  orig_attempt_func = rl_attempted_completion_function;
  orig_suppress = rl_completion_suppress_append;
  orig_sort = rl_sort_completion_matches;

  rl_menu_completion_entry_function = history_completion_generator;
  rl_attempted_completion_function = (rl_completion_func_t *)NULL;
  rl_filename_completion_desired = 0;
  rl_completion_suppress_append = 1;
  rl_sort_completion_matches = 0;

  /* XXX - use rl_completion_mode here? */
  dabbrev_expand_active = 1;
  if (rl_last_func == bash_dabbrev_expand)
    rl_last_func = rl_menu_complete;
  r = rl_menu_complete (count, key);
  dabbrev_expand_active = 0;

  rl_last_func = bash_dabbrev_expand;
  rl_menu_completion_entry_function = orig_func;
  rl_attempted_completion_function = orig_attempt_func;
  rl_completion_suppress_append = orig_suppress;
  rl_sort_completion_matches = orig_sort;

  return r;
}

#if defined (SPECIFIC_COMPLETION_FUNCTIONS)
static int
bash_complete_username (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_username_internal (rl_completion_mode (bash_complete_username));
}

static int
bash_possible_username_completions (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_username_internal ('?');
}

static int
bash_complete_username_internal (what_to_do)
     int what_to_do;
{
  return bash_specific_completion (what_to_do, rl_username_completion_function);
}

static int
bash_complete_filename (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_filename_internal (rl_completion_mode (bash_complete_filename));
}

static int
bash_possible_filename_completions (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_filename_internal ('?');
}

static int
bash_complete_filename_internal (what_to_do)
     int what_to_do;
{
  rl_compentry_func_t *orig_func;
  rl_completion_func_t *orig_attempt_func;
  rl_icppfunc_t *orig_dir_func;
  /*const*/ char *orig_rl_completer_word_break_characters;
  int r;

  orig_func = rl_completion_entry_function;
  orig_attempt_func = rl_attempted_completion_function;
  orig_dir_func = rl_directory_completion_hook;
  orig_rl_completer_word_break_characters = rl_completer_word_break_characters;
  rl_completion_entry_function = rl_filename_completion_function;
  rl_attempted_completion_function = (rl_completion_func_t *)NULL;
  rl_directory_completion_hook = (rl_icppfunc_t *)NULL;
  rl_completer_word_break_characters = " \t\n\"\'";

  r = rl_complete_internal (what_to_do);

  rl_completion_entry_function = orig_func;
  rl_attempted_completion_function = orig_attempt_func;
  rl_directory_completion_hook = orig_dir_func;
  rl_completer_word_break_characters = orig_rl_completer_word_break_characters;

  return r;
}

static int
bash_complete_hostname (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_hostname_internal (rl_completion_mode (bash_complete_hostname));
}

static int
bash_possible_hostname_completions (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_hostname_internal ('?');
}

static int
bash_complete_variable (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_variable_internal (rl_completion_mode (bash_complete_variable));
}

static int
bash_possible_variable_completions (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_variable_internal ('?');
}

static int
bash_complete_command (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_command_internal (rl_completion_mode (bash_complete_command));
}

static int
bash_possible_command_completions (ignore, ignore2)
     int ignore, ignore2;
{
  return bash_complete_command_internal ('?');
}

static int
bash_complete_hostname_internal (what_to_do)
     int what_to_do;
{
  return bash_specific_completion (what_to_do, hostname_completion_function);
}

static int
bash_complete_variable_internal (what_to_do)
     int what_to_do;
{
  return bash_specific_completion (what_to_do, variable_completion_function);
}

static int
bash_complete_command_internal (what_to_do)
     int what_to_do;
{
  return bash_specific_completion (what_to_do, command_word_completion_function);
}

static char *globtext;
static char *globorig;

static char *
glob_complete_word (text, state)
     const char *text;
     int state;
{
  static char **matches = (char **)NULL;
  static int ind;
  int glen;
  char *ret, *ttext;

  if (state == 0)
    {
      rl_filename_completion_desired = 1;
      FREE (matches);
      if (globorig != globtext)
	FREE (globorig);
      FREE (globtext);

      ttext = bash_tilde_expand (text, 0);

      if (rl_explicit_arg)
	{
	  globorig = savestring (ttext);
	  glen = strlen (ttext);
	  globtext = (char *)xmalloc (glen + 2);
	  strcpy (globtext, ttext);
	  globtext[glen] = '*';
	  globtext[glen+1] = '\0';
	}
      else
        globtext = globorig = savestring (ttext);

      if (ttext != text)
	free (ttext);

      matches = shell_glob_filename (globtext);
      if (GLOB_FAILED (matches))
	matches = (char **)NULL;
      ind = 0;
    }

  ret = matches ? matches[ind] : (char *)NULL;
  ind++;
  return ret;
}

static int
bash_glob_completion_internal (what_to_do)
     int what_to_do;
{
  return bash_specific_completion (what_to_do, glob_complete_word);
}

/* A special quoting function so we don't end up quoting globbing characters
   in the word if there are no matches or multiple matches. */
static char *
bash_glob_quote_filename (s, rtype, qcp)
     char *s;
     int rtype;
     char *qcp;
{
  if (globorig && qcp && *qcp == '\0' && STREQ (s, globorig))
    return (savestring (s));
  else
    return (bash_quote_filename (s, rtype, qcp));
}

static int
bash_glob_complete_word (count, key)
     int count, key;
{
  int r;
  rl_quote_func_t *orig_quoting_function;

  if (rl_editing_mode == EMACS_EDITING_MODE)
    rl_explicit_arg = 1;	/* force `*' append */
  orig_quoting_function = rl_filename_quoting_function;
  rl_filename_quoting_function = bash_glob_quote_filename;
  
  r = bash_glob_completion_internal (rl_completion_mode (bash_glob_complete_word));

  rl_filename_quoting_function = orig_quoting_function;
  return r;
}

static int
bash_glob_expand_word (count, key)
     int count, key;
{
  return bash_glob_completion_internal ('*');
}

static int
bash_glob_list_expansions (count, key)
     int count, key;
{
  return bash_glob_completion_internal ('?');
}

static int
bash_specific_completion (what_to_do, generator)
     int what_to_do;
     rl_compentry_func_t *generator;
{
  rl_compentry_func_t *orig_func;
  rl_completion_func_t *orig_attempt_func;
  int r;

  orig_func = rl_completion_entry_function;
  orig_attempt_func = rl_attempted_completion_function;
  rl_completion_entry_function = generator;
  rl_attempted_completion_function = NULL;

  r = rl_complete_internal (what_to_do);

  rl_completion_entry_function = orig_func;
  rl_attempted_completion_function = orig_attempt_func;

  return r;
}

#endif	/* SPECIFIC_COMPLETION_FUNCTIONS */

#if defined (VI_MODE)
/* Completion, from vi mode's point of view.  This is a modified version of
   rl_vi_complete which uses the bash globbing code to implement what POSIX
   specifies, which is to append a `*' and attempt filename generation (which
   has the side effect of expanding any globbing characters in the word). */
static int
bash_vi_complete (count, key)
     int count, key;
{
#if defined (SPECIFIC_COMPLETION_FUNCTIONS)
  int p, r;
  char *t;

  if ((rl_point < rl_end) && (!whitespace (rl_line_buffer[rl_point])))
    {
      if (!whitespace (rl_line_buffer[rl_point + 1]))
	rl_vi_end_word (1, 'E');
      rl_point++;
    }

  /* Find boundaries of current word, according to vi definition of a
     `bigword'. */
  t = 0;
  if (rl_point > 0)
    {
      p = rl_point;
      rl_vi_bWord (1, 'B');
      r = rl_point;
      rl_point = p;
      p = r;

      t = substring (rl_line_buffer, p, rl_point);
    }      

  if (t && glob_pattern_p (t) == 0)
    rl_explicit_arg = 1;	/* XXX - force glob_complete_word to append `*' */
  FREE (t);

  if (key == '*')	/* Expansion and replacement. */
    r = bash_glob_expand_word (count, key);
  else if (key == '=')	/* List possible completions. */
    r = bash_glob_list_expansions (count, key);
  else if (key == '\\')	/* Standard completion */
    r = bash_glob_complete_word (count, key);
  else
    r = rl_complete (0, key);

  if (key == '*' || key == '\\')
    rl_vi_start_inserting (key, 1, 1);

  return (r);
#else
  return rl_vi_complete (count, key);
#endif /* !SPECIFIC_COMPLETION_FUNCTIONS */
}
#endif /* VI_MODE */

/* Filename quoting for completion. */
/* A function to strip unquoted quote characters (single quotes, double
   quotes, and backslashes).  It allows single quotes to appear
   within double quotes, and vice versa.  It should be smarter. */
static char *
bash_dequote_filename (text, quote_char)
     char *text;
     int quote_char;
{
  char *ret, *p, *r;
  int l, quoted;

  l = strlen (text);
  ret = (char *)xmalloc (l + 1);
  for (quoted = quote_char, p = text, r = ret; p && *p; p++)
    {
      /* Allow backslash-escaped characters to pass through unscathed. */
      if (*p == '\\')
	{
	  /* Backslashes are preserved within single quotes. */
	  if (quoted == '\'')
	    *r++ = *p;
	  /* Backslashes are preserved within double quotes unless the
	     character is one that is defined to be escaped */
	  else if (quoted == '"' && ((sh_syntaxtab[p[1]] & CBSDQUOTE) == 0))
	    *r++ = *p;

	  *r++ = *++p;
	  if (*p == '\0')
	    return ret;		/* XXX - was break; */
	  continue;
	}
      /* Close quote. */
      if (quoted && *p == quoted)
	{
	  quoted = 0;
	  continue;
	}
      /* Open quote. */
      if (quoted == 0 && (*p == '\'' || *p == '"'))
	{
	  quoted = *p;
	  continue;
	}
      *r++ = *p;
    }
  *r = '\0';
  return ret;
}

/* Quote characters that the readline completion code would treat as
   word break characters with backslashes.  Pass backslash-quoted
   characters through without examination. */
static char *
quote_word_break_chars (text)
     char *text;
{
  char *ret, *r, *s;
  int l;

  l = strlen (text);
  ret = (char *)xmalloc ((2 * l) + 1);
  for (s = text, r = ret; *s; s++)
    {
      /* Pass backslash-quoted characters through, including the backslash. */
      if (*s == '\\')
	{
	  *r++ = '\\';
	  *r++ = *++s;
	  if (*s == '\0')
	    break;
	  continue;
	}
      /* OK, we have an unquoted character.  Check its presence in
	 rl_completer_word_break_characters. */
      if (mbschr (rl_completer_word_break_characters, *s))
	*r++ = '\\';
      /* XXX -- check for standalone tildes here and backslash-quote them */
      if (s == text && *s == '~' && file_exists (text))
        *r++ = '\\';
      *r++ = *s;
    }
  *r = '\0';
  return ret;
}

/* Quote a filename using double quotes, single quotes, or backslashes
   depending on the value of completion_quoting_style.  If we're
   completing using backslashes, we need to quote some additional
   characters (those that readline treats as word breaks), so we call
   quote_word_break_chars on the result.  This returns newly-allocated
   memory. */
static char *
bash_quote_filename (s, rtype, qcp)
     char *s;
     int rtype;
     char *qcp;
{
  char *rtext, *mtext, *ret;
  int rlen, cs;

  rtext = (char *)NULL;

  /* If RTYPE == MULT_MATCH, it means that there is
     more than one match.  In this case, we do not add
     the closing quote or attempt to perform tilde
     expansion.  If RTYPE == SINGLE_MATCH, we try
     to perform tilde expansion, because single and double
     quotes inhibit tilde expansion by the shell. */

  cs = completion_quoting_style;
  /* Might need to modify the default completion style based on *qcp,
     since it's set to any user-provided opening quote.  We also change
     to single-quoting if there is no user-provided opening quote and
     the word being completed contains newlines, since those are not
     quoted correctly using backslashes (a backslash-newline pair is
     special to the shell parser). */
  if (*qcp == '\0' && cs == COMPLETE_BSQUOTE && mbschr (s, '\n'))
    cs = COMPLETE_SQUOTE;
  else if (*qcp == '"')
    cs = COMPLETE_DQUOTE;
  else if (*qcp == '\'')
    cs = COMPLETE_SQUOTE;
#if defined (BANG_HISTORY)
  else if (*qcp == '\0' && history_expansion && cs == COMPLETE_DQUOTE &&
	   history_expansion_inhibited == 0 && mbschr (s, '!'))
    cs = COMPLETE_BSQUOTE;

  if (*qcp == '"' && history_expansion && cs == COMPLETE_DQUOTE &&
	history_expansion_inhibited == 0 && mbschr (s, '!'))
    {
      cs = COMPLETE_BSQUOTE;
      *qcp = '\0';
    }
#endif

  /* Don't tilde-expand backslash-quoted filenames, since only single and
     double quotes inhibit tilde expansion. */
  mtext = s;
  if (mtext[0] == '~' && rtype == SINGLE_MATCH && cs != COMPLETE_BSQUOTE)
    mtext = bash_tilde_expand (s, 0);

  switch (cs)
    {
    case COMPLETE_DQUOTE:
      rtext = sh_double_quote (mtext);
      break;
    case COMPLETE_SQUOTE:
      rtext = sh_single_quote (mtext);
      break;
    case COMPLETE_BSQUOTE:
      rtext = sh_backslash_quote (mtext);
      break;
    }

  if (mtext != s)
    free (mtext);

  /* We may need to quote additional characters: those that readline treats
     as word breaks that are not quoted by backslash_quote. */
  if (rtext && cs == COMPLETE_BSQUOTE)
    {
      mtext = quote_word_break_chars (rtext);
      free (rtext);
      rtext = mtext;
    }

  /* Leave the opening quote intact.  The readline completion code takes
     care of avoiding doubled opening quotes. */
  rlen = strlen (rtext);
  ret = (char *)xmalloc (rlen + 1);
  strcpy (ret, rtext);

  /* If there are multiple matches, cut off the closing quote. */
  if (rtype == MULT_MATCH && cs != COMPLETE_BSQUOTE)
    ret[rlen - 1] = '\0';
  free (rtext);
  return ret;
}

/* Support for binding readline key sequences to Unix commands. */
static Keymap cmd_xmap;

static int
putx(c)
     int c;
{
  putc (c, rl_outstream);
}
  
static int
bash_execute_unix_command (count, key)
     int count;	/* ignored */
     int key;
{
  Keymap ckmap;		/* current keymap */
  Keymap xkmap;		/* unix command executing keymap */
  register int i, r;
  intmax_t mi;
  sh_parser_state_t ps;
  char *cmd, *value, *l, *l1, *ce;
  SHELL_VAR *v;
  char ibuf[INT_STRLEN_BOUND(int) + 1];

  /* First, we need to find the right command to execute.  This is tricky,
     because we might have already indirected into another keymap. */
  ckmap = rl_get_keymap ();
  if (ckmap != rl_executing_keymap)
    {
      /* bogus.  we have to search.  only handle one level of indirection. */
      for (i = 0; i < KEYMAP_SIZE; i++)
	{
	  if (ckmap[i].type == ISKMAP && (Keymap)ckmap[i].function == rl_executing_keymap)
	    break;
	}
      if (i < KEYMAP_SIZE)
	xkmap = (Keymap)cmd_xmap[i].function;
      else
	{
	  rl_crlf ();
	  internal_error (_("bash_execute_unix_command: cannot find keymap for command"));
	  rl_forced_update_display ();
	  return 1;
	}
    }
  else
    xkmap = cmd_xmap;

  cmd = (char *)xkmap[key].function;

  if (cmd == 0)
    {
      rl_ding ();
      return 1;
    }

  ce = rl_get_termcap ("ce");
  if (ce)	/* clear current line */
    {
      fprintf (rl_outstream, "\r");
      tputs (ce, 1, putx);
      fflush (rl_outstream);
    }
  else
    rl_crlf ();	/* move to a new line */

  v = bind_variable ("READLINE_LINE", rl_line_buffer, 0);
  if (v)
    VSETATTR (v, att_exported);
  l = v ? value_cell (v) : 0;
  value = inttostr (rl_point, ibuf, sizeof (ibuf));
  v = bind_int_variable ("READLINE_POINT", value);
  if (v)
    VSETATTR (v, att_exported);
  array_needs_making = 1;

  save_parser_state (&ps);
  r = parse_and_execute (cmd, "bash_execute_unix_command", SEVAL_NOHIST|SEVAL_NOFREE);
  restore_parser_state (&ps);

  v = find_variable ("READLINE_LINE");
  l1 = v ? value_cell (v) : 0;
  if (l1 != l)
    maybe_make_readline_line (value_cell (v));
  v = find_variable ("READLINE_POINT");
  if (v && legal_number (value_cell (v), &mi))
    {
      i = mi;
      if (i != rl_point)
	{
	  rl_point = i;
	  if (rl_point > rl_end)
	    rl_point = rl_end;
	  else if (rl_point < 0)
	    rl_point = 0;
	}
    }      

  unbind_variable ("READLINE_LINE");
  unbind_variable ("READLINE_POINT");
  array_needs_making = 1;

  /* and restore the readline buffer and display after command execution. */
  rl_forced_update_display ();
  return 0;
}

static void
init_unix_command_map ()
{
  cmd_xmap = rl_make_bare_keymap ();
}

static int
isolate_sequence (string, ind, need_dquote, startp)
     char *string;
     int ind, need_dquote, *startp;
{
  register int i;
  int c, passc, delim;

  for (i = ind; string[i] && whitespace (string[i]); i++)
    ;
  /* NEED_DQUOTE means that the first non-white character *must* be `"'. */
  if (need_dquote && string[i] != '"')
    {
      builtin_error (_("%s: first non-whitespace character is not `\"'"), string);
      return -1;
    }

  /* We can have delimited strings even if NEED_DQUOTE == 0, like the command
     string to bind the key sequence to. */
  delim = (string[i] == '"' || string[i] == '\'') ? string[i] : 0;
    
  if (startp)
    *startp = delim ? ++i : i;

  for (passc = 0; c = string[i]; i++)
    {
      if (passc)
	{
	  passc = 0;
	  continue;
	}
      if (c == '\\')
	{
	  passc++;
	  continue;
	}
      if (c == delim)
	break;
    }

  if (delim && string[i] != delim)
    {
      builtin_error (_("no closing `%c' in %s"), delim, string);
      return -1;
    }

  return i;
}

int
bind_keyseq_to_unix_command (line)
     char *line;
{
  Keymap kmap;
  char *kseq, *value;
  int i, kstart;

  if (cmd_xmap == 0)
    init_unix_command_map ();

  kmap = rl_get_keymap ();

  /* We duplicate some of the work done by rl_parse_and_bind here, but
     this code only has to handle `"keyseq": ["]command["]' and can
     generate an error for anything else. */
  i = isolate_sequence (line, 0, 1, &kstart);
  if (i < 0)
    return -1;

  /* Create the key sequence string to pass to rl_generic_bind */
  kseq = substring (line, kstart, i);

  for ( ; line[i] && line[i] != ':'; i++)
    ;
  if (line[i] != ':')
    {
      builtin_error (_("%s: missing colon separator"), line);
      return -1;
    }

  i = isolate_sequence (line, i + 1, 0, &kstart);
  if (i < 0)
    return -1;

  /* Create the value string containing the command to execute. */
  value = substring (line, kstart, i);

  /* Save the command to execute and the key sequence in the CMD_XMAP */
  rl_generic_bind (ISMACR, kseq, value, cmd_xmap);

  /* and bind the key sequence in the current keymap to a function that
     understands how to execute from CMD_XMAP */
  rl_bind_keyseq_in_map (kseq, bash_execute_unix_command, kmap);
  
  return 0;
}

/* Used by the programmable completion code.  Complete TEXT as a filename,
   but return only directories as matches.  Dequotes the filename before
   attempting to find matches. */
char **
bash_directory_completion_matches (text)
     const char *text;
{
  char **m1;
  char *dfn;
  int qc;

  qc = rl_dispatching ? rl_completion_quote_character : 0;  
  dfn = bash_dequote_filename ((char *)text, qc);
  m1 = rl_completion_matches (dfn, rl_filename_completion_function);
  free (dfn);

  if (m1 == 0 || m1[0] == 0)
    return m1;
  /* We don't bother recomputing the lcd of the matches, because it will just
     get thrown away by the programmable completion code and recomputed
     later. */
  (void)bash_ignore_filenames (m1);
  return m1;
}

char *
bash_dequote_text (text)
     const char *text;
{
  char *dtxt;
  int qc;

  qc = (text[0] == '"' || text[0] == '\'') ? text[0] : 0;
  dtxt = bash_dequote_filename ((char *)text, qc);
  return (dtxt);
}
#endif /* READLINE */
