/* execute_cmd.c -- Execute a COMMAND structure. */

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

#if !defined (__GNUC__) && !defined (HAVE_ALLOCA_H) && defined (_AIX)
  #pragma alloca
#endif /* _AIX && RISC6000 && !__GNUC__ */

#include <stdio.h>
#include "chartypes.h"
#include "bashtypes.h"
#if !defined (_MINIX) && defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif
#include "filecntl.h"
#include "posixstat.h"
#include <signal.h>
#ifndef _MINIX
#  include <sys/param.h>
#endif

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "posixtime.h"

#if defined (HAVE_SYS_RESOURCE_H) && !defined (RLIMTYPE)
#  include <sys/resource.h>
#endif

#if defined (HAVE_SYS_TIMES_H) && defined (HAVE_TIMES)
#  include <sys/times.h>
#endif

#include <errno.h>

#if !defined (errno)
extern int errno;
#endif

#define NEED_FPURGE_DECL

#include "bashansi.h"
#include "bashintl.h"

#include "memalloc.h"
#include "shell.h"
#include <y.tab.h>	/* use <...> so we pick it up from the build directory */
#include "flags.h"
#include "builtins.h"
#include "hashlib.h"
#include "jobs.h"
#include "execute_cmd.h"
#include "findcmd.h"
#include "redir.h"
#include "trap.h"
#include "pathexp.h"
#include "hashcmd.h"

#if defined (COND_COMMAND)
#  include "test.h"
#endif

#include "builtins/common.h"
#include "builtins/builtext.h"	/* list of builtins */

#include <glob/strmatch.h>
#include <tilde/tilde.h>

#if defined (BUFFERED_INPUT)
#  include "input.h"
#endif

#if defined (ALIAS)
#  include "alias.h"
#endif

#if defined (HISTORY)
#  include "bashhist.h"
#endif

extern int posixly_correct;
extern int expand_aliases;
extern int autocd;
extern int breaking, continuing, loop_level;
extern int parse_and_execute_level, running_trap, sourcelevel;
extern int command_string_index, line_number;
extern int dot_found_in_search;
extern int already_making_children;
extern int tempenv_assign_error;
extern char *the_printed_command, *shell_name;
extern pid_t last_command_subst_pid;
extern sh_builtin_func_t *last_shell_builtin, *this_shell_builtin;
extern char **subshell_argv, **subshell_envp;
extern int subshell_argc;
#if 0
extern char *glob_argv_flags;
#endif

extern int close __P((int));

/* Static functions defined and used in this file. */
static void close_pipes __P((int, int));
static void do_piping __P((int, int));
static void bind_lastarg __P((char *));
static int shell_control_structure __P((enum command_type));
static void cleanup_redirects __P((REDIRECT *));

#if defined (JOB_CONTROL)
static int restore_signal_mask __P((sigset_t *));
#endif

static void async_redirect_stdin __P((void));

static int builtin_status __P((int));

static int execute_for_command __P((FOR_COM *));
#if defined (SELECT_COMMAND)
static int print_index_and_element __P((int, int, WORD_LIST *));
static void indent __P((int, int));
static void print_select_list __P((WORD_LIST *, int, int, int));
static char *select_query __P((WORD_LIST *, int, char *, int));
static int execute_select_command __P((SELECT_COM *));
#endif
#if defined (DPAREN_ARITHMETIC)
static int execute_arith_command __P((ARITH_COM *));
#endif
#if defined (COND_COMMAND)
static int execute_cond_node __P((COND_COM *));
static int execute_cond_command __P((COND_COM *));
#endif
#if defined (COMMAND_TIMING)
static int mkfmt __P((char *, int, int, time_t, int));
static void print_formatted_time __P((FILE *, char *,
				      time_t, int, time_t, int,
				      time_t, int, int));
static int time_command __P((COMMAND *, int, int, int, struct fd_bitmap *));
#endif
#if defined (ARITH_FOR_COMMAND)
static intmax_t eval_arith_for_expr __P((WORD_LIST *, int *));
static int execute_arith_for_command __P((ARITH_FOR_COM *));
#endif
static int execute_case_command __P((CASE_COM *));
static int execute_while_command __P((WHILE_COM *));
static int execute_until_command __P((WHILE_COM *));
static int execute_while_or_until __P((WHILE_COM *, int));
static int execute_if_command __P((IF_COM *));
static int execute_null_command __P((REDIRECT *, int, int, int));
static void fix_assignment_words __P((WORD_LIST *));
static int execute_simple_command __P((SIMPLE_COM *, int, int, int, struct fd_bitmap *));
static int execute_builtin __P((sh_builtin_func_t *, WORD_LIST *, int, int));
static int execute_function __P((SHELL_VAR *, WORD_LIST *, int, struct fd_bitmap *, int, int));
static int execute_builtin_or_function __P((WORD_LIST *, sh_builtin_func_t *,
					    SHELL_VAR *,
					    REDIRECT *, struct fd_bitmap *, int));
static void execute_subshell_builtin_or_function __P((WORD_LIST *, REDIRECT *,
						      sh_builtin_func_t *,
						      SHELL_VAR *,
						      int, int, int,
						      struct fd_bitmap *,
						      int));
static void execute_disk_command __P((WORD_LIST *, REDIRECT *, char *,
				      int, int, int, struct fd_bitmap *, int));

static char *getinterp __P((char *, int, int *));
static void initialize_subshell __P((void));
static int execute_in_subshell __P((COMMAND *, int, int, int, struct fd_bitmap *));
#if defined (COPROCESS_SUPPORT)
static int execute_coproc __P((COMMAND *, int, int, struct fd_bitmap *));
#endif

static int execute_pipeline __P((COMMAND *, int, int, int, struct fd_bitmap *));

static int execute_connection __P((COMMAND *, int, int, int, struct fd_bitmap *));

static int execute_intern_function __P((WORD_DESC *, COMMAND *));

/* Set to 1 if fd 0 was the subject of redirection to a subshell.  Global
   so that reader_loop can set it to zero before executing a command. */
int stdin_redir;

/* The name of the command that is currently being executed.
   `test' needs this, for example. */
char *this_command_name;

/* The printed representation of the currently-executing command (same as
   the_printed_command), except when a trap is being executed.  Useful for
   a debugger to know where exactly the program is currently executing. */
char *the_printed_command_except_trap;

/* For catching RETURN in a function. */
int return_catch_flag;
int return_catch_value;
procenv_t return_catch;

/* The value returned by the last synchronous command. */
int last_command_exit_value;

/* Whether or not the last command (corresponding to last_command_exit_value)
   was terminated by a signal, and, if so, which one. */
int last_command_exit_signal;

/* The list of redirections to perform which will undo the redirections
   that I made in the shell. */
REDIRECT *redirection_undo_list = (REDIRECT *)NULL;

/* The list of redirections to perform which will undo the internal
   redirections performed by the `exec' builtin.  These are redirections
   that must be undone even when exec discards redirection_undo_list. */
REDIRECT *exec_redirection_undo_list = (REDIRECT *)NULL;

/* When greater than zero, value is the `level' of builtins we are
   currently executing (e.g. `eval echo a' would have it set to 2). */
int executing_builtin = 0;

/* Non-zero if we are executing a command list (a;b;c, etc.) */
int executing_list = 0;

/* Non-zero if failing commands in a command substitution should not exit the
   shell even if -e is set.  Used to pass the CMD_IGNORE_RETURN flag down to
   commands run in command substitutions by parse_and_execute. */
int comsub_ignore_return = 0;

/* Non-zero if we have just forked and are currently running in a subshell
   environment. */
int subshell_environment;

/* Count of nested subshells, like SHLVL.  Available via $BASH_SUBSHELL */
int subshell_level = 0;

/* Currently-executing shell function. */
SHELL_VAR *this_shell_function;

/* If non-zero, matches in case and [[ ... ]] are case-insensitive */
int match_ignore_case = 0;

struct stat SB;		/* used for debugging */

static int special_builtin_failed;

static COMMAND *currently_executing_command;

/* The line number that the currently executing function starts on. */
static int function_line_number;

/* XXX - set to 1 if we're running the DEBUG trap and we want to show the line
   number containing the function name.  Used by executing_line_number to
   report the correct line number.  Kind of a hack. */
static int showing_function_line;

static int line_number_for_err_trap;

/* A sort of function nesting level counter */
static int funcnest = 0;
int funcnest_max = 0;		/* XXX - for bash-4.2 */

struct fd_bitmap *current_fds_to_close = (struct fd_bitmap *)NULL;

#define FD_BITMAP_DEFAULT_SIZE 32

/* Functions to allocate and deallocate the structures used to pass
   information from the shell to its children about file descriptors
   to close. */
struct fd_bitmap *
new_fd_bitmap (size)
     int size;
{
  struct fd_bitmap *ret;

  ret = (struct fd_bitmap *)xmalloc (sizeof (struct fd_bitmap));

  ret->size = size;

  if (size)
    {
      ret->bitmap = (char *)xmalloc (size);
      memset (ret->bitmap, '\0', size);
    }
  else
    ret->bitmap = (char *)NULL;
  return (ret);
}

void
dispose_fd_bitmap (fdbp)
     struct fd_bitmap *fdbp;
{
  FREE (fdbp->bitmap);
  free (fdbp);
}

void
close_fd_bitmap (fdbp)
     struct fd_bitmap *fdbp;
{
  register int i;

  if (fdbp)
    {
      for (i = 0; i < fdbp->size; i++)
	if (fdbp->bitmap[i])
	  {
	    close (i);
	    fdbp->bitmap[i] = 0;
	  }
    }
}

/* Return the line number of the currently executing command. */
int
executing_line_number ()
{
  if (executing && showing_function_line == 0 &&
      (variable_context == 0 || interactive_shell == 0) &&
      currently_executing_command)
    {
#if defined (COND_COMMAND)
      if (currently_executing_command->type == cm_cond)
	return currently_executing_command->value.Cond->line;
#endif
#if defined (DPAREN_ARITHMETIC)
      else if (currently_executing_command->type == cm_arith)
	return currently_executing_command->value.Arith->line;
#endif
#if defined (ARITH_FOR_COMMAND)
      else if (currently_executing_command->type == cm_arith_for)
	return currently_executing_command->value.ArithFor->line;
#endif

	return line_number;
    }
  else
    return line_number;
}

/* Execute the command passed in COMMAND.  COMMAND is exactly what
   read_command () places into GLOBAL_COMMAND.  See "command.h" for the
   details of the command structure.

   EXECUTION_SUCCESS or EXECUTION_FAILURE are the only possible
   return values.  Executing a command with nothing in it returns
   EXECUTION_SUCCESS. */
int
execute_command (command)
     COMMAND *command;
{
  struct fd_bitmap *bitmap;
  int result;

  current_fds_to_close = (struct fd_bitmap *)NULL;
  bitmap = new_fd_bitmap (FD_BITMAP_DEFAULT_SIZE);
  begin_unwind_frame ("execute-command");
  add_unwind_protect (dispose_fd_bitmap, (char *)bitmap);

  /* Just do the command, but not asynchronously. */
  result = execute_command_internal (command, 0, NO_PIPE, NO_PIPE, bitmap);

  dispose_fd_bitmap (bitmap);
  discard_unwind_frame ("execute-command");

#if defined (PROCESS_SUBSTITUTION)
  /* don't unlink fifos if we're in a shell function; wait until the function
     returns. */
  if (variable_context == 0)
    unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */

  QUIT;
  return (result);
}

/* Return 1 if TYPE is a shell control structure type. */
static int
shell_control_structure (type)
     enum command_type type;
{
  switch (type)
    {
#if defined (ARITH_FOR_COMMAND)
    case cm_arith_for:
#endif
#if defined (SELECT_COMMAND)
    case cm_select:
#endif
#if defined (DPAREN_ARITHMETIC)
    case cm_arith:
#endif
#if defined (COND_COMMAND)
    case cm_cond:
#endif
    case cm_case:
    case cm_while:
    case cm_until:
    case cm_if:
    case cm_for:
    case cm_group:
    case cm_function_def:
      return (1);

    default:
      return (0);
    }
}

/* A function to use to unwind_protect the redirection undo list
   for loops. */
static void
cleanup_redirects (list)
     REDIRECT *list;
{
  do_redirections (list, RX_ACTIVE);
  dispose_redirects (list);
}

#if 0
/* Function to unwind_protect the redirections for functions and builtins. */
static void
cleanup_func_redirects (list)
     REDIRECT *list;
{
  do_redirections (list, RX_ACTIVE);
}
#endif

void
dispose_exec_redirects ()
{
  if (exec_redirection_undo_list)
    {
      dispose_redirects (exec_redirection_undo_list);
      exec_redirection_undo_list = (REDIRECT *)NULL;
    }
}

#if defined (JOB_CONTROL)
/* A function to restore the signal mask to its proper value when the shell
   is interrupted or errors occur while creating a pipeline. */
static int
restore_signal_mask (set)
     sigset_t *set;
{
  return (sigprocmask (SIG_SETMASK, set, (sigset_t *)NULL));
}
#endif /* JOB_CONTROL */

#ifdef DEBUG
/* A debugging function that can be called from gdb, for instance. */
void
open_files ()
{
  register int i;
  int f, fd_table_size;

  fd_table_size = getdtablesize ();

  fprintf (stderr, "pid %ld open files:", (long)getpid ());
  for (i = 3; i < fd_table_size; i++)
    {
      if ((f = fcntl (i, F_GETFD, 0)) != -1)
	fprintf (stderr, " %d (%s)", i, f ? "close" : "open");
    }
  fprintf (stderr, "\n");
}
#endif

static void
async_redirect_stdin ()
{
  int fd;

  fd = open ("/dev/null", O_RDONLY);
  if (fd > 0)
    {
      dup2 (fd, 0);
      close (fd);
    }
  else if (fd < 0)
    internal_error (_("cannot redirect standard input from /dev/null: %s"), strerror (errno));
}

#define DESCRIBE_PID(pid) do { if (interactive) describe_pid (pid); } while (0)

/* Execute the command passed in COMMAND, perhaps doing it asynchrounously.
   COMMAND is exactly what read_command () places into GLOBAL_COMMAND.
   ASYNCHROUNOUS, if non-zero, says to do this command in the background.
   PIPE_IN and PIPE_OUT are file descriptors saying where input comes
   from and where it goes.  They can have the value of NO_PIPE, which means
   I/O is stdin/stdout.
   FDS_TO_CLOSE is a list of file descriptors to close once the child has
   been forked.  This list often contains the unusable sides of pipes, etc.

   EXECUTION_SUCCESS or EXECUTION_FAILURE are the only possible
   return values.  Executing a command with nothing in it returns
   EXECUTION_SUCCESS. */
int
execute_command_internal (command, asynchronous, pipe_in, pipe_out,
			  fds_to_close)
     COMMAND *command;
     int asynchronous;
     int pipe_in, pipe_out;
     struct fd_bitmap *fds_to_close;
{
  int exec_result, user_subshell, invert, ignore_return, was_error_trap;
  REDIRECT *my_undo_list, *exec_undo_list;
  volatile int last_pid;
  volatile int save_line_number;

#if 0
  if (command == 0 || breaking || continuing || read_but_dont_execute)
    return (EXECUTION_SUCCESS);
#else
  if (breaking || continuing)
    return (last_command_exit_value);
  if (command == 0 || read_but_dont_execute)
    return (EXECUTION_SUCCESS);
#endif

  QUIT;
  run_pending_traps ();

#if 0
  if (running_trap == 0)
#endif
    currently_executing_command = command;

  invert = (command->flags & CMD_INVERT_RETURN) != 0;

  /* If we're inverting the return value and `set -e' has been executed,
     we don't want a failing command to inadvertently cause the shell
     to exit. */
  if (exit_immediately_on_error && invert)	/* XXX */
    command->flags |= CMD_IGNORE_RETURN;	/* XXX */

  exec_result = EXECUTION_SUCCESS;

  /* If a command was being explicitly run in a subshell, or if it is
     a shell control-structure, and it has a pipe, then we do the command
     in a subshell. */
  if (command->type == cm_subshell && (command->flags & CMD_NO_FORK))
    return (execute_in_subshell (command, asynchronous, pipe_in, pipe_out, fds_to_close));

#if defined (COPROCESS_SUPPORT)
  if (command->type == cm_coproc)
    return (execute_coproc (command, pipe_in, pipe_out, fds_to_close));
#endif

  user_subshell = command->type == cm_subshell || ((command->flags & CMD_WANT_SUBSHELL) != 0);

  if (command->type == cm_subshell ||
      (command->flags & (CMD_WANT_SUBSHELL|CMD_FORCE_SUBSHELL)) ||
      (shell_control_structure (command->type) &&
       (pipe_out != NO_PIPE || pipe_in != NO_PIPE || asynchronous)))
    {
      pid_t paren_pid;

      /* Fork a subshell, turn off the subshell bit, turn off job
	 control and call execute_command () on the command again. */
      line_number_for_err_trap = line_number;
      paren_pid = make_child (savestring (make_command_string (command)),
			      asynchronous);
      if (paren_pid == 0)
	exit (execute_in_subshell (command, asynchronous, pipe_in, pipe_out, fds_to_close));
	/* NOTREACHED */
      else
	{
	  close_pipes (pipe_in, pipe_out);

#if defined (PROCESS_SUBSTITUTION) && defined (HAVE_DEV_FD)
	  unlink_fifo_list ();
#endif
	  /* If we are part of a pipeline, and not the end of the pipeline,
	     then we should simply return and let the last command in the
	     pipe be waited for.  If we are not in a pipeline, or are the
	     last command in the pipeline, then we wait for the subshell
	     and return its exit status as usual. */
	  if (pipe_out != NO_PIPE)
	    return (EXECUTION_SUCCESS);

	  stop_pipeline (asynchronous, (COMMAND *)NULL);

	  if (asynchronous == 0)
	    {
	      was_error_trap = signal_is_trapped (ERROR_TRAP) && signal_is_ignored (ERROR_TRAP) == 0;
	      invert = (command->flags & CMD_INVERT_RETURN) != 0;
	      ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

	      last_command_exit_value = wait_for (paren_pid);

	      /* If we have to, invert the return value. */
	      if (invert)
		exec_result = ((last_command_exit_value == EXECUTION_SUCCESS)
				? EXECUTION_FAILURE
				: EXECUTION_SUCCESS);
	      else
		exec_result = last_command_exit_value;

	      if (user_subshell && was_error_trap && ignore_return == 0 && invert == 0 && exec_result != EXECUTION_SUCCESS)
		{
		  last_command_exit_value = exec_result;
		  save_line_number = line_number;
		  line_number = line_number_for_err_trap;
		  run_error_trap ();
		  line_number = save_line_number;
		}

	      if (user_subshell && ignore_return == 0 && invert == 0 && exit_immediately_on_error && exec_result != EXECUTION_SUCCESS)
		{
		  last_command_exit_value = exec_result;
		  run_pending_traps ();
		  jump_to_top_level (ERREXIT);
		}

	      return (last_command_exit_value = exec_result);
	    }
	  else
	    {
	      DESCRIBE_PID (paren_pid);

	      run_pending_traps ();

	      return (EXECUTION_SUCCESS);
	    }
	}
    }

#if defined (COMMAND_TIMING)
  if (command->flags & CMD_TIME_PIPELINE)
    {
      if (asynchronous)
	{
	  command->flags |= CMD_FORCE_SUBSHELL;
	  exec_result = execute_command_internal (command, 1, pipe_in, pipe_out, fds_to_close);
	}
      else
	{
	  exec_result = time_command (command, asynchronous, pipe_in, pipe_out, fds_to_close);
#if 0
	  if (running_trap == 0)
#endif
	    currently_executing_command = (COMMAND *)NULL;
	}
      return (exec_result);
    }
#endif /* COMMAND_TIMING */

  if (shell_control_structure (command->type) && command->redirects)
    stdin_redir = stdin_redirects (command->redirects);

  /* Handle WHILE FOR CASE etc. with redirections.  (Also '&' input
     redirection.)  */
  if (do_redirections (command->redirects, RX_ACTIVE|RX_UNDOABLE) != 0)
    {
      cleanup_redirects (redirection_undo_list);
      redirection_undo_list = (REDIRECT *)NULL;
      dispose_exec_redirects ();
      return (last_command_exit_value = EXECUTION_FAILURE);
    }

  if (redirection_undo_list)
    {
      my_undo_list = (REDIRECT *)copy_redirects (redirection_undo_list);
      dispose_redirects (redirection_undo_list);
      redirection_undo_list = (REDIRECT *)NULL;
    }
  else
    my_undo_list = (REDIRECT *)NULL;

  if (exec_redirection_undo_list)
    {
      exec_undo_list = (REDIRECT *)copy_redirects (exec_redirection_undo_list);
      dispose_redirects (exec_redirection_undo_list);
      exec_redirection_undo_list = (REDIRECT *)NULL;
    }
  else
    exec_undo_list = (REDIRECT *)NULL;

  if (my_undo_list || exec_undo_list)
    begin_unwind_frame ("loop_redirections");

  if (my_undo_list)
    add_unwind_protect ((Function *)cleanup_redirects, my_undo_list);

  if (exec_undo_list)
    add_unwind_protect ((Function *)dispose_redirects, exec_undo_list);

  ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

  QUIT;

  switch (command->type)
    {
    case cm_simple:
      {
	save_line_number = line_number;
	/* We can't rely on variables retaining their values across a
	   call to execute_simple_command if a longjmp occurs as the
	   result of a `return' builtin.  This is true for sure with gcc. */
#if defined (RECYCLES_PIDS)
	last_made_pid = NO_PID;
#endif
	last_pid = last_made_pid;
	was_error_trap = signal_is_trapped (ERROR_TRAP) && signal_is_ignored (ERROR_TRAP) == 0;

	if (ignore_return && command->value.Simple)
	  command->value.Simple->flags |= CMD_IGNORE_RETURN;
	if (command->flags & CMD_STDIN_REDIR)
	  command->value.Simple->flags |= CMD_STDIN_REDIR;

	line_number_for_err_trap = line_number = command->value.Simple->line;
	exec_result =
	  execute_simple_command (command->value.Simple, pipe_in, pipe_out,
				  asynchronous, fds_to_close);
	line_number = save_line_number;

	/* The temporary environment should be used for only the simple
	   command immediately following its definition. */
	dispose_used_env_vars ();

#if (defined (ultrix) && defined (mips)) || defined (C_ALLOCA)
	/* Reclaim memory allocated with alloca () on machines which
	   may be using the alloca emulation code. */
	(void) alloca (0);
#endif /* (ultrix && mips) || C_ALLOCA */

	/* If we forked to do the command, then we must wait_for ()
	   the child. */

	/* XXX - this is something to watch out for if there are problems
	   when the shell is compiled without job control. */
	if (already_making_children && pipe_out == NO_PIPE &&
	    last_made_pid != last_pid)
	  {
	    stop_pipeline (asynchronous, (COMMAND *)NULL);

	    if (asynchronous)
	      {
		DESCRIBE_PID (last_made_pid);
	      }
	    else
#if !defined (JOB_CONTROL)
	      /* Do not wait for asynchronous processes started from
		 startup files. */
	    if (last_made_pid != last_asynchronous_pid)
#endif
	    /* When executing a shell function that executes other
	       commands, this causes the last simple command in
	       the function to be waited for twice.  This also causes
	       subshells forked to execute builtin commands (e.g., in
	       pipelines) to be waited for twice. */
	      exec_result = wait_for (last_made_pid);
	  }
      }

      /* 2009/02/13 -- pipeline failure is processed elsewhere.  This handles
	 only the failure of a simple command. */
      if (was_error_trap && ignore_return == 0 && invert == 0 && pipe_in == NO_PIPE && pipe_out == NO_PIPE && exec_result != EXECUTION_SUCCESS)
	{
	  last_command_exit_value = exec_result;
	  line_number = line_number_for_err_trap;
	  run_error_trap ();
	  line_number = save_line_number;
	}

      if (ignore_return == 0 && invert == 0 &&
	  ((posixly_correct && interactive == 0 && special_builtin_failed) ||
	   (exit_immediately_on_error && pipe_in == NO_PIPE && pipe_out == NO_PIPE && exec_result != EXECUTION_SUCCESS)))
	{
	  last_command_exit_value = exec_result;
	  run_pending_traps ();
	  jump_to_top_level (ERREXIT);
	}

      break;

    case cm_for:
      if (ignore_return)
	command->value.For->flags |= CMD_IGNORE_RETURN;
      exec_result = execute_for_command (command->value.For);
      break;

#if defined (ARITH_FOR_COMMAND)
    case cm_arith_for:
      if (ignore_return)
	command->value.ArithFor->flags |= CMD_IGNORE_RETURN;
      exec_result = execute_arith_for_command (command->value.ArithFor);
      break;
#endif

#if defined (SELECT_COMMAND)
    case cm_select:
      if (ignore_return)
	command->value.Select->flags |= CMD_IGNORE_RETURN;
      exec_result = execute_select_command (command->value.Select);
      break;
#endif

    case cm_case:
      if (ignore_return)
	command->value.Case->flags |= CMD_IGNORE_RETURN;
      exec_result = execute_case_command (command->value.Case);
      break;

    case cm_while:
      if (ignore_return)
	command->value.While->flags |= CMD_IGNORE_RETURN;
      exec_result = execute_while_command (command->value.While);
      break;

    case cm_until:
      if (ignore_return)
	command->value.While->flags |= CMD_IGNORE_RETURN;
      exec_result = execute_until_command (command->value.While);
      break;

    case cm_if:
      if (ignore_return)
	command->value.If->flags |= CMD_IGNORE_RETURN;
      exec_result = execute_if_command (command->value.If);
      break;

    case cm_group:

      /* This code can be executed from either of two paths: an explicit
	 '{}' command, or via a function call.  If we are executed via a
	 function call, we have already taken care of the function being
	 executed in the background (down there in execute_simple_command ()),
	 and this command should *not* be marked as asynchronous.  If we
	 are executing a regular '{}' group command, and asynchronous == 1,
	 we must want to execute the whole command in the background, so we
	 need a subshell, and we want the stuff executed in that subshell
	 (this group command) to be executed in the foreground of that
	 subshell (i.e. there will not be *another* subshell forked).

	 What we do is to force a subshell if asynchronous, and then call
	 execute_command_internal again with asynchronous still set to 1,
	 but with the original group command, so the printed command will
	 look right.

	 The code above that handles forking off subshells will note that
	 both subshell and async are on, and turn off async in the child
	 after forking the subshell (but leave async set in the parent, so
	 the normal call to describe_pid is made).  This turning off
	 async is *crucial*; if it is not done, this will fall into an
	 infinite loop of executions through this spot in subshell after
	 subshell until the process limit is exhausted. */

      if (asynchronous)
	{
	  command->flags |= CMD_FORCE_SUBSHELL;
	  exec_result =
	    execute_command_internal (command, 1, pipe_in, pipe_out,
				      fds_to_close);
	}
      else
	{
	  if (ignore_return && command->value.Group->command)
	    command->value.Group->command->flags |= CMD_IGNORE_RETURN;
	  exec_result =
	    execute_command_internal (command->value.Group->command,
				      asynchronous, pipe_in, pipe_out,
				      fds_to_close);
	}
      break;

    case cm_connection:
      exec_result = execute_connection (command, asynchronous,
					pipe_in, pipe_out, fds_to_close);
      break;

#if defined (DPAREN_ARITHMETIC)
    case cm_arith:
      was_error_trap = signal_is_trapped (ERROR_TRAP) && signal_is_ignored (ERROR_TRAP) == 0;
      if (ignore_return)
	command->value.Arith->flags |= CMD_IGNORE_RETURN;
      line_number_for_err_trap = save_line_number = line_number;
      exec_result = execute_arith_command (command->value.Arith);
      line_number = save_line_number;

      if (was_error_trap && ignore_return == 0 && invert == 0 && exec_result != EXECUTION_SUCCESS)
	{
	  last_command_exit_value = exec_result;
	  save_line_number = line_number;
	  line_number = line_number_for_err_trap;
	  run_error_trap ();
	  line_number = save_line_number;
	}

      if (ignore_return == 0 && invert == 0 && exit_immediately_on_error && exec_result != EXECUTION_SUCCESS)
	{
	  last_command_exit_value = exec_result;
	  run_pending_traps ();
	  jump_to_top_level (ERREXIT);
	}

      break;
#endif

#if defined (COND_COMMAND)
    case cm_cond:
      was_error_trap = signal_is_trapped (ERROR_TRAP) && signal_is_ignored (ERROR_TRAP) == 0;
      if (ignore_return)
	command->value.Cond->flags |= CMD_IGNORE_RETURN;

      line_number_for_err_trap = save_line_number = line_number;
      exec_result = execute_cond_command (command->value.Cond);
      line_number = save_line_number;

      if (was_error_trap && ignore_return == 0 && invert == 0 && exec_result != EXECUTION_SUCCESS)
	{
	  last_command_exit_value = exec_result;
	  save_line_number = line_number;
	  line_number = line_number_for_err_trap;
	  run_error_trap ();
	  line_number = save_line_number;
	}

      if (ignore_return == 0 && invert == 0 && exit_immediately_on_error && exec_result != EXECUTION_SUCCESS)
	{
	  last_command_exit_value = exec_result;
	  run_pending_traps ();
	  jump_to_top_level (ERREXIT);
	}

      break;
#endif
    
    case cm_function_def:
      exec_result = execute_intern_function (command->value.Function_def->name,
					     command->value.Function_def->command);
      break;

    default:
      command_error ("execute_command", CMDERR_BADTYPE, command->type, 0);
    }

  if (my_undo_list)
    {
      do_redirections (my_undo_list, RX_ACTIVE);
      dispose_redirects (my_undo_list);
    }

  if (exec_undo_list)
    dispose_redirects (exec_undo_list);

  if (my_undo_list || exec_undo_list)
    discard_unwind_frame ("loop_redirections");

  /* Invert the return value if we have to */
  if (invert)
    exec_result = (exec_result == EXECUTION_SUCCESS)
		    ? EXECUTION_FAILURE
		    : EXECUTION_SUCCESS;

#if defined (DPAREN_ARITHMETIC) || defined (COND_COMMAND)
  /* This is where we set PIPESTATUS from the exit status of the appropriate
     compound commands (the ones that look enough like simple commands to
     cause confusion).  We might be able to optimize by not doing this if
     subshell_environment != 0. */
  switch (command->type)
    {
#  if defined (DPAREN_ARITHMETIC)
    case cm_arith:
#  endif
#  if defined (COND_COMMAND)
    case cm_cond:
#  endif
      set_pipestatus_from_exit (exec_result);
      break;
    }
#endif

  last_command_exit_value = exec_result;
  run_pending_traps ();
#if 0
  if (running_trap == 0)
#endif
    currently_executing_command = (COMMAND *)NULL;
  return (last_command_exit_value);
}

#if defined (COMMAND_TIMING)

#if defined (HAVE_GETRUSAGE) && defined (HAVE_GETTIMEOFDAY)
extern struct timeval *difftimeval __P((struct timeval *, struct timeval *, struct timeval *));
extern struct timeval *addtimeval __P((struct timeval *, struct timeval *, struct timeval *));
extern int timeval_to_cpu __P((struct timeval *, struct timeval *, struct timeval *));
#endif

#define POSIX_TIMEFORMAT "real %2R\nuser %2U\nsys %2S"
#define BASH_TIMEFORMAT  "\nreal\t%3lR\nuser\t%3lU\nsys\t%3lS"

static const int precs[] = { 0, 100, 10, 1 };

/* Expand one `%'-prefixed escape sequence from a time format string. */
static int
mkfmt (buf, prec, lng, sec, sec_fraction)
     char *buf;
     int prec, lng;
     time_t sec;
     int sec_fraction;
{
  time_t min;
  char abuf[INT_STRLEN_BOUND(time_t) + 1];
  int ind, aind;

  ind = 0;
  abuf[sizeof(abuf) - 1] = '\0';

  /* If LNG is non-zero, we want to decompose SEC into minutes and seconds. */
  if (lng)
    {
      min = sec / 60;
      sec %= 60;
      aind = sizeof(abuf) - 2;
      do
	abuf[aind--] = (min % 10) + '0';
      while (min /= 10);
      aind++;
      while (abuf[aind])
	buf[ind++] = abuf[aind++];
      buf[ind++] = 'm';
    }

  /* Now add the seconds. */
  aind = sizeof (abuf) - 2;
  do
    abuf[aind--] = (sec % 10) + '0';
  while (sec /= 10);
  aind++;
  while (abuf[aind])
    buf[ind++] = abuf[aind++];

  /* We want to add a decimal point and PREC places after it if PREC is
     nonzero.  PREC is not greater than 3.  SEC_FRACTION is between 0
     and 999. */
  if (prec != 0)
    {
      buf[ind++] = '.';
      for (aind = 1; aind <= prec; aind++)
	{
	  buf[ind++] = (sec_fraction / precs[aind]) + '0';
	  sec_fraction %= precs[aind];
	}
    }

  if (lng)
    buf[ind++] = 's';
  buf[ind] = '\0';

  return (ind);
}

/* Interpret the format string FORMAT, interpolating the following escape
   sequences:
		%[prec][l][RUS]

   where the optional `prec' is a precision, meaning the number of
   characters after the decimal point, the optional `l' means to format
   using minutes and seconds (MMmNN[.FF]s), like the `times' builtin',
   and the last character is one of
   
		R	number of seconds of `real' time
		U	number of seconds of `user' time
		S	number of seconds of `system' time

   An occurrence of `%%' in the format string is translated to a `%'.  The
   result is printed to FP, a pointer to a FILE.  The other variables are
   the seconds and thousandths of a second of real, user, and system time,
   resectively. */
static void
print_formatted_time (fp, format, rs, rsf, us, usf, ss, ssf, cpu)
     FILE *fp;
     char *format;
     time_t rs;
     int rsf;
     time_t us;
     int usf;
     time_t ss;
     int ssf, cpu;
{
  int prec, lng, len;
  char *str, *s, ts[INT_STRLEN_BOUND (time_t) + sizeof ("mSS.FFFF")];
  time_t sum;
  int sum_frac;
  int sindex, ssize;

  len = strlen (format);
  ssize = (len + 64) - (len % 64);
  str = (char *)xmalloc (ssize);
  sindex = 0;

  for (s = format; *s; s++)
    {
      if (*s != '%' || s[1] == '\0')
	{
	  RESIZE_MALLOCED_BUFFER (str, sindex, 1, ssize, 64);
	  str[sindex++] = *s;
	}
      else if (s[1] == '%')
	{
	  s++;
	  RESIZE_MALLOCED_BUFFER (str, sindex, 1, ssize, 64);
	  str[sindex++] = *s;
	}
      else if (s[1] == 'P')
	{
	  s++;
#if 0
	  /* clamp CPU usage at 100% */
	  if (cpu > 10000)
	    cpu = 10000;
#endif
	  sum = cpu / 100;
	  sum_frac = (cpu % 100) * 10;
	  len = mkfmt (ts, 2, 0, sum, sum_frac);
	  RESIZE_MALLOCED_BUFFER (str, sindex, len, ssize, 64);
	  strcpy (str + sindex, ts);
	  sindex += len;
	}
      else
	{
	  prec = 3;	/* default is three places past the decimal point. */
	  lng = 0;	/* default is to not use minutes or append `s' */
	  s++;
	  if (DIGIT (*s))		/* `precision' */
	    {
	      prec = *s++ - '0';
	      if (prec > 3) prec = 3;
	    }
	  if (*s == 'l')		/* `length extender' */
	    {
	      lng = 1;
	      s++;
	    }
	  if (*s == 'R' || *s == 'E')
	    len = mkfmt (ts, prec, lng, rs, rsf);
	  else if (*s == 'U')
	    len = mkfmt (ts, prec, lng, us, usf);
	  else if (*s == 'S')
	    len = mkfmt (ts, prec, lng, ss, ssf);
	  else
	    {
	      internal_error (_("TIMEFORMAT: `%c': invalid format character"), *s);
	      free (str);
	      return;
	    }
	  RESIZE_MALLOCED_BUFFER (str, sindex, len, ssize, 64);
	  strcpy (str + sindex, ts);
	  sindex += len;
	}
    }

  str[sindex] = '\0';
  fprintf (fp, "%s\n", str);
  fflush (fp);

  free (str);
}

static int
time_command (command, asynchronous, pipe_in, pipe_out, fds_to_close)
     COMMAND *command;
     int asynchronous, pipe_in, pipe_out;
     struct fd_bitmap *fds_to_close;
{
  int rv, posix_time, old_flags;
  time_t rs, us, ss;
  int rsf, usf, ssf;
  int cpu;
  char *time_format;

#if defined (HAVE_GETRUSAGE) && defined (HAVE_GETTIMEOFDAY)
  struct timeval real, user, sys;
  struct timeval before, after;
#  if defined (HAVE_STRUCT_TIMEZONE)
  struct timezone dtz;				/* posix doesn't define this */
#  endif
  struct rusage selfb, selfa, kidsb, kidsa;	/* a = after, b = before */
#else
#  if defined (HAVE_TIMES)
  clock_t tbefore, tafter, real, user, sys;
  struct tms before, after;
#  endif
#endif

#if defined (HAVE_GETRUSAGE) && defined (HAVE_GETTIMEOFDAY)
#  if defined (HAVE_STRUCT_TIMEZONE)
  gettimeofday (&before, &dtz);
#  else
  gettimeofday (&before, (void *)NULL);
#  endif /* !HAVE_STRUCT_TIMEZONE */
  getrusage (RUSAGE_SELF, &selfb);
  getrusage (RUSAGE_CHILDREN, &kidsb);
#else
#  if defined (HAVE_TIMES)
  tbefore = times (&before);
#  endif
#endif

  posix_time = (command->flags & CMD_TIME_POSIX);

  old_flags = command->flags;
  command->flags &= ~(CMD_TIME_PIPELINE|CMD_TIME_POSIX);
  rv = execute_command_internal (command, asynchronous, pipe_in, pipe_out, fds_to_close);
  command->flags = old_flags;

  rs = us = ss = 0;
  rsf = usf = ssf = cpu = 0;

#if defined (HAVE_GETRUSAGE) && defined (HAVE_GETTIMEOFDAY)
#  if defined (HAVE_STRUCT_TIMEZONE)
  gettimeofday (&after, &dtz);
#  else
  gettimeofday (&after, (void *)NULL);
#  endif /* !HAVE_STRUCT_TIMEZONE */
  getrusage (RUSAGE_SELF, &selfa);
  getrusage (RUSAGE_CHILDREN, &kidsa);

  difftimeval (&real, &before, &after);
  timeval_to_secs (&real, &rs, &rsf);

  addtimeval (&user, difftimeval(&after, &selfb.ru_utime, &selfa.ru_utime),
		     difftimeval(&before, &kidsb.ru_utime, &kidsa.ru_utime));
  timeval_to_secs (&user, &us, &usf);

  addtimeval (&sys, difftimeval(&after, &selfb.ru_stime, &selfa.ru_stime),
		    difftimeval(&before, &kidsb.ru_stime, &kidsa.ru_stime));
  timeval_to_secs (&sys, &ss, &ssf);

  cpu = timeval_to_cpu (&real, &user, &sys);
#else
#  if defined (HAVE_TIMES)
  tafter = times (&after);

  real = tafter - tbefore;
  clock_t_to_secs (real, &rs, &rsf);

  user = (after.tms_utime - before.tms_utime) + (after.tms_cutime - before.tms_cutime);
  clock_t_to_secs (user, &us, &usf);

  sys = (after.tms_stime - before.tms_stime) + (after.tms_cstime - before.tms_cstime);
  clock_t_to_secs (sys, &ss, &ssf);

  cpu = (real == 0) ? 0 : ((user + sys) * 10000) / real;

#  else
  rs = us = ss = 0;
  rsf = usf = ssf = cpu = 0;
#  endif
#endif

  if (posix_time)
    time_format = POSIX_TIMEFORMAT;
  else if ((time_format = get_string_value ("TIMEFORMAT")) == 0)
    time_format = BASH_TIMEFORMAT;

  if (time_format && *time_format)
    print_formatted_time (stderr, time_format, rs, rsf, us, usf, ss, ssf, cpu);

  return rv;
}
#endif /* COMMAND_TIMING */

/* Execute a command that's supposed to be in a subshell.  This must be
   called after make_child and we must be running in the child process.
   The caller will return or exit() immediately with the value this returns. */
static int
execute_in_subshell (command, asynchronous, pipe_in, pipe_out, fds_to_close)
     COMMAND *command;
     int asynchronous;
     int pipe_in, pipe_out;
     struct fd_bitmap *fds_to_close;
{
  int user_subshell, return_code, function_value, should_redir_stdin, invert;
  int ois, user_coproc;
  int result;
  COMMAND *tcom;

  USE_VAR(user_subshell);
  USE_VAR(user_coproc);
  USE_VAR(invert);
  USE_VAR(tcom);
  USE_VAR(asynchronous);

  subshell_level++;
  should_redir_stdin = (asynchronous && (command->flags & CMD_STDIN_REDIR) &&
			  pipe_in == NO_PIPE &&
			  stdin_redirects (command->redirects) == 0);

  invert = (command->flags & CMD_INVERT_RETURN) != 0;
  user_subshell = command->type == cm_subshell || ((command->flags & CMD_WANT_SUBSHELL) != 0);
  user_coproc = command->type == cm_coproc;

  command->flags &= ~(CMD_FORCE_SUBSHELL | CMD_WANT_SUBSHELL | CMD_INVERT_RETURN);

  /* If a command is asynchronous in a subshell (like ( foo ) & or
     the special case of an asynchronous GROUP command where the
     the subshell bit is turned on down in case cm_group: below),
     turn off `asynchronous', so that two subshells aren't spawned.
     XXX - asynchronous used to be set to 0 in this block, but that
     means that setup_async_signals was never run.  Now it's set to
     0 after subshell_environment is set appropriately and setup_async_signals
     is run.

     This seems semantically correct to me.  For example,
     ( foo ) & seems to say ``do the command `foo' in a subshell
     environment, but don't wait for that subshell to finish'',
     and "{ foo ; bar ; } &" seems to me to be like functions or
     builtins in the background, which executed in a subshell
     environment.  I just don't see the need to fork two subshells. */

  /* Don't fork again, we are already in a subshell.  A `doubly
     async' shell is not interactive, however. */
  if (asynchronous)
    {
#if defined (JOB_CONTROL)
      /* If a construct like ( exec xxx yyy ) & is given while job
	 control is active, we want to prevent exec from putting the
	 subshell back into the original process group, carefully
	 undoing all the work we just did in make_child. */
      original_pgrp = -1;
#endif /* JOB_CONTROL */
      ois = interactive_shell;
      interactive_shell = 0;
      /* This test is to prevent alias expansion by interactive shells that
	 run `(command) &' but to allow scripts that have enabled alias
	 expansion with `shopt -s expand_alias' to continue to expand
	 aliases. */
      if (ois != interactive_shell)
	expand_aliases = 0;
    }

  /* Subshells are neither login nor interactive. */
  login_shell = interactive = 0;

  if (user_subshell)
    subshell_environment = SUBSHELL_PAREN;
  else
    {
      subshell_environment = 0;			/* XXX */
      if (asynchronous)
	subshell_environment |= SUBSHELL_ASYNC;
      if (pipe_in != NO_PIPE || pipe_out != NO_PIPE)
	subshell_environment |= SUBSHELL_PIPE;
      if (user_coproc)
	subshell_environment |= SUBSHELL_COPROC;
    }

  reset_terminating_signals ();		/* in sig.c */
  /* Cancel traps, in trap.c. */
  restore_original_signals ();

  /* Make sure restore_original_signals doesn't undo the work done by
     make_child to ensure that asynchronous children are immune to SIGINT
     and SIGQUIT.  Turn off asynchronous to make sure more subshells are
     not spawned. */
  if (asynchronous)
    {
      setup_async_signals ();
      asynchronous = 0;
    }

#if defined (JOB_CONTROL)
  set_sigchld_handler ();
#endif /* JOB_CONTROL */

  set_sigint_handler ();

#if defined (JOB_CONTROL)
  /* Delete all traces that there were any jobs running.  This is
     only for subshells. */
  without_job_control ();
#endif /* JOB_CONTROL */

  if (fds_to_close)
    close_fd_bitmap (fds_to_close);

  do_piping (pipe_in, pipe_out);

#if defined (COPROCESS_SUPPORT)
  coproc_closeall ();
#endif

  /* If this is a user subshell, set a flag if stdin was redirected.
     This is used later to decide whether to redirect fd 0 to
     /dev/null for async commands in the subshell.  This adds more
     sh compatibility, but I'm not sure it's the right thing to do. */
  if (user_subshell)
    {
      stdin_redir = stdin_redirects (command->redirects);
      restore_default_signal (0);
    }

  /* If this is an asynchronous command (command &), we want to
     redirect the standard input from /dev/null in the absence of
     any specific redirection involving stdin. */
  if (should_redir_stdin && stdin_redir == 0)
    async_redirect_stdin ();

  /* Do redirections, then dispose of them before recursive call. */
  if (command->redirects)
    {
      if (do_redirections (command->redirects, RX_ACTIVE) != 0)
	exit (invert ? EXECUTION_SUCCESS : EXECUTION_FAILURE);

      dispose_redirects (command->redirects);
      command->redirects = (REDIRECT *)NULL;
    }

  if (command->type == cm_subshell)
    tcom = command->value.Subshell->command;
  else if (user_coproc)
    tcom = command->value.Coproc->command;
  else
    tcom = command;

  if (command->flags & CMD_TIME_PIPELINE)
    tcom->flags |= CMD_TIME_PIPELINE;
  if (command->flags & CMD_TIME_POSIX)
    tcom->flags |= CMD_TIME_POSIX;
  
  /* Make sure the subshell inherits any CMD_IGNORE_RETURN flag. */
  if ((command->flags & CMD_IGNORE_RETURN) && tcom != command)
    tcom->flags |= CMD_IGNORE_RETURN;

  /* If this is a simple command, tell execute_disk_command that it
     might be able to get away without forking and simply exec.
     This means things like ( sleep 10 ) will only cause one fork.
     If we're timing the command or inverting its return value, however,
     we cannot do this optimization. */
  if ((user_subshell || user_coproc) && (tcom->type == cm_simple || tcom->type == cm_subshell) &&
      ((tcom->flags & CMD_TIME_PIPELINE) == 0) &&
      ((tcom->flags & CMD_INVERT_RETURN) == 0))
    {
      tcom->flags |= CMD_NO_FORK;
      if (tcom->type == cm_simple)
	tcom->value.Simple->flags |= CMD_NO_FORK;
    }

  invert = (tcom->flags & CMD_INVERT_RETURN) != 0;
  tcom->flags &= ~CMD_INVERT_RETURN;

  result = setjmp (top_level);

  /* If we're inside a function while executing this subshell, we
     need to handle a possible `return'. */
  function_value = 0;
  if (return_catch_flag)
    function_value = setjmp (return_catch);

  /* If we're going to exit the shell, we don't want to invert the return
     status. */
  if (result == EXITPROG)
    invert = 0, return_code = last_command_exit_value;
  else if (result)
    return_code = EXECUTION_FAILURE;
  else if (function_value)
    return_code = return_catch_value;
  else
    return_code = execute_command_internal (tcom, asynchronous, NO_PIPE, NO_PIPE, fds_to_close);

  /* If we are asked to, invert the return value. */
  if (invert)
    return_code = (return_code == EXECUTION_SUCCESS) ? EXECUTION_FAILURE
						     : EXECUTION_SUCCESS;

  /* If we were explicitly placed in a subshell with (), we need
     to do the `shell cleanup' things, such as running traps[0]. */
  if (user_subshell && signal_is_trapped (0))
    {
      last_command_exit_value = return_code;
      return_code = run_exit_trap ();
    }

  subshell_level--;
  return (return_code);
  /* NOTREACHED */
}

#if defined (COPROCESS_SUPPORT)
#define COPROC_MAX	16

typedef struct cpelement
  {
    struct cpelement *next;
    struct coproc *coproc;
  }
cpelement_t;
    
typedef struct cplist
  {
    struct cpelement *head;
    struct cpelement *tail;
    int ncoproc;
  }
cplist_t;

static struct cpelement *cpe_alloc __P((struct coproc *));
static void cpe_dispose __P((struct cpelement *));
static struct cpelement *cpl_add __P((struct coproc *));
static struct cpelement *cpl_delete __P((pid_t));
static void cpl_reap __P((void));
static void cpl_flush __P((void));
static struct cpelement *cpl_search __P((pid_t));
static struct cpelement *cpl_searchbyname __P((char *));
static void cpl_prune __P((void));

Coproc sh_coproc = { 0, NO_PID, -1, -1, 0, 0 };

cplist_t coproc_list = {0, 0, 0};

/* Functions to manage the list of coprocs */

static struct cpelement *
cpe_alloc (cp)
     Coproc *cp;
{
  struct cpelement *cpe;

  cpe = (struct cpelement *)xmalloc (sizeof (struct cpelement));
  cpe->coproc = cp;
  cpe->next = (struct cpelement *)0;
  return cpe;
}

static void
cpe_dispose (cpe)
      struct cpelement *cpe;
{
  free (cpe);
}

static struct cpelement *
cpl_add (cp)
     Coproc *cp;
{
  struct cpelement *cpe;

  cpe = cpe_alloc (cp);

  if (coproc_list.head == 0)
    {
      coproc_list.head = coproc_list.tail = cpe;
      coproc_list.ncoproc = 0;			/* just to make sure */
    }
  else
    {
      coproc_list.tail->next = cpe;
      coproc_list.tail = cpe;
    }
  coproc_list.ncoproc++;

  return cpe;
}

static struct cpelement *
cpl_delete (pid)
     pid_t pid;
{
  struct cpelement *prev, *p;

  for (prev = p = coproc_list.head; p; prev = p, p = p->next)
    if (p->coproc->c_pid == pid)
      {
        prev->next = p->next;	/* remove from list */
        break;
      }

  if (p == 0)
    return 0;		/* not found */

#if defined (DEBUG)
  itrace("cpl_delete: deleting %d", pid);
#endif

  /* Housekeeping in the border cases. */
  if (p == coproc_list.head)
    coproc_list.head = coproc_list.head->next;
  else if (p == coproc_list.tail)
    coproc_list.tail = prev;

  coproc_list.ncoproc--;
  if (coproc_list.ncoproc == 0)
    coproc_list.head = coproc_list.tail = 0;
  else if (coproc_list.ncoproc == 1)
    coproc_list.tail = coproc_list.head;		/* just to make sure */

  return (p);
}

static void
cpl_reap ()
{
  struct cpelement *prev, *p;

  for (prev = p = coproc_list.head; p; prev = p, p = p->next)
    if (p->coproc->c_flags & COPROC_DEAD)
      {
        prev->next = p->next;	/* remove from list */

	/* Housekeeping in the border cases. */
	if (p == coproc_list.head)
	  coproc_list.head = coproc_list.head->next;
	else if (p == coproc_list.tail)
	  coproc_list.tail = prev;

	coproc_list.ncoproc--;
	if (coproc_list.ncoproc == 0)
	  coproc_list.head = coproc_list.tail = 0;
	else if (coproc_list.ncoproc == 1)
	  coproc_list.tail = coproc_list.head;		/* just to make sure */

#if defined (DEBUG)
	itrace("cpl_reap: deleting %d", p->coproc->c_pid);
#endif

	coproc_dispose (p->coproc);
	cpe_dispose (p);
      }
}

/* Clear out the list of saved statuses */
static void
cpl_flush ()
{
  struct cpelement *cpe, *p;

  for (cpe = coproc_list.head; cpe; )
    {
      p = cpe;
      cpe = cpe->next;

      coproc_dispose (p->coproc);
      cpe_dispose (p);
    }

  coproc_list.head = coproc_list.tail = 0;
  coproc_list.ncoproc = 0;
}

/* Search for PID in the list of coprocs; return the cpelement struct if
   found.  If not found, return NULL. */
static struct cpelement *
cpl_search (pid)
     pid_t pid;
{
  struct cpelement *cp;

  for (cp = coproc_list.head ; cp; cp = cp->next)
    if (cp->coproc->c_pid == pid)
      return cp;
  return (struct cpelement *)NULL;
}

/* Search for the coproc named NAME in the list of coprocs; return the
   cpelement struct if found.  If not found, return NULL. */
static struct cpelement *
cpl_searchbyname (name)
     char *name;
{
  struct cpelement *cp;

  for (cp = coproc_list.head ; cp; cp = cp->next)
    if (STREQ (cp->coproc->c_name, name))
      return cp;
  return (struct cpelement *)NULL;
}

#if 0
static void
cpl_prune ()
{
  struct cpelement *cp;

  while (coproc_list.head && coproc_list.ncoproc > COPROC_MAX)
    {
      cp = coproc_list.head;
      coproc_list.head = coproc_list.head->next;
      coproc_dispose (cp->coproc);
      cpe_dispose (cp);
      coproc_list.ncoproc--;
    }
}
#endif

/* These currently use a single global "shell coproc" but are written in a
   way to not preclude additional coprocs later (using the list management
   package above). */

struct coproc *
getcoprocbypid (pid)
     pid_t pid;
{
  return (pid == sh_coproc.c_pid ? &sh_coproc : 0);
}

struct coproc *
getcoprocbyname (name)
     const char *name;
{
  return ((sh_coproc.c_name && STREQ (sh_coproc.c_name, name)) ? &sh_coproc : 0);
}

void
coproc_init (cp)
     struct coproc *cp;
{
  cp->c_name = 0;
  cp->c_pid = NO_PID;
  cp->c_rfd = cp->c_wfd = -1;
  cp->c_rsave = cp->c_wsave = -1;
  cp->c_flags = cp->c_status = 0;  
}

struct coproc *
coproc_alloc (name, pid)
     char *name;
     pid_t pid;
{
  struct coproc *cp;

  cp = &sh_coproc;		/* XXX */
  coproc_init (cp);

  cp->c_name = savestring (name);
  cp->c_pid = pid;

  return (cp);
}

void
coproc_dispose (cp)
     struct coproc *cp;
{
  if (cp == 0)
    return;

  coproc_unsetvars (cp);
  FREE (cp->c_name);
  coproc_close (cp);
  coproc_init (cp);
}

/* Placeholder for now. */
void
coproc_flush ()
{
  coproc_dispose (&sh_coproc);
}

void
coproc_close (cp)
     struct coproc *cp;
{
  if (cp->c_rfd >= 0)
    {
      close (cp->c_rfd);
      cp->c_rfd = -1;
    }
  if (cp->c_wfd >= 0)
    {
      close (cp->c_wfd);
      cp->c_wfd = -1;
    }
  cp->c_rsave = cp->c_wsave = -1;
}

void
coproc_closeall ()
{
  coproc_close (&sh_coproc);
}

void
coproc_reap ()
{
  struct coproc *cp;

  cp = &sh_coproc;
  if (cp && (cp->c_flags & COPROC_DEAD))
    coproc_dispose (cp);
}

void
coproc_rclose (cp, fd)
     struct coproc *cp;
     int fd;
{
  if (cp->c_rfd >= 0 && cp->c_rfd == fd)
    {
      close (cp->c_rfd);
      cp->c_rfd = -1;
    }
}

void
coproc_wclose (cp, fd)
     struct coproc *cp;
     int fd;
{
  if (cp->c_wfd >= 0 && cp->c_wfd == fd)
    {
      close (cp->c_wfd);
      cp->c_wfd = -1;
    }
}

void
coproc_checkfd (cp, fd)
     struct coproc *cp;
     int fd;
{
  int update;

  update = 0;
  if (cp->c_rfd >= 0 && cp->c_rfd == fd)
    update = cp->c_rfd = -1;
  if (cp->c_wfd >= 0 && cp->c_wfd == fd)
    update = cp->c_wfd = -1;
  if (update)
    coproc_setvars (cp);
}

void
coproc_fdchk (fd)
     int fd;
{
  coproc_checkfd (&sh_coproc, fd);
}

void
coproc_fdclose (cp, fd)
     struct coproc *cp;
     int fd;
{
  coproc_rclose (cp, fd);
  coproc_wclose (cp, fd);
  coproc_setvars (cp);
}

void
coproc_fdsave (cp)
     struct coproc *cp;
{
  cp->c_rsave = cp->c_rfd;
  cp->c_wsave = cp->c_wfd;
}

void
coproc_fdrestore (cp)
     struct coproc *cp;
{
  cp->c_rfd = cp->c_rsave;
  cp->c_wfd = cp->c_wsave;
}

void
coproc_pidchk (pid, status)
     pid_t pid;
{
  struct coproc *cp;

  cp = getcoprocbypid (pid);
#if 0
  if (cp)
    itrace("coproc_pidchk: pid %d has died", pid);
#endif
  if (cp)
    {
      cp->c_status = status;
      cp->c_flags |= COPROC_DEAD;
      cp->c_flags &= ~COPROC_RUNNING;
#if 0
      coproc_dispose (cp);
#endif
    }
}

void
coproc_setvars (cp)
     struct coproc *cp;
{
  SHELL_VAR *v;
  char *namevar, *t;
  int l;
#if defined (ARRAY_VARS)
  arrayind_t ind;
#endif

  if (cp->c_name == 0)
    return;

  l = strlen (cp->c_name);
  namevar = xmalloc (l + 16);

#if defined (ARRAY_VARS)
  v = find_variable (cp->c_name);
  if (v == 0)
    v = make_new_array_variable (cp->c_name);
  if (array_p (v) == 0)
    v = convert_var_to_array (v);

  t = itos (cp->c_rfd);
  ind = 0;
  v = bind_array_variable (cp->c_name, ind, t, 0);
  free (t);

  t = itos (cp->c_wfd);
  ind = 1;
  bind_array_variable (cp->c_name, ind, t, 0);
  free (t);
#else
  sprintf (namevar, "%s_READ", cp->c_name);
  t = itos (cp->c_rfd);
  bind_variable (namevar, t, 0);
  free (t);
  sprintf (namevar, "%s_WRITE", cp->c_name);
  t = itos (cp->c_wfd);
  bind_variable (namevar, t, 0);
  free (t);
#endif

  sprintf (namevar, "%s_PID", cp->c_name);
  t = itos (cp->c_pid);
  bind_variable (namevar, t, 0);
  free (t);

  free (namevar);
}

void
coproc_unsetvars (cp)
     struct coproc *cp;
{
  int l;
  char *namevar;

  if (cp->c_name == 0)
    return;

  l = strlen (cp->c_name);
  namevar = xmalloc (l + 16);

  sprintf (namevar, "%s_PID", cp->c_name);
  unbind_variable (namevar);  

#if defined (ARRAY_VARS)
  unbind_variable (cp->c_name);
#else
  sprintf (namevar, "%s_READ", cp->c_name);
  unbind_variable (namevar);
  sprintf (namevar, "%s_WRITE", cp->c_name);
  unbind_variable (namevar);
#endif  

  free (namevar);
}

static int
execute_coproc (command, pipe_in, pipe_out, fds_to_close)
     COMMAND *command;
     int pipe_in, pipe_out;
     struct fd_bitmap *fds_to_close;
{
  int rpipe[2], wpipe[2], estat;
  pid_t coproc_pid;
  Coproc *cp;
  char *tcmd;

  /* XXX -- will require changes to handle multiple coprocs */
  if (sh_coproc.c_pid != NO_PID)
    {
#if 0
      internal_error ("execute_coproc: coproc [%d:%s] already exists", sh_coproc.c_pid, sh_coproc.c_name);
      return (last_command_exit_value = EXECUTION_FAILURE);
#else
      internal_warning ("execute_coproc: coproc [%d:%s] still exists", sh_coproc.c_pid, sh_coproc.c_name);
#endif
    }
  coproc_init (&sh_coproc);

  command_string_index = 0;
  tcmd = make_command_string (command);

  sh_openpipe ((int *)&rpipe);	/* 0 = parent read, 1 = child write */
  sh_openpipe ((int *)&wpipe); /* 0 = child read, 1 = parent write */

  coproc_pid = make_child (savestring (tcmd), 1);
  if (coproc_pid == 0)
    {
      close (rpipe[0]);
      close (wpipe[1]);

      estat = execute_in_subshell (command, 1, wpipe[0], rpipe[1], fds_to_close);

      fflush (stdout);
      fflush (stderr);

      exit (estat);
    }

  close (rpipe[1]);
  close (wpipe[0]);

  cp = coproc_alloc (command->value.Coproc->name, coproc_pid);
  cp->c_rfd = rpipe[0];
  cp->c_wfd = wpipe[1];

  SET_CLOSE_ON_EXEC (cp->c_rfd);
  SET_CLOSE_ON_EXEC (cp->c_wfd);

  coproc_setvars (cp);

#if 0
  itrace ("execute_coproc: [%d] %s", coproc_pid, the_printed_command);
#endif

  close_pipes (pipe_in, pipe_out);
#if defined (PROCESS_SUBSTITUTION) && defined (HAVE_DEV_FD)
  unlink_fifo_list ();
#endif
  stop_pipeline (1, (COMMAND *)NULL);
  DESCRIBE_PID (coproc_pid);
  run_pending_traps ();

  return (EXECUTION_SUCCESS);
}
#endif

static int
execute_pipeline (command, asynchronous, pipe_in, pipe_out, fds_to_close)
     COMMAND *command;
     int asynchronous, pipe_in, pipe_out;
     struct fd_bitmap *fds_to_close;
{
  int prev, fildes[2], new_bitmap_size, dummyfd, ignore_return, exec_result;
  COMMAND *cmd;
  struct fd_bitmap *fd_bitmap;

#if defined (JOB_CONTROL)
  sigset_t set, oset;
  BLOCK_CHILD (set, oset);
#endif /* JOB_CONTROL */

  ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

  prev = pipe_in;
  cmd = command;

  while (cmd && cmd->type == cm_connection &&
	 cmd->value.Connection && cmd->value.Connection->connector == '|')
    {
      /* Make a pipeline between the two commands. */
      if (pipe (fildes) < 0)
	{
	  sys_error (_("pipe error"));
#if defined (JOB_CONTROL)
	  terminate_current_pipeline ();
	  kill_current_pipeline ();
	  UNBLOCK_CHILD (oset);
#endif /* JOB_CONTROL */
	  last_command_exit_value = EXECUTION_FAILURE;
	  /* The unwind-protects installed below will take care
	     of closing all of the open file descriptors. */
	  throw_to_top_level ();
	  return (EXECUTION_FAILURE);	/* XXX */
	}

      /* Here is a problem: with the new file close-on-exec
	 code, the read end of the pipe (fildes[0]) stays open
	 in the first process, so that process will never get a
	 SIGPIPE.  There is no way to signal the first process
	 that it should close fildes[0] after forking, so it
	 remains open.  No SIGPIPE is ever sent because there
	 is still a file descriptor open for reading connected
	 to the pipe.  We take care of that here.  This passes
	 around a bitmap of file descriptors that must be
	 closed after making a child process in execute_simple_command. */

      /* We need fd_bitmap to be at least as big as fildes[0].
	 If fildes[0] is less than fds_to_close->size, then
	 use fds_to_close->size. */
      new_bitmap_size = (fildes[0] < fds_to_close->size)
				? fds_to_close->size
				: fildes[0] + 8;

      fd_bitmap = new_fd_bitmap (new_bitmap_size);

      /* Now copy the old information into the new bitmap. */
      xbcopy ((char *)fds_to_close->bitmap, (char *)fd_bitmap->bitmap, fds_to_close->size);

      /* And mark the pipe file descriptors to be closed. */
      fd_bitmap->bitmap[fildes[0]] = 1;

      /* In case there are pipe or out-of-processes errors, we
	 want all these file descriptors to be closed when
	 unwind-protects are run, and the storage used for the
	 bitmaps freed up. */
      begin_unwind_frame ("pipe-file-descriptors");
      add_unwind_protect (dispose_fd_bitmap, fd_bitmap);
      add_unwind_protect (close_fd_bitmap, fd_bitmap);
      if (prev >= 0)
	add_unwind_protect (close, prev);
      dummyfd = fildes[1];
      add_unwind_protect (close, dummyfd);

#if defined (JOB_CONTROL)
      add_unwind_protect (restore_signal_mask, &oset);
#endif /* JOB_CONTROL */

      if (ignore_return && cmd->value.Connection->first)
	cmd->value.Connection->first->flags |= CMD_IGNORE_RETURN;
      execute_command_internal (cmd->value.Connection->first, asynchronous,
				prev, fildes[1], fd_bitmap);

      if (prev >= 0)
	close (prev);

      prev = fildes[0];
      close (fildes[1]);

      dispose_fd_bitmap (fd_bitmap);
      discard_unwind_frame ("pipe-file-descriptors");

      cmd = cmd->value.Connection->second;
    }

  /* Now execute the rightmost command in the pipeline.  */
  if (ignore_return && cmd)
    cmd->flags |= CMD_IGNORE_RETURN;
  exec_result = execute_command_internal (cmd, asynchronous, prev, pipe_out, fds_to_close);

  if (prev >= 0)
    close (prev);

#if defined (JOB_CONTROL)
  UNBLOCK_CHILD (oset);
#endif

  QUIT;
  return (exec_result);
}

static int
execute_connection (command, asynchronous, pipe_in, pipe_out, fds_to_close)
     COMMAND *command;
     int asynchronous, pipe_in, pipe_out;
     struct fd_bitmap *fds_to_close;
{
  REDIRECT *rp;
  COMMAND *tc, *second;
  int ignore_return, exec_result, was_error_trap, invert;
  volatile int save_line_number;

  ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

  switch (command->value.Connection->connector)
    {
    /* Do the first command asynchronously. */
    case '&':
      tc = command->value.Connection->first;
      if (tc == 0)
	return (EXECUTION_SUCCESS);

      rp = tc->redirects;

      if (ignore_return)
	tc->flags |= CMD_IGNORE_RETURN;
      tc->flags |= CMD_AMPERSAND;

      /* If this shell was compiled without job control support,
	 if we are currently in a subshell via `( xxx )', or if job
	 control is not active then the standard input for an
	 asynchronous command is forced to /dev/null. */
#if defined (JOB_CONTROL)
      if ((subshell_environment || !job_control) && !stdin_redir)
#else
      if (!stdin_redir)
#endif /* JOB_CONTROL */
	tc->flags |= CMD_STDIN_REDIR;

      exec_result = execute_command_internal (tc, 1, pipe_in, pipe_out, fds_to_close);
      QUIT;

      if (tc->flags & CMD_STDIN_REDIR)
	tc->flags &= ~CMD_STDIN_REDIR;

      second = command->value.Connection->second;
      if (second)
	{
	  if (ignore_return)
	    second->flags |= CMD_IGNORE_RETURN;

	  exec_result = execute_command_internal (second, asynchronous, pipe_in, pipe_out, fds_to_close);
	}

      break;

    /* Just call execute command on both sides. */
    case ';':
      if (ignore_return)
	{
	  if (command->value.Connection->first)
	    command->value.Connection->first->flags |= CMD_IGNORE_RETURN;
	  if (command->value.Connection->second)
	    command->value.Connection->second->flags |= CMD_IGNORE_RETURN;
	}
      executing_list++;
      QUIT;
      execute_command (command->value.Connection->first);
      QUIT;
      exec_result = execute_command_internal (command->value.Connection->second,
				      asynchronous, pipe_in, pipe_out,
				      fds_to_close);
      executing_list--;
      break;

    case '|':
      was_error_trap = signal_is_trapped (ERROR_TRAP) && signal_is_ignored (ERROR_TRAP) == 0;
      invert = (command->flags & CMD_INVERT_RETURN) != 0;
      ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

      line_number_for_err_trap = line_number;
      exec_result = execute_pipeline (command, asynchronous, pipe_in, pipe_out, fds_to_close);

      if (was_error_trap && ignore_return == 0 && invert == 0 && exec_result != EXECUTION_SUCCESS)
	{
	  last_command_exit_value = exec_result;
	  save_line_number = line_number;
	  line_number = line_number_for_err_trap;
	  run_error_trap ();
	  line_number = save_line_number;
	}

      if (ignore_return == 0 && invert == 0 && exit_immediately_on_error && exec_result != EXECUTION_SUCCESS)
	{
	  last_command_exit_value = exec_result;
	  run_pending_traps ();
	  jump_to_top_level (ERREXIT);
	}

      break;

    case AND_AND:
    case OR_OR:
      if (asynchronous)
	{
	  /* If we have something like `a && b &' or `a || b &', run the
	     && or || stuff in a subshell.  Force a subshell and just call
	     execute_command_internal again.  Leave asynchronous on
	     so that we get a report from the parent shell about the
	     background job. */
	  command->flags |= CMD_FORCE_SUBSHELL;
	  exec_result = execute_command_internal (command, 1, pipe_in, pipe_out, fds_to_close);
	  break;
	}

      /* Execute the first command.  If the result of that is successful
	 and the connector is AND_AND, or the result is not successful
	 and the connector is OR_OR, then execute the second command,
	 otherwise return. */

      executing_list++;
      if (command->value.Connection->first)
	command->value.Connection->first->flags |= CMD_IGNORE_RETURN;

      exec_result = execute_command (command->value.Connection->first);
      QUIT;
      if (((command->value.Connection->connector == AND_AND) &&
	   (exec_result == EXECUTION_SUCCESS)) ||
	  ((command->value.Connection->connector == OR_OR) &&
	   (exec_result != EXECUTION_SUCCESS)))
	{
	  if (ignore_return && command->value.Connection->second)
	    command->value.Connection->second->flags |= CMD_IGNORE_RETURN;

	  exec_result = execute_command (command->value.Connection->second);
	}
      executing_list--;
      break;

    default:
      command_error ("execute_connection", CMDERR_BADCONN, command->value.Connection->connector, 0);
      jump_to_top_level (DISCARD);
      exec_result = EXECUTION_FAILURE;
    }

  return exec_result;
}

#define REAP() \
  do \
    { \
      if (!interactive_shell) \
	reap_dead_jobs (); \
    } \
  while (0)

/* Execute a FOR command.  The syntax is: FOR word_desc IN word_list;
   DO command; DONE */
static int
execute_for_command (for_command)
     FOR_COM *for_command;
{
  register WORD_LIST *releaser, *list;
  SHELL_VAR *v;
  char *identifier;
  int retval, save_line_number;
#if 0
  SHELL_VAR *old_value = (SHELL_VAR *)NULL; /* Remember the old value of x. */
#endif

  save_line_number = line_number;
  if (check_identifier (for_command->name, 1) == 0)
    {
      if (posixly_correct && interactive_shell == 0)
	{
	  last_command_exit_value = EX_BADUSAGE;
	  jump_to_top_level (ERREXIT);
	}
      return (EXECUTION_FAILURE);
    }

  loop_level++;
  identifier = for_command->name->word;

  list = releaser = expand_words_no_vars (for_command->map_list);

  begin_unwind_frame ("for");
  add_unwind_protect (dispose_words, releaser);

#if 0
  if (lexical_scoping)
    {
      old_value = copy_variable (find_variable (identifier));
      if (old_value)
	add_unwind_protect (dispose_variable, old_value);
    }
#endif

  if (for_command->flags & CMD_IGNORE_RETURN)
    for_command->action->flags |= CMD_IGNORE_RETURN;

  for (retval = EXECUTION_SUCCESS; list; list = list->next)
    {
      QUIT;

      line_number = for_command->line;

      /* Remember what this command looks like, for debugger. */
      command_string_index = 0;
      print_for_command_head (for_command);

      if (echo_command_at_execute)
	xtrace_print_for_command_head (for_command);

      /* Save this command unless it's a trap command and we're not running
	 a debug trap. */
#if 0
      if (signal_in_progress (DEBUG_TRAP) == 0 && (this_command_name == 0 || (STREQ (this_command_name, "trap") == 0)))
#else
      if (signal_in_progress (DEBUG_TRAP) == 0 && running_trap == 0)
#endif
	{
	  FREE (the_printed_command_except_trap);
	  the_printed_command_except_trap = savestring (the_printed_command);
	}

      retval = run_debug_trap ();
#if defined (DEBUGGER)
      /* In debugging mode, if the DEBUG trap returns a non-zero status, we
	 skip the command. */
      if (debugging_mode && retval != EXECUTION_SUCCESS)
        continue;
#endif

      this_command_name = (char *)NULL;
      v = bind_variable (identifier, list->word->word, 0);
      if (readonly_p (v) || noassign_p (v))
	{
	  line_number = save_line_number;
	  if (readonly_p (v) && interactive_shell == 0 && posixly_correct)
	    {
	      last_command_exit_value = EXECUTION_FAILURE;
	      jump_to_top_level (FORCE_EOF);
	    }
	  else
	    {
	      dispose_words (releaser);
	      discard_unwind_frame ("for");
	      loop_level--;
	      return (EXECUTION_FAILURE);
	    }
	}
      retval = execute_command (for_command->action);
      REAP ();
      QUIT;

      if (breaking)
	{
	  breaking--;
	  break;
	}

      if (continuing)
	{
	  continuing--;
	  if (continuing)
	    break;
	}
    }

  loop_level--;
  line_number = save_line_number;

#if 0
  if (lexical_scoping)
    {
      if (!old_value)
        unbind_variable (identifier);
      else
	{
	  SHELL_VAR *new_value;

	  new_value = bind_variable (identifier, value_cell(old_value), 0);
	  new_value->attributes = old_value->attributes;
	  dispose_variable (old_value);
	}
    }
#endif

  dispose_words (releaser);
  discard_unwind_frame ("for");
  return (retval);
}

#if defined (ARITH_FOR_COMMAND)
/* Execute an arithmetic for command.  The syntax is

	for (( init ; step ; test ))
	do
		body
	done

   The execution should be exactly equivalent to

	eval \(\( init \)\)
	while eval \(\( test \)\) ; do
		body;
		eval \(\( step \)\)
	done
*/
static intmax_t
eval_arith_for_expr (l, okp)
     WORD_LIST *l;
     int *okp;
{
  WORD_LIST *new;
  intmax_t expresult;
  int r;

  new = expand_words_no_vars (l);
  if (new)
    {
      if (echo_command_at_execute)
	xtrace_print_arith_cmd (new);
      this_command_name = "((";		/* )) for expression error messages */

      command_string_index = 0;
      print_arith_command (new);
      if (signal_in_progress (DEBUG_TRAP) == 0)
	{
	  FREE (the_printed_command_except_trap);
	  the_printed_command_except_trap = savestring (the_printed_command);
	}

      r = run_debug_trap ();
      /* In debugging mode, if the DEBUG trap returns a non-zero status, we
	 skip the command. */
#if defined (DEBUGGER)
      if (debugging_mode == 0 || r == EXECUTION_SUCCESS)
	expresult = evalexp (new->word->word, okp);
      else
	{
	  expresult = 0;
	  if (okp)
	    *okp = 1;
	}
#else
      expresult = evalexp (new->word->word, okp);
#endif
      dispose_words (new);
    }
  else
    {
      expresult = 0;
      if (okp)
	*okp = 1;
    }
  return (expresult);
}

static int
execute_arith_for_command (arith_for_command)
     ARITH_FOR_COM *arith_for_command;
{
  intmax_t expresult;
  int expok, body_status, arith_lineno, save_lineno;

  body_status = EXECUTION_SUCCESS;
  loop_level++;
  save_lineno = line_number;

  if (arith_for_command->flags & CMD_IGNORE_RETURN)
    arith_for_command->action->flags |= CMD_IGNORE_RETURN;

  this_command_name = "((";	/* )) for expression error messages */

  /* save the starting line number of the command so we can reset
     line_number before executing each expression -- for $LINENO
     and the DEBUG trap. */
  line_number = arith_lineno = arith_for_command->line;
  if (variable_context && interactive_shell)
    line_number -= function_line_number;

  /* Evaluate the initialization expression. */
  expresult = eval_arith_for_expr (arith_for_command->init, &expok);
  if (expok == 0)
    {
      line_number = save_lineno;
      return (EXECUTION_FAILURE);
    }

  while (1)
    {
      /* Evaluate the test expression. */
      line_number = arith_lineno;
      expresult = eval_arith_for_expr (arith_for_command->test, &expok);
      line_number = save_lineno;

      if (expok == 0)
	{
	  body_status = EXECUTION_FAILURE;
	  break;
	}
      REAP ();
      if (expresult == 0)
	break;

      /* Execute the body of the arithmetic for command. */
      QUIT;
      body_status = execute_command (arith_for_command->action);
      QUIT;

      /* Handle any `break' or `continue' commands executed by the body. */
      if (breaking)
	{
	  breaking--;
	  break;
	}

      if (continuing)
	{
	  continuing--;
	  if (continuing)
	    break;
	}

      /* Evaluate the step expression. */
      line_number = arith_lineno;
      expresult = eval_arith_for_expr (arith_for_command->step, &expok);
      line_number = save_lineno;

      if (expok == 0)
	{
	  body_status = EXECUTION_FAILURE;
	  break;
	}
    }

  loop_level--;
  line_number = save_lineno;

  return (body_status);
}
#endif

#if defined (SELECT_COMMAND)
static int LINES, COLS, tabsize;

#define RP_SPACE ") "
#define RP_SPACE_LEN 2

/* XXX - does not handle numbers > 1000000 at all. */
#define NUMBER_LEN(s) \
((s < 10) ? 1 \
	  : ((s < 100) ? 2 \
		      : ((s < 1000) ? 3 \
				   : ((s < 10000) ? 4 \
						 : ((s < 100000) ? 5 \
								: 6)))))

static int
print_index_and_element (len, ind, list)
      int len, ind;
      WORD_LIST *list;
{
  register WORD_LIST *l;
  register int i;

  if (list == 0)
    return (0);
  for (i = ind, l = list; l && --i; l = l->next)
    ;
  fprintf (stderr, "%*d%s%s", len, ind, RP_SPACE, l->word->word);
  return (STRLEN (l->word->word));
}

static void
indent (from, to)
     int from, to;
{
  while (from < to)
    {
      if ((to / tabsize) > (from / tabsize))
	{
	  putc ('\t', stderr);
	  from += tabsize - from % tabsize;
	}
      else
	{
	  putc (' ', stderr);
	  from++;
	}
    }
}

static void
print_select_list (list, list_len, max_elem_len, indices_len)
     WORD_LIST *list;
     int list_len, max_elem_len, indices_len;
{
  int ind, row, elem_len, pos, cols, rows;
  int first_column_indices_len, other_indices_len;

  if (list == 0)
    {
      putc ('\n', stderr);
      return;
    }

  cols = max_elem_len ? COLS / max_elem_len : 1;
  if (cols == 0)
    cols = 1;
  rows = list_len ? list_len / cols + (list_len % cols != 0) : 1;
  cols = list_len ? list_len / rows + (list_len % rows != 0) : 1;

  if (rows == 1)
    {
      rows = cols;
      cols = 1;
    }

  first_column_indices_len = NUMBER_LEN (rows);
  other_indices_len = indices_len;

  for (row = 0; row < rows; row++)
    {
      ind = row;
      pos = 0;
      while (1)
	{
	  indices_len = (pos == 0) ? first_column_indices_len : other_indices_len;
	  elem_len = print_index_and_element (indices_len, ind + 1, list);
	  elem_len += indices_len + RP_SPACE_LEN;
	  ind += rows;
	  if (ind >= list_len)
	    break;
	  indent (pos + elem_len, pos + max_elem_len);
	  pos += max_elem_len;
	}
      putc ('\n', stderr);
    }
}

/* Print the elements of LIST, one per line, preceded by an index from 1 to
   LIST_LEN.  Then display PROMPT and wait for the user to enter a number.
   If the number is between 1 and LIST_LEN, return that selection.  If EOF
   is read, return a null string.  If a blank line is entered, or an invalid
   number is entered, the loop is executed again. */
static char *
select_query (list, list_len, prompt, print_menu)
     WORD_LIST *list;
     int list_len;
     char *prompt;
     int print_menu;
{
  int max_elem_len, indices_len, len;
  intmax_t reply;
  WORD_LIST *l;
  char *repl_string, *t;

  t = get_string_value ("LINES");
  LINES = (t && *t) ? atoi (t) : 24;
  t = get_string_value ("COLUMNS");
  COLS =  (t && *t) ? atoi (t) : 80;

#if 0
  t = get_string_value ("TABSIZE");
  tabsize = (t && *t) ? atoi (t) : 8;
  if (tabsize <= 0)
    tabsize = 8;
#else
  tabsize = 8;
#endif

  max_elem_len = 0;
  for (l = list; l; l = l->next)
    {
      len = STRLEN (l->word->word);
      if (len > max_elem_len)
	max_elem_len = len;
    }
  indices_len = NUMBER_LEN (list_len);
  max_elem_len += indices_len + RP_SPACE_LEN + 2;

  while (1)
    {
      if (print_menu)
	print_select_list (list, list_len, max_elem_len, indices_len);
      fprintf (stderr, "%s", prompt);
      fflush (stderr);
      QUIT;

      if (read_builtin ((WORD_LIST *)NULL) == EXECUTION_FAILURE)
	{
	  putchar ('\n');
	  return ((char *)NULL);
	}
      repl_string = get_string_value ("REPLY");
      if (*repl_string == 0)
	{
	  print_menu = 1;
	  continue;
	}
      if (legal_number (repl_string, &reply) == 0)
	return "";
      if (reply < 1 || reply > list_len)
	return "";

      for (l = list; l && --reply; l = l->next)
	;
      return (l->word->word);
    }
}

/* Execute a SELECT command.  The syntax is:
   SELECT word IN list DO command_list DONE
   Only `break' or `return' in command_list will terminate
   the command. */
static int
execute_select_command (select_command)
     SELECT_COM *select_command;
{
  WORD_LIST *releaser, *list;
  SHELL_VAR *v;
  char *identifier, *ps3_prompt, *selection;
  int retval, list_len, show_menu, save_line_number;

  if (check_identifier (select_command->name, 1) == 0)
    return (EXECUTION_FAILURE);

  save_line_number = line_number;
  line_number = select_command->line;

  command_string_index = 0;
  print_select_command_head (select_command);

  if (echo_command_at_execute)
    xtrace_print_select_command_head (select_command);

#if 0
  if (signal_in_progress (DEBUG_TRAP) == 0 && (this_command_name == 0 || (STREQ (this_command_name, "trap") == 0)))
#else
  if (signal_in_progress (DEBUG_TRAP) == 0 && running_trap == 0)
#endif
    {
      FREE (the_printed_command_except_trap);
      the_printed_command_except_trap = savestring (the_printed_command);
    }

  retval = run_debug_trap ();
#if defined (DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && retval != EXECUTION_SUCCESS)
    return (EXECUTION_SUCCESS);
#endif

  loop_level++;
  identifier = select_command->name->word;

  /* command and arithmetic substitution, parameter and variable expansion,
     word splitting, pathname expansion, and quote removal. */
  list = releaser = expand_words_no_vars (select_command->map_list);
  list_len = list_length (list);
  if (list == 0 || list_len == 0)
    {
      if (list)
	dispose_words (list);
      line_number = save_line_number;
      return (EXECUTION_SUCCESS);
    }

  begin_unwind_frame ("select");
  add_unwind_protect (dispose_words, releaser);

  if (select_command->flags & CMD_IGNORE_RETURN)
    select_command->action->flags |= CMD_IGNORE_RETURN;

  retval = EXECUTION_SUCCESS;
  show_menu = 1;

  while (1)
    {
      line_number = select_command->line;
      ps3_prompt = get_string_value ("PS3");
      if (ps3_prompt == 0)
	ps3_prompt = "#? ";

      QUIT;
      selection = select_query (list, list_len, ps3_prompt, show_menu);
      QUIT;
      if (selection == 0)
	{
	  /* select_query returns EXECUTION_FAILURE if the read builtin
	     fails, so we want to return failure in this case. */
	  retval = EXECUTION_FAILURE;
	  break;
	}

      v = bind_variable (identifier, selection, 0);
      if (readonly_p (v) || noassign_p (v))
	{
	  if (readonly_p (v) && interactive_shell == 0 && posixly_correct)
	    {
	      last_command_exit_value = EXECUTION_FAILURE;
	      jump_to_top_level (FORCE_EOF);
	    }
	  else
	    {
	      dispose_words (releaser);
	      discard_unwind_frame ("select");
	      loop_level--;
	      line_number = save_line_number;
	      return (EXECUTION_FAILURE);
	    }
	}

      retval = execute_command (select_command->action);

      REAP ();
      QUIT;

      if (breaking)
	{
	  breaking--;
	  break;
	}

      if (continuing)
	{
	  continuing--;
	  if (continuing)
	    break;
	}

#if defined (KSH_COMPATIBLE_SELECT)
      show_menu = 0;
      selection = get_string_value ("REPLY");
      if (selection && *selection == '\0')
        show_menu = 1;
#endif
    }

  loop_level--;
  line_number = save_line_number;

  dispose_words (releaser);
  discard_unwind_frame ("select");
  return (retval);
}
#endif /* SELECT_COMMAND */

/* Execute a CASE command.  The syntax is: CASE word_desc IN pattern_list ESAC.
   The pattern_list is a linked list of pattern clauses; each clause contains
   some patterns to compare word_desc against, and an associated command to
   execute. */
static int
execute_case_command (case_command)
     CASE_COM *case_command;
{
  register WORD_LIST *list;
  WORD_LIST *wlist, *es;
  PATTERN_LIST *clauses;
  char *word, *pattern;
  int retval, match, ignore_return, save_line_number;

  save_line_number = line_number;
  line_number = case_command->line;

  command_string_index = 0;
  print_case_command_head (case_command);

  if (echo_command_at_execute)
    xtrace_print_case_command_head (case_command);

#if 0
  if (signal_in_progress (DEBUG_TRAP) == 0 && (this_command_name == 0 || (STREQ (this_command_name, "trap") == 0)))
#else
  if (signal_in_progress (DEBUG_TRAP) == 0 && running_trap == 0)
#endif
    {
      FREE (the_printed_command_except_trap);
      the_printed_command_except_trap = savestring (the_printed_command);
    }

  retval = run_debug_trap();
#if defined (DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && retval != EXECUTION_SUCCESS)
    {
      line_number = save_line_number;
      return (EXECUTION_SUCCESS);
    }
#endif

  wlist = expand_word_unsplit (case_command->word, 0);
  word = wlist ? string_list (wlist) : savestring ("");
  dispose_words (wlist);

  retval = EXECUTION_SUCCESS;
  ignore_return = case_command->flags & CMD_IGNORE_RETURN;

  begin_unwind_frame ("case");
  add_unwind_protect ((Function *)xfree, word);

#define EXIT_CASE()  goto exit_case_command

  for (clauses = case_command->clauses; clauses; clauses = clauses->next)
    {
      QUIT;
      for (list = clauses->patterns; list; list = list->next)
	{
	  es = expand_word_leave_quoted (list->word, 0);

	  if (es && es->word && es->word->word && *(es->word->word))
	    pattern = quote_string_for_globbing (es->word->word, QGLOB_CVTNULL);
	  else
	    {
	      pattern = (char *)xmalloc (1);
	      pattern[0] = '\0';
	    }

	  /* Since the pattern does not undergo quote removal (as per
	     Posix.2, section 3.9.4.3), the strmatch () call must be able
	     to recognize backslashes as escape characters. */
	  match = strmatch (pattern, word, FNMATCH_EXTFLAG|FNMATCH_IGNCASE) != FNM_NOMATCH;
	  free (pattern);

	  dispose_words (es);

	  if (match)
	    {
	      do
		{
		  if (clauses->action && ignore_return)
		    clauses->action->flags |= CMD_IGNORE_RETURN;
		  retval = execute_command (clauses->action);
		}
	      while ((clauses->flags & CASEPAT_FALLTHROUGH) && (clauses = clauses->next));
	      if (clauses == 0 || (clauses->flags & CASEPAT_TESTNEXT) == 0)
		EXIT_CASE ();
	      else
		break;
	    }

	  QUIT;
	}
    }

exit_case_command:
  free (word);
  discard_unwind_frame ("case");
  line_number = save_line_number;
  return (retval);
}

#define CMD_WHILE 0
#define CMD_UNTIL 1

/* The WHILE command.  Syntax: WHILE test DO action; DONE.
   Repeatedly execute action while executing test produces
   EXECUTION_SUCCESS. */
static int
execute_while_command (while_command)
     WHILE_COM *while_command;
{
  return (execute_while_or_until (while_command, CMD_WHILE));
}

/* UNTIL is just like WHILE except that the test result is negated. */
static int
execute_until_command (while_command)
     WHILE_COM *while_command;
{
  return (execute_while_or_until (while_command, CMD_UNTIL));
}

/* The body for both while and until.  The only difference between the
   two is that the test value is treated differently.  TYPE is
   CMD_WHILE or CMD_UNTIL.  The return value for both commands should
   be EXECUTION_SUCCESS if no commands in the body are executed, and
   the status of the last command executed in the body otherwise. */
static int
execute_while_or_until (while_command, type)
     WHILE_COM *while_command;
     int type;
{
  int return_value, body_status;

  body_status = EXECUTION_SUCCESS;
  loop_level++;

  while_command->test->flags |= CMD_IGNORE_RETURN;
  if (while_command->flags & CMD_IGNORE_RETURN)
    while_command->action->flags |= CMD_IGNORE_RETURN;

  while (1)
    {
      return_value = execute_command (while_command->test);
      REAP ();

      /* Need to handle `break' in the test when we would break out of the
         loop.  The job control code will set `breaking' to loop_level
         when a job in a loop is stopped with SIGTSTP.  If the stopped job
         is in the loop test, `breaking' will not be reset unless we do
         this, and the shell will cease to execute commands. */
      if (type == CMD_WHILE && return_value != EXECUTION_SUCCESS)
	{
	  if (breaking)
	    breaking--;
	  break;
	}
      if (type == CMD_UNTIL && return_value == EXECUTION_SUCCESS)
	{
	  if (breaking)
	    breaking--;
	  break;
	}

      QUIT;
      body_status = execute_command (while_command->action);
      QUIT;

      if (breaking)
	{
	  breaking--;
	  break;
	}

      if (continuing)
	{
	  continuing--;
	  if (continuing)
	    break;
	}
    }
  loop_level--;

  return (body_status);
}

/* IF test THEN command [ELSE command].
   IF also allows ELIF in the place of ELSE IF, but
   the parser makes *that* stupidity transparent. */
static int
execute_if_command (if_command)
     IF_COM *if_command;
{
  int return_value, save_line_number;

  save_line_number = line_number;
  if_command->test->flags |= CMD_IGNORE_RETURN;
  return_value = execute_command (if_command->test);
  line_number = save_line_number;

  if (return_value == EXECUTION_SUCCESS)
    {
      QUIT;

      if (if_command->true_case && (if_command->flags & CMD_IGNORE_RETURN))
	if_command->true_case->flags |= CMD_IGNORE_RETURN;

      return (execute_command (if_command->true_case));
    }
  else
    {
      QUIT;

      if (if_command->false_case && (if_command->flags & CMD_IGNORE_RETURN))
	if_command->false_case->flags |= CMD_IGNORE_RETURN;

      return (execute_command (if_command->false_case));
    }
}

#if defined (DPAREN_ARITHMETIC)
static int
execute_arith_command (arith_command)
     ARITH_COM *arith_command;
{
  int expok, save_line_number, retval;
  intmax_t expresult;
  WORD_LIST *new;
  char *exp;

  expresult = 0;

  save_line_number = line_number;
  this_command_name = "((";	/* )) */
  line_number = arith_command->line;
  /* If we're in a function, update the line number information. */
  if (variable_context && interactive_shell)
    line_number -= function_line_number;

  command_string_index = 0;
  print_arith_command (arith_command->exp);

  if (signal_in_progress (DEBUG_TRAP) == 0)
    {
      FREE (the_printed_command_except_trap);
      the_printed_command_except_trap = savestring (the_printed_command);
    }

  /* Run the debug trap before each arithmetic command, but do it after we
     update the line number information and before we expand the various
     words in the expression. */
  retval = run_debug_trap ();
#if defined (DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && retval != EXECUTION_SUCCESS)
    {
      line_number = save_line_number;
      return (EXECUTION_SUCCESS);
    }
#endif

  new = expand_words_no_vars (arith_command->exp);

  /* If we're tracing, make a new word list with `((' at the front and `))'
     at the back and print it. */
  if (echo_command_at_execute)
    xtrace_print_arith_cmd (new);

  if (new)
    {
      exp = new->next ? string_list (new) : new->word->word;
      expresult = evalexp (exp, &expok);
      line_number = save_line_number;
      if (exp != new->word->word)
	free (exp);
      dispose_words (new);
    }
  else
    {
      expresult = 0;
      expok = 1;
    }

  if (expok == 0)
    return (EXECUTION_FAILURE);

  return (expresult == 0 ? EXECUTION_FAILURE : EXECUTION_SUCCESS);
}
#endif /* DPAREN_ARITHMETIC */

#if defined (COND_COMMAND)

static char * const nullstr = "";

static int
execute_cond_node (cond)
     COND_COM *cond;
{
  int result, invert, patmatch, rmatch, mflags, ignore;
  char *arg1, *arg2;

  invert = (cond->flags & CMD_INVERT_RETURN);
  ignore = (cond->flags & CMD_IGNORE_RETURN);
  if (ignore)
    {
      if (cond->left)
	cond->left->flags |= CMD_IGNORE_RETURN;
      if (cond->right)
	cond->right->flags |= CMD_IGNORE_RETURN;
    }
      
  if (cond->type == COND_EXPR)
    result = execute_cond_node (cond->left);
  else if (cond->type == COND_OR)
    {
      result = execute_cond_node (cond->left);
      if (result != EXECUTION_SUCCESS)
	result = execute_cond_node (cond->right);
    }
  else if (cond->type == COND_AND)
    {
      result = execute_cond_node (cond->left);
      if (result == EXECUTION_SUCCESS)
	result = execute_cond_node (cond->right);
    }
  else if (cond->type == COND_UNARY)
    {
      if (ignore)
	comsub_ignore_return++;
      arg1 = cond_expand_word (cond->left->op, 0);
      if (ignore)
	comsub_ignore_return--;
      if (arg1 == 0)
	arg1 = nullstr;
      if (echo_command_at_execute)
	xtrace_print_cond_term (cond->type, invert, cond->op, arg1, (char *)NULL);
      result = unary_test (cond->op->word, arg1) ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
      if (arg1 != nullstr)
	free (arg1);
    }
  else if (cond->type == COND_BINARY)
    {
      rmatch = 0;
      patmatch = ((cond->op->word[1] == '=') && (cond->op->word[2] == '\0') &&
		  (cond->op->word[0] == '!' || cond->op->word[0] == '=') ||
		  (cond->op->word[0] == '=' && cond->op->word[1] == '\0'));
#if defined (COND_REGEXP)
      rmatch = (cond->op->word[0] == '=' && cond->op->word[1] == '~' &&
		cond->op->word[2] == '\0');
#endif

      if (ignore)
	comsub_ignore_return++;
      arg1 = cond_expand_word (cond->left->op, 0);
      if (ignore)
	comsub_ignore_return--;
      if (arg1 == 0)
	arg1 = nullstr;
      if (ignore)
	comsub_ignore_return++;
      arg2 = cond_expand_word (cond->right->op,
			       (rmatch && shell_compatibility_level > 31) ? 2 : (patmatch ? 1 : 0));
      if (ignore)
	comsub_ignore_return--;
      if (arg2 == 0)
	arg2 = nullstr;

      if (echo_command_at_execute)
	xtrace_print_cond_term (cond->type, invert, cond->op, arg1, arg2);

#if defined (COND_REGEXP)
      if (rmatch)
	{
	  mflags = SHMAT_PWARN;
#if defined (ARRAY_VARS)
	  mflags |= SHMAT_SUBEXP;
#endif

	  result = sh_regmatch (arg1, arg2, mflags);
	}
      else
#endif /* COND_REGEXP */
	{
	  int oe;
	  oe = extended_glob;
	  extended_glob = 1;
	  result = binary_test (cond->op->word, arg1, arg2, TEST_PATMATCH|TEST_ARITHEXP|TEST_LOCALE)
				  ? EXECUTION_SUCCESS
				  : EXECUTION_FAILURE;
	  extended_glob = oe;
	}
      if (arg1 != nullstr)
	free (arg1);
      if (arg2 != nullstr)
	free (arg2);
    }
  else
    {
      command_error ("execute_cond_node", CMDERR_BADTYPE, cond->type, 0);
      jump_to_top_level (DISCARD);
      result = EXECUTION_FAILURE;
    }

  if (invert)
    result = (result == EXECUTION_SUCCESS) ? EXECUTION_FAILURE : EXECUTION_SUCCESS;

  return result;
}

static int
execute_cond_command (cond_command)
     COND_COM *cond_command;
{
  int retval, save_line_number;

  retval = EXECUTION_SUCCESS;
  save_line_number = line_number;

  this_command_name = "[[";
  line_number = cond_command->line;
  /* If we're in a function, update the line number information. */
  if (variable_context && interactive_shell)
    line_number -= function_line_number;
  command_string_index = 0;
  print_cond_command (cond_command);

  if (signal_in_progress (DEBUG_TRAP) == 0)
    {
      FREE (the_printed_command_except_trap);
      the_printed_command_except_trap = savestring (the_printed_command);
    }

  /* Run the debug trap before each conditional command, but do it after we
     update the line number information. */
  retval = run_debug_trap ();
#if defined (DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && retval != EXECUTION_SUCCESS)
    {
      line_number = save_line_number;
      return (EXECUTION_SUCCESS);
    }
#endif

#if 0
  debug_print_cond_command (cond_command);
#endif

  last_command_exit_value = retval = execute_cond_node (cond_command);
  line_number = save_line_number;
  return (retval);
}
#endif /* COND_COMMAND */

static void
bind_lastarg (arg)
     char *arg;
{
  SHELL_VAR *var;

  if (arg == 0)
    arg = "";
  var = bind_variable ("_", arg, 0);
  VUNSETATTR (var, att_exported);
}

/* Execute a null command.  Fork a subshell if the command uses pipes or is
   to be run asynchronously.  This handles all the side effects that are
   supposed to take place. */
static int
execute_null_command (redirects, pipe_in, pipe_out, async)
     REDIRECT *redirects;
     int pipe_in, pipe_out, async;
{
  int r;
  int forcefork;
  REDIRECT *rd;

  for (forcefork = 0, rd = redirects; rd; rd = rd->next)
    forcefork += rd->rflags & REDIR_VARASSIGN;

  if (forcefork || pipe_in != NO_PIPE || pipe_out != NO_PIPE || async)
    {
      /* We have a null command, but we really want a subshell to take
	 care of it.  Just fork, do piping and redirections, and exit. */
      if (make_child ((char *)NULL, async) == 0)
	{
	  /* Cancel traps, in trap.c. */
	  restore_original_signals ();		/* XXX */

	  do_piping (pipe_in, pipe_out);

#if defined (COPROCESS_SUPPORT)
	  coproc_closeall ();
#endif

	  subshell_environment = 0;
	  if (async)
	    subshell_environment |= SUBSHELL_ASYNC;
	  if (pipe_in != NO_PIPE || pipe_out != NO_PIPE)
	    subshell_environment |= SUBSHELL_PIPE;

	  if (do_redirections (redirects, RX_ACTIVE) == 0)
	    exit (EXECUTION_SUCCESS);
	  else
	    exit (EXECUTION_FAILURE);
	}
      else
	{
	  close_pipes (pipe_in, pipe_out);
#if defined (PROCESS_SUBSTITUTION) && defined (HAVE_DEV_FD)
	  unlink_fifo_list ();
#endif
	  return (EXECUTION_SUCCESS);
	}
    }
  else
    {
      /* Even if there aren't any command names, pretend to do the
	 redirections that are specified.  The user expects the side
	 effects to take place.  If the redirections fail, then return
	 failure.  Otherwise, if a command substitution took place while
	 expanding the command or a redirection, return the value of that
	 substitution.  Otherwise, return EXECUTION_SUCCESS. */

      r = do_redirections (redirects, RX_ACTIVE|RX_UNDOABLE);
      cleanup_redirects (redirection_undo_list);
      redirection_undo_list = (REDIRECT *)NULL;

      if (r != 0)
	return (EXECUTION_FAILURE);
      else if (last_command_subst_pid != NO_PID)
	return (last_command_exit_value);
      else
	return (EXECUTION_SUCCESS);
    }
}

/* This is a hack to suppress word splitting for assignment statements
   given as arguments to builtins with the ASSIGNMENT_BUILTIN flag set. */
static void
fix_assignment_words (words)
     WORD_LIST *words;
{
  WORD_LIST *w;
  struct builtin *b;
  int assoc;

  if (words == 0)
    return;

  b = 0;
  assoc = 0;

  for (w = words; w; w = w->next)
    if (w->word->flags & W_ASSIGNMENT)
      {
	if (b == 0)
	  {
	    b = builtin_address_internal (words->word->word, 0);
	    if (b == 0 || (b->flags & ASSIGNMENT_BUILTIN) == 0)
	      return;
	    else if (b && (b->flags & ASSIGNMENT_BUILTIN))
	      words->word->flags |= W_ASSNBLTIN;
	  }
	w->word->flags |= (W_NOSPLIT|W_NOGLOB|W_TILDEEXP|W_ASSIGNARG);
#if defined (ARRAY_VARS)
	if (assoc)
	  w->word->flags |= W_ASSIGNASSOC;
#endif
      }
#if defined (ARRAY_VARS)
    /* Note that we saw an associative array option to a builtin that takes
       assignment statements.  This is a bit of a kludge. */
    else if (w->word->word[0] == '-' && strchr (w->word->word, 'A'))
      {
	if (b == 0)
	  {
	    b = builtin_address_internal (words->word->word, 0);
	    if (b == 0 || (b->flags & ASSIGNMENT_BUILTIN) == 0)
	      return;
	    else if (b && (b->flags & ASSIGNMENT_BUILTIN))
	      words->word->flags |= W_ASSNBLTIN;
	  }
	if (words->word->flags & W_ASSNBLTIN)
	  assoc = 1;
      }
#endif
}

/* Return 1 if the file found by searching $PATH for PATHNAME, defaulting
   to PATHNAME, is a directory.  Used by the autocd code below. */
static int
is_dirname (pathname)
     char *pathname;
{
  char *temp;
  temp = search_for_command (pathname);
  return (temp ? file_isdir (temp) : file_isdir (pathname));
}

/* The meaty part of all the executions.  We have to start hacking the
   real execution of commands here.  Fork a process, set things up,
   execute the command. */
static int
execute_simple_command (simple_command, pipe_in, pipe_out, async, fds_to_close)
     SIMPLE_COM *simple_command;
     int pipe_in, pipe_out, async;
     struct fd_bitmap *fds_to_close;
{
  WORD_LIST *words, *lastword;
  char *command_line, *lastarg, *temp;
  int first_word_quoted, result, builtin_is_special, already_forked, dofork;
  pid_t old_last_async_pid;
  sh_builtin_func_t *builtin;
  SHELL_VAR *func;

  result = EXECUTION_SUCCESS;
  special_builtin_failed = builtin_is_special = 0;
  command_line = (char *)0;

  /* If we're in a function, update the line number information. */
  if (variable_context && interactive_shell && sourcelevel == 0)
    line_number -= function_line_number;

  /* Remember what this command line looks like at invocation. */
  command_string_index = 0;
  print_simple_command (simple_command);

#if 0
  if (signal_in_progress (DEBUG_TRAP) == 0 && (this_command_name == 0 || (STREQ (this_command_name, "trap") == 0)))
#else
  if (signal_in_progress (DEBUG_TRAP) == 0 && running_trap == 0)
#endif
    {
      FREE (the_printed_command_except_trap);
      the_printed_command_except_trap = the_printed_command ? savestring (the_printed_command) : (char *)0;
    }

  /* Run the debug trap before each simple command, but do it after we
     update the line number information. */
  result = run_debug_trap ();
#if defined (DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && result != EXECUTION_SUCCESS)
    return (EXECUTION_SUCCESS);
#endif

  first_word_quoted =
    simple_command->words ? (simple_command->words->word->flags & W_QUOTED) : 0;

  last_command_subst_pid = NO_PID;
  old_last_async_pid = last_asynchronous_pid;

  already_forked = dofork = 0;

  /* If we're in a pipeline or run in the background, set DOFORK so we
     make the child early, before word expansion.  This keeps assignment
     statements from affecting the parent shell's environment when they
     should not. */
  dofork = pipe_in != NO_PIPE || pipe_out != NO_PIPE || async;

  /* Something like `%2 &' should restart job 2 in the background, not cause
     the shell to fork here. */
  if (dofork && pipe_in == NO_PIPE && pipe_out == NO_PIPE &&
	simple_command->words && simple_command->words->word &&
	simple_command->words->word->word &&
	(simple_command->words->word->word[0] == '%'))
    dofork = 0;

  if (dofork)
    {
      /* Do this now, because execute_disk_command will do it anyway in the
	 vast majority of cases. */
      maybe_make_export_env ();

      /* Don't let a DEBUG trap overwrite the command string to be saved with
	 the process/job associated with this child. */
      if (make_child (savestring (the_printed_command_except_trap), async) == 0)
	{
	  already_forked = 1;
	  simple_command->flags |= CMD_NO_FORK;

	  subshell_environment = SUBSHELL_FORK;
	  if (pipe_in != NO_PIPE || pipe_out != NO_PIPE)
	    subshell_environment |= SUBSHELL_PIPE;
	  if (async)
	    subshell_environment |= SUBSHELL_ASYNC;

	  /* We need to do this before piping to handle some really
	     pathological cases where one of the pipe file descriptors
	     is < 2. */
	  if (fds_to_close)
	    close_fd_bitmap (fds_to_close);

	  do_piping (pipe_in, pipe_out);
	  pipe_in = pipe_out = NO_PIPE;
#if defined (COPROCESS_SUPPORT)
	  coproc_closeall ();
#endif

	  last_asynchronous_pid = old_last_async_pid;
	}
      else
	{
	  close_pipes (pipe_in, pipe_out);
#if defined (PROCESS_SUBSTITUTION) && defined (HAVE_DEV_FD)
	  unlink_fifo_list ();
#endif
	  command_line = (char *)NULL;      /* don't free this. */
	  bind_lastarg ((char *)NULL);
	  return (result);
	}
    }

  /* If we are re-running this as the result of executing the `command'
     builtin, do not expand the command words a second time. */
  if ((simple_command->flags & CMD_INHIBIT_EXPANSION) == 0)
    {
      current_fds_to_close = fds_to_close;
      fix_assignment_words (simple_command->words);
      /* Pass the ignore return flag down to command substitutions */
      if (simple_command->flags & CMD_IGNORE_RETURN)	/* XXX */
	comsub_ignore_return++;
      words = expand_words (simple_command->words);
      if (simple_command->flags & CMD_IGNORE_RETURN)
	comsub_ignore_return--;
      current_fds_to_close = (struct fd_bitmap *)NULL;
    }
  else
    words = copy_word_list (simple_command->words);

  /* It is possible for WORDS not to have anything left in it.
     Perhaps all the words consisted of `$foo', and there was
     no variable `$foo'. */
  if (words == 0)
    {
      this_command_name = 0;
      result = execute_null_command (simple_command->redirects,
				     pipe_in, pipe_out,
				     already_forked ? 0 : async);
      if (already_forked)
	exit (result);
      else
	{
	  bind_lastarg ((char *)NULL);
	  set_pipestatus_from_exit (result);
	  return (result);
	}
    }

  lastarg = (char *)NULL;

  begin_unwind_frame ("simple-command");

  if (echo_command_at_execute)
    xtrace_print_word_list (words, 1);

  builtin = (sh_builtin_func_t *)NULL;
  func = (SHELL_VAR *)NULL;
  if ((simple_command->flags & CMD_NO_FUNCTIONS) == 0)
    {
      /* Posix.2 says special builtins are found before functions.  We
	 don't set builtin_is_special anywhere other than here, because
	 this path is followed only when the `command' builtin is *not*
	 being used, and we don't want to exit the shell if a special
	 builtin executed with `command builtin' fails.  `command' is not
	 a special builtin. */
      if (posixly_correct)
	{
	  builtin = find_special_builtin (words->word->word);
	  if (builtin)
	    builtin_is_special = 1;
	}
      if (builtin == 0)
	func = find_function (words->word->word);
    }

  /* In POSIX mode, assignment errors in the temporary environment cause a
     non-interactive shell to exit. */
  if (builtin_is_special && interactive_shell == 0 && tempenv_assign_error)
    {
      last_command_exit_value = EXECUTION_FAILURE;
      jump_to_top_level (ERREXIT);
    }

  add_unwind_protect (dispose_words, words);
  QUIT;

  /* Bind the last word in this command to "$_" after execution. */
  for (lastword = words; lastword->next; lastword = lastword->next)
    ;
  lastarg = lastword->word->word;

#if defined (JOB_CONTROL)
  /* Is this command a job control related thing? */
  if (words->word->word[0] == '%' && already_forked == 0)
    {
      this_command_name = async ? "bg" : "fg";
      last_shell_builtin = this_shell_builtin;
      this_shell_builtin = builtin_address (this_command_name);
      result = (*this_shell_builtin) (words);
      goto return_result;
    }

  /* One other possiblilty.  The user may want to resume an existing job.
     If they do, find out whether this word is a candidate for a running
     job. */
  if (job_control && already_forked == 0 && async == 0 &&
	!first_word_quoted &&
	!words->next &&
	words->word->word[0] &&
	!simple_command->redirects &&
	pipe_in == NO_PIPE &&
	pipe_out == NO_PIPE &&
	(temp = get_string_value ("auto_resume")))
    {
      int job, jflags, started_status;

      jflags = JM_STOPPED|JM_FIRSTMATCH;
      if (STREQ (temp, "exact"))
	jflags |= JM_EXACT;
      else if (STREQ (temp, "substring"))
	jflags |= JM_SUBSTRING;
      else
	jflags |= JM_PREFIX;
      job = get_job_by_name (words->word->word, jflags);
      if (job != NO_JOB)
	{
	  run_unwind_frame ("simple-command");
	  this_command_name = "fg";
	  last_shell_builtin = this_shell_builtin;
	  this_shell_builtin = builtin_address ("fg");

	  started_status = start_job (job, 1);
	  return ((started_status < 0) ? EXECUTION_FAILURE : started_status);
	}
    }
#endif /* JOB_CONTROL */

run_builtin:
  /* Remember the name of this command globally. */
  this_command_name = words->word->word;

  QUIT;

  /* This command could be a shell builtin or a user-defined function.
     We have already found special builtins by this time, so we do not
     set builtin_is_special.  If this is a function or builtin, and we
     have pipes, then fork a subshell in here.  Otherwise, just execute
     the command directly. */
  if (func == 0 && builtin == 0)
    builtin = find_shell_builtin (this_command_name);

  last_shell_builtin = this_shell_builtin;
  this_shell_builtin = builtin;

  if (builtin || func)
    {
      if (builtin)
	unwind_protect_int (executing_builtin);	/* modified in execute_builtin */
      if (already_forked)
	{
	  /* reset_terminating_signals (); */	/* XXX */
	  /* Cancel traps, in trap.c. */
	  restore_original_signals ();

	  if (async)
	    {
	      if ((simple_command->flags & CMD_STDIN_REDIR) &&
		    pipe_in == NO_PIPE &&
		    (stdin_redirects (simple_command->redirects) == 0))
		async_redirect_stdin ();
	      setup_async_signals ();
	    }

	  subshell_level++;
	  execute_subshell_builtin_or_function
	    (words, simple_command->redirects, builtin, func,
	     pipe_in, pipe_out, async, fds_to_close,
	     simple_command->flags);
	  subshell_level--;
	}
      else
	{
	  result = execute_builtin_or_function
	    (words, builtin, func, simple_command->redirects, fds_to_close,
	     simple_command->flags);
	  if (builtin)
	    {
	      if (result > EX_SHERRBASE)
		{
		  result = builtin_status (result);
		  if (builtin_is_special)
		    special_builtin_failed = 1;
		}
	      /* In POSIX mode, if there are assignment statements preceding
		 a special builtin, they persist after the builtin
		 completes. */
	      if (posixly_correct && builtin_is_special && temporary_env)
		merge_temporary_env ();
	    }
	  else		/* function */
	    {
	      if (result == EX_USAGE)
		result = EX_BADUSAGE;
	      else if (result > EX_SHERRBASE)
		result = EXECUTION_FAILURE;
	    }

	  set_pipestatus_from_exit (result);

	  goto return_result;
	}
    }

  if (autocd && interactive && words->word && is_dirname (words->word->word))
    {
      words = make_word_list (make_word ("cd"), words);
      xtrace_print_word_list (words, 0);
      goto run_builtin;
    }

  if (command_line == 0)
    command_line = savestring (the_printed_command_except_trap);

#if defined (PROCESS_SUBSTITUTION)
  if ((subshell_environment & SUBSHELL_COMSUB) && (simple_command->flags & CMD_NO_FORK) && fifos_pending() > 0)
    simple_command->flags &= ~CMD_NO_FORK;
#endif

  execute_disk_command (words, simple_command->redirects, command_line,
			pipe_in, pipe_out, async, fds_to_close,
			simple_command->flags);

 return_result:
  bind_lastarg (lastarg);
  FREE (command_line);
  dispose_words (words);
  discard_unwind_frame ("simple-command");
  this_command_name = (char *)NULL;	/* points to freed memory now */
  return (result);
}

/* Translate the special builtin exit statuses.  We don't really need a
   function for this; it's a placeholder for future work. */
static int
builtin_status (result)
     int result;
{
  int r;

  switch (result)
    {
    case EX_USAGE:
      r = EX_BADUSAGE;
      break;
    case EX_REDIRFAIL:
    case EX_BADSYNTAX:
    case EX_BADASSIGN:
    case EX_EXPFAIL:
      r = EXECUTION_FAILURE;
      break;
    default:
      r = EXECUTION_SUCCESS;
      break;
    }
  return (r);
}

static int
execute_builtin (builtin, words, flags, subshell)
     sh_builtin_func_t *builtin;
     WORD_LIST *words;
     int flags, subshell;
{
  int old_e_flag, result, eval_unwind;
  int isbltinenv;
  char *error_trap;

#if 0
  /* XXX -- added 12/11 */
  terminate_immediately++;
#endif

  error_trap = 0;
  old_e_flag = exit_immediately_on_error;
  /* The eval builtin calls parse_and_execute, which does not know about
     the setting of flags, and always calls the execution functions with
     flags that will exit the shell on an error if -e is set.  If the
     eval builtin is being called, and we're supposed to ignore the exit
     value of the command, we turn the -e flag off ourselves and disable
     the ERR trap, then restore them when the command completes.  This is
     also a problem (as below) for the command and source/. builtins. */
  if (subshell == 0 && (flags & CMD_IGNORE_RETURN) &&
	(builtin == eval_builtin || builtin == command_builtin || builtin == source_builtin))
    {
      begin_unwind_frame ("eval_builtin");
      unwind_protect_int (exit_immediately_on_error);
      error_trap = TRAP_STRING (ERROR_TRAP);
      if (error_trap)
	{
	  error_trap = savestring (error_trap);
	  add_unwind_protect (xfree, error_trap);
	  add_unwind_protect (set_error_trap, error_trap);
	  restore_default_signal (ERROR_TRAP);
	}
      exit_immediately_on_error = 0;
      eval_unwind = 1;
    }
  else
    eval_unwind = 0;

  /* The temporary environment for a builtin is supposed to apply to
     all commands executed by that builtin.  Currently, this is a
     problem only with the `unset', `source' and `eval' builtins. */

  isbltinenv = (builtin == source_builtin || builtin == eval_builtin || builtin == unset_builtin);

  if (isbltinenv)
    {
      if (subshell == 0)
	begin_unwind_frame ("builtin_env");

      if (temporary_env)
	{
	  push_scope (VC_BLTNENV, temporary_env);
	  if (subshell == 0)
	    add_unwind_protect (pop_scope, (flags & CMD_COMMAND_BUILTIN) ? 0 : "1");
          temporary_env = (HASH_TABLE *)NULL;	  
	}
    }

  /* `return' does a longjmp() back to a saved environment in execute_function.
     If a variable assignment list preceded the command, and the shell is
     running in POSIX mode, we need to merge that into the shell_variables
     table, since `return' is a POSIX special builtin. */
  if (posixly_correct && subshell == 0 && builtin == return_builtin && temporary_env)
    {
      begin_unwind_frame ("return_temp_env");
      add_unwind_protect (merge_temporary_env, (char *)NULL);
    }

  executing_builtin++;
  result = ((*builtin) (words->next));

  /* This shouldn't happen, but in case `return' comes back instead of
     longjmp'ing, we need to unwind. */
  if (posixly_correct && subshell == 0 && builtin == return_builtin && temporary_env)
    discard_unwind_frame ("return_temp_env");

  if (subshell == 0 && isbltinenv)
    run_unwind_frame ("builtin_env");

  if (eval_unwind)
    {
      exit_immediately_on_error += old_e_flag;
      if (error_trap)
	{
	  set_error_trap (error_trap);
	  xfree (error_trap);
	}
      discard_unwind_frame ("eval_builtin");
    }

#if 0
  /* XXX -- added 12/11 */
  terminate_immediately--;
#endif

  return (result);
}

static int
execute_function (var, words, flags, fds_to_close, async, subshell)
     SHELL_VAR *var;
     WORD_LIST *words;
     int flags;
     struct fd_bitmap *fds_to_close;
     int async, subshell;
{
  int return_val, result;
  COMMAND *tc, *fc, *save_current;
  char *debug_trap, *error_trap, *return_trap;
#if defined (ARRAY_VARS)
  SHELL_VAR *funcname_v, *nfv, *bash_source_v, *bash_lineno_v;
  ARRAY *funcname_a, *bash_source_a, *bash_lineno_a;
#endif
  FUNCTION_DEF *shell_fn;
  char *sfile, *t;

  USE_VAR(fc);

#if 0	/* for bash-4.2 */
  if (funcnest_max > 0 && funcnest >= funcnest_max)
    {
      internal_error ("%s: maximum function nesting level exceeded (%d)", var->name, funcnest);
      jump_to_top_level (DISCARD);
    }
#endif

#if defined (ARRAY_VARS)
  GET_ARRAY_FROM_VAR ("FUNCNAME", funcname_v, funcname_a);
  GET_ARRAY_FROM_VAR ("BASH_SOURCE", bash_source_v, bash_source_a);
  GET_ARRAY_FROM_VAR ("BASH_LINENO", bash_lineno_v, bash_lineno_a);
#endif

  tc = (COMMAND *)copy_command (function_cell (var));
  if (tc && (flags & CMD_IGNORE_RETURN))
    tc->flags |= CMD_IGNORE_RETURN;

  if (subshell == 0)
    {
      begin_unwind_frame ("function_calling");
      push_context (var->name, subshell, temporary_env);
      add_unwind_protect (pop_context, (char *)NULL);
      unwind_protect_int (line_number);
      unwind_protect_int (return_catch_flag);
      unwind_protect_jmp_buf (return_catch);
      add_unwind_protect (dispose_command, (char *)tc);
      unwind_protect_pointer (this_shell_function);
      unwind_protect_int (loop_level);
      unwind_protect_int (funcnest);
    }
  else
    push_context (var->name, subshell, temporary_env);	/* don't unwind-protect for subshells */

  temporary_env = (HASH_TABLE *)NULL;

  this_shell_function = var;
  make_funcname_visible (1);

  debug_trap = TRAP_STRING(DEBUG_TRAP);
  error_trap = TRAP_STRING(ERROR_TRAP);
  return_trap = TRAP_STRING(RETURN_TRAP);
  
  /* The order of the unwind protects for debug_trap, error_trap and
     return_trap is important here!  unwind-protect commands are run
     in reverse order of registration.  If this causes problems, take
     out the xfree unwind-protect calls and live with the small memory leak. */

  /* function_trace_mode != 0 means that all functions inherit the DEBUG trap.
     if the function has the trace attribute set, it inherits the DEBUG trap */
  if (debug_trap && ((trace_p (var) == 0) && function_trace_mode == 0))
    {
      if (subshell == 0)
	{
	  debug_trap = savestring (debug_trap);
	  add_unwind_protect (xfree, debug_trap);
	  add_unwind_protect (set_debug_trap, debug_trap);
	}
      restore_default_signal (DEBUG_TRAP);
    }

  /* error_trace_mode != 0 means that functions inherit the ERR trap. */
  if (error_trap && error_trace_mode == 0)
    {
      if (subshell == 0)
	{
	  error_trap = savestring (error_trap);
	  add_unwind_protect (xfree, error_trap);
	  add_unwind_protect (set_error_trap, error_trap);
	}
      restore_default_signal (ERROR_TRAP);
    }

  /* Shell functions inherit the RETURN trap if function tracing is on
     globally or on individually for this function. */
#if 0
  if (return_trap && ((trace_p (var) == 0) && function_trace_mode == 0))
#else
  if (return_trap && (signal_in_progress (DEBUG_TRAP) || ((trace_p (var) == 0) && function_trace_mode == 0)))
#endif
    {
      if (subshell == 0)
	{
	  return_trap = savestring (return_trap);
	  add_unwind_protect (xfree, return_trap);
	  add_unwind_protect (set_return_trap, return_trap);
	}
      restore_default_signal (RETURN_TRAP);
    }
  
  funcnest++;
#if defined (ARRAY_VARS)
  /* This is quite similar to the code in shell.c and elsewhere. */
  shell_fn = find_function_def (this_shell_function->name);
  sfile = shell_fn ? shell_fn->source_file : "";
  array_push (funcname_a, this_shell_function->name);

  array_push (bash_source_a, sfile);
  t = itos (executing_line_number ());
  array_push (bash_lineno_a, t);
  free (t);
#endif

  /* The temporary environment for a function is supposed to apply to
     all commands executed within the function body. */

  remember_args (words->next, 1);

  /* Update BASH_ARGV and BASH_ARGC */
  if (debugging_mode)
    push_args (words->next);

  /* Number of the line on which the function body starts. */
  line_number = function_line_number = tc->line;

#if defined (JOB_CONTROL)
  if (subshell)
    stop_pipeline (async, (COMMAND *)NULL);
#endif

  fc = tc;

  return_catch_flag++;
  return_val = setjmp (return_catch);

  if (return_val)
    {
      result = return_catch_value;
      /* Run the RETURN trap in the function's context. */
      save_current = currently_executing_command;
      run_return_trap ();
      currently_executing_command = save_current;
    }
  else
    {
      /* Run the debug trap here so we can trap at the start of a function's
	 execution rather than the execution of the body's first command. */
      showing_function_line = 1;
      save_current = currently_executing_command;
      result = run_debug_trap ();
#if defined (DEBUGGER)
      /* In debugging mode, if the DEBUG trap returns a non-zero status, we
	 skip the command. */
      if (debugging_mode == 0 || result == EXECUTION_SUCCESS)
	{
	  showing_function_line = 0;
	  currently_executing_command = save_current;
	  result = execute_command_internal (fc, 0, NO_PIPE, NO_PIPE, fds_to_close);

	  /* Run the RETURN trap in the function's context */
	  save_current = currently_executing_command;
	  run_return_trap ();
	  currently_executing_command = save_current;
	}
#else
      result = execute_command_internal (fc, 0, NO_PIPE, NO_PIPE, fds_to_close);

      save_current = currently_executing_command;
      run_return_trap ();
      currently_executing_command = save_current;
#endif
      showing_function_line = 0;
    }

  /* Restore BASH_ARGC and BASH_ARGV */
  if (debugging_mode)
    pop_args ();

  if (subshell == 0)
    run_unwind_frame ("function_calling");

#if defined (ARRAY_VARS)
  /* These two variables cannot be unset, and cannot be affected by the
     function. */
  array_pop (bash_source_a);
  array_pop (bash_lineno_a);

  /* FUNCNAME can be unset, and so can potentially be changed by the
     function. */
  GET_ARRAY_FROM_VAR ("FUNCNAME", nfv, funcname_a);
  if (nfv == funcname_v)
    array_pop (funcname_a);
#endif
  
  if (variable_context == 0 || this_shell_function == 0)
    {
      make_funcname_visible (0);
#if defined (PROCESS_SUBSTITUTION)
      unlink_fifo_list ();
#endif
    }
  
  return (result);
}

/* A convenience routine for use by other parts of the shell to execute
   a particular shell function. */
int
execute_shell_function (var, words)
     SHELL_VAR *var;
     WORD_LIST *words;
{
  int ret;
  struct fd_bitmap *bitmap;

  bitmap = new_fd_bitmap (FD_BITMAP_DEFAULT_SIZE);
  begin_unwind_frame ("execute-shell-function");
  add_unwind_protect (dispose_fd_bitmap, (char *)bitmap);
      
  ret = execute_function (var, words, 0, bitmap, 0, 0);

  dispose_fd_bitmap (bitmap);
  discard_unwind_frame ("execute-shell-function");

  return ret;
}

/* Execute a shell builtin or function in a subshell environment.  This
   routine does not return; it only calls exit().  If BUILTIN is non-null,
   it points to a function to call to execute a shell builtin; otherwise
   VAR points at the body of a function to execute.  WORDS is the arguments
   to the command, REDIRECTS specifies redirections to perform before the
   command is executed. */
static void
execute_subshell_builtin_or_function (words, redirects, builtin, var,
				      pipe_in, pipe_out, async, fds_to_close,
				      flags)
     WORD_LIST *words;
     REDIRECT *redirects;
     sh_builtin_func_t *builtin;
     SHELL_VAR *var;
     int pipe_in, pipe_out, async;
     struct fd_bitmap *fds_to_close;
     int flags;
{
  int result, r, funcvalue;
#if defined (JOB_CONTROL)
  int jobs_hack;

  jobs_hack = (builtin == jobs_builtin) &&
		((subshell_environment & SUBSHELL_ASYNC) == 0 || pipe_out != NO_PIPE);
#endif

  /* A subshell is neither a login shell nor interactive. */
  login_shell = interactive = 0;

  if (async)
    subshell_environment |= SUBSHELL_ASYNC;
  if (pipe_in != NO_PIPE || pipe_out != NO_PIPE)
    subshell_environment |= SUBSHELL_PIPE;

  maybe_make_export_env ();	/* XXX - is this needed? */

#if defined (JOB_CONTROL)
  /* Eradicate all traces of job control after we fork the subshell, so
     all jobs begun by this subshell are in the same process group as
     the shell itself. */

  /* Allow the output of `jobs' to be piped. */
  if (jobs_hack)
    kill_current_pipeline ();
  else
    without_job_control ();

  set_sigchld_handler ();
#endif /* JOB_CONTROL */

  set_sigint_handler ();

  if (fds_to_close)
    close_fd_bitmap (fds_to_close);

  do_piping (pipe_in, pipe_out);

  if (do_redirections (redirects, RX_ACTIVE) != 0)
    exit (EXECUTION_FAILURE);

  if (builtin)
    {
      /* Give builtins a place to jump back to on failure,
	 so we don't go back up to main(). */
      result = setjmp (top_level);

      /* Give the return builtin a place to jump to when executed in a subshell
         or pipeline */
      funcvalue = 0;
      if (return_catch_flag && builtin == return_builtin)
        funcvalue = setjmp (return_catch);

      if (result == EXITPROG)
	exit (last_command_exit_value);
      else if (result)
	exit (EXECUTION_FAILURE);
      else if (funcvalue)
	exit (return_catch_value);
      else
	{
	  r = execute_builtin (builtin, words, flags, 1);
	  fflush (stdout);
	  if (r == EX_USAGE)
	    r = EX_BADUSAGE;
	  exit (r);
	}
    }
  else
    {
      r = execute_function (var, words, flags, fds_to_close, async, 1);
      fflush (stdout);
      exit (r);
    }
}

/* Execute a builtin or function in the current shell context.  If BUILTIN
   is non-null, it is the builtin command to execute, otherwise VAR points
   to the body of a function.  WORDS are the command's arguments, REDIRECTS
   are the redirections to perform.  FDS_TO_CLOSE is the usual bitmap of
   file descriptors to close.

   If BUILTIN is exec_builtin, the redirections specified in REDIRECTS are
   not undone before this function returns. */
static int
execute_builtin_or_function (words, builtin, var, redirects,
			     fds_to_close, flags)
     WORD_LIST *words;
     sh_builtin_func_t *builtin;
     SHELL_VAR *var;
     REDIRECT *redirects;
     struct fd_bitmap *fds_to_close;
     int flags;
{
  int result;
  REDIRECT *saved_undo_list;
  sh_builtin_func_t *saved_this_shell_builtin;

  if (do_redirections (redirects, RX_ACTIVE|RX_UNDOABLE) != 0)
    {
      cleanup_redirects (redirection_undo_list);
      redirection_undo_list = (REDIRECT *)NULL;
      dispose_exec_redirects ();
      return (EX_REDIRFAIL);	/* was EXECUTION_FAILURE */
    }

  saved_this_shell_builtin = this_shell_builtin;
  saved_undo_list = redirection_undo_list;

  /* Calling the "exec" builtin changes redirections forever. */
  if (builtin == exec_builtin)
    {
      dispose_redirects (saved_undo_list);
      saved_undo_list = exec_redirection_undo_list;
      exec_redirection_undo_list = (REDIRECT *)NULL;
    }
  else
    dispose_exec_redirects ();

  if (saved_undo_list)
    {
      begin_unwind_frame ("saved redirects");
      add_unwind_protect (cleanup_redirects, (char *)saved_undo_list);
    }

  redirection_undo_list = (REDIRECT *)NULL;

  if (builtin)
    result = execute_builtin (builtin, words, flags, 0);
  else
    result = execute_function (var, words, flags, fds_to_close, 0, 0);

  /* We do this before undoing the effects of any redirections. */
  fflush (stdout);
  fpurge (stdout);
  if (ferror (stdout))
    clearerr (stdout);  

  /* If we are executing the `command' builtin, but this_shell_builtin is
     set to `exec_builtin', we know that we have something like
     `command exec [redirection]', since otherwise `exec' would have
     overwritten the shell and we wouldn't get here.  In this case, we
     want to behave as if the `command' builtin had not been specified
     and preserve the redirections. */
  if (builtin == command_builtin && this_shell_builtin == exec_builtin)
    {
      if (saved_undo_list)
	dispose_redirects (saved_undo_list);
      redirection_undo_list = exec_redirection_undo_list;
      saved_undo_list = exec_redirection_undo_list = (REDIRECT *)NULL;      
      discard_unwind_frame ("saved_redirects");
    }

  if (saved_undo_list)
    {
      redirection_undo_list = saved_undo_list;
      discard_unwind_frame ("saved redirects");
    }

  if (redirection_undo_list)
    {
      cleanup_redirects (redirection_undo_list);
      redirection_undo_list = (REDIRECT *)NULL;
    }

  return (result);
}

void
setup_async_signals ()
{
#if defined (__BEOS__)
  set_signal_handler (SIGHUP, SIG_IGN);	/* they want csh-like behavior */
#endif

#if defined (JOB_CONTROL)
  if (job_control == 0)
#endif
    {
      set_signal_handler (SIGINT, SIG_IGN);
      set_signal_ignored (SIGINT);
      set_signal_handler (SIGQUIT, SIG_IGN);
      set_signal_ignored (SIGQUIT);
    }
}

/* Execute a simple command that is hopefully defined in a disk file
   somewhere.

   1) fork ()
   2) connect pipes
   3) look up the command
   4) do redirections
   5) execve ()
   6) If the execve failed, see if the file has executable mode set.
   If so, and it isn't a directory, then execute its contents as
   a shell script.

   Note that the filename hashing stuff has to take place up here,
   in the parent.  This is probably why the Bourne style shells
   don't handle it, since that would require them to go through
   this gnarly hair, for no good reason.

   NOTE: callers expect this to fork or exit(). */

/* Name of a shell function to call when a command name is not found. */
#ifndef NOTFOUND_HOOK
#  define NOTFOUND_HOOK "command_not_found_handle"
#endif

static void
execute_disk_command (words, redirects, command_line, pipe_in, pipe_out,
		      async, fds_to_close, cmdflags)
     WORD_LIST *words;
     REDIRECT *redirects;
     char *command_line;
     int pipe_in, pipe_out, async;
     struct fd_bitmap *fds_to_close;
     int cmdflags;
{
  char *pathname, *command, **args;
  int nofork;
  pid_t pid;
  SHELL_VAR *hookf;
  WORD_LIST *wl;

  nofork = (cmdflags & CMD_NO_FORK);  /* Don't fork, just exec, if no pipes */
  pathname = words->word->word;

#if defined (RESTRICTED_SHELL)
  command = (char *)NULL;
  if (restricted && mbschr (pathname, '/'))
    {
      internal_error (_("%s: restricted: cannot specify `/' in command names"),
		    pathname);
      last_command_exit_value = EXECUTION_FAILURE;

      /* If we're not going to fork below, we must already be in a child
         process or a context in which it's safe to call exit(2).  */
      if (nofork && pipe_in == NO_PIPE && pipe_out == NO_PIPE)
	exit (last_command_exit_value);
      else
	goto parent_return;
    }
#endif /* RESTRICTED_SHELL */

  command = search_for_command (pathname);

  if (command)
    {
      maybe_make_export_env ();
      put_command_name_into_env (command);
    }

  /* We have to make the child before we check for the non-existence
     of COMMAND, since we want the error messages to be redirected. */
  /* If we can get away without forking and there are no pipes to deal with,
     don't bother to fork, just directly exec the command. */
  if (nofork && pipe_in == NO_PIPE && pipe_out == NO_PIPE)
    pid = 0;
  else
    pid = make_child (savestring (command_line), async);

  if (pid == 0)
    {
      int old_interactive;

#if 0
      /* This has been disabled for the time being. */
#if !defined (ARG_MAX) || ARG_MAX >= 10240
      if (posixly_correct == 0)
	put_gnu_argv_flags_into_env ((long)getpid (), glob_argv_flags);
#endif
#endif

      /* Cancel traps, in trap.c. */
      restore_original_signals ();

      /* restore_original_signals may have undone the work done
	 by make_child to ensure that SIGINT and SIGQUIT are ignored
	 in asynchronous children. */
      if (async)
	{
	  if ((cmdflags & CMD_STDIN_REDIR) &&
		pipe_in == NO_PIPE &&
		(stdin_redirects (redirects) == 0))
	    async_redirect_stdin ();
	  setup_async_signals ();
	}

      /* This functionality is now provided by close-on-exec of the
	 file descriptors manipulated by redirection and piping.
	 Some file descriptors still need to be closed in all children
	 because of the way bash does pipes; fds_to_close is a
	 bitmap of all such file descriptors. */
      if (fds_to_close)
	close_fd_bitmap (fds_to_close);

      do_piping (pipe_in, pipe_out);

      old_interactive = interactive;
      if (async)
	interactive = 0;

      subshell_environment = SUBSHELL_FORK;

      if (redirects && (do_redirections (redirects, RX_ACTIVE) != 0))
	{
#if defined (PROCESS_SUBSTITUTION)
	  /* Try to remove named pipes that may have been created as the
	     result of redirections. */
	  unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */
	  exit (EXECUTION_FAILURE);
	}

      if (async)
	interactive = old_interactive;

      if (command == 0)
	{
	  hookf = find_function (NOTFOUND_HOOK);
	  if (hookf == 0)
	    {
	      internal_error (_("%s: command not found"), pathname);
	      exit (EX_NOTFOUND);	/* Posix.2 says the exit status is 127 */
	    }

	  wl = make_word_list (make_word (NOTFOUND_HOOK), words);
	  exit (execute_shell_function (hookf, wl));
	}

      /* Execve expects the command name to be in args[0].  So we
	 leave it there, in the same format that the user used to
	 type it in. */
      args = strvec_from_word_list (words, 0, 0, (int *)NULL);
      exit (shell_execve (command, args, export_env));
    }
  else
    {
parent_return:
      /* Make sure that the pipes are closed in the parent. */
      close_pipes (pipe_in, pipe_out);
#if defined (PROCESS_SUBSTITUTION) && defined (HAVE_DEV_FD)
      if (variable_context == 0)
        unlink_fifo_list ();
#endif
      FREE (command);
    }
}

/* CPP defines to decide whether a particular index into the #! line
   corresponds to a valid interpreter name or argument character, or
   whitespace.  The MSDOS define is to allow \r to be treated the same
   as \n. */

#if !defined (MSDOS)
#  define STRINGCHAR(ind) \
    (ind < sample_len && !whitespace (sample[ind]) && sample[ind] != '\n')
#  define WHITECHAR(ind) \
    (ind < sample_len && whitespace (sample[ind]))
#else	/* MSDOS */
#  define STRINGCHAR(ind) \
    (ind < sample_len && !whitespace (sample[ind]) && sample[ind] != '\n' && sample[ind] != '\r')
#  define WHITECHAR(ind) \
    (ind < sample_len && whitespace (sample[ind]))
#endif	/* MSDOS */

static char *
getinterp (sample, sample_len, endp)
     char *sample;
     int sample_len, *endp;
{
  register int i;
  char *execname;
  int start;

  /* Find the name of the interpreter to exec. */
  for (i = 2; i < sample_len && whitespace (sample[i]); i++)
    ;

  for (start = i; STRINGCHAR(i); i++)
    ;

  execname = substring (sample, start, i);

  if (endp)
    *endp = i;
  return execname;
}

#if !defined (HAVE_HASH_BANG_EXEC)
/* If the operating system on which we're running does not handle
   the #! executable format, then help out.  SAMPLE is the text read
   from the file, SAMPLE_LEN characters.  COMMAND is the name of
   the script; it and ARGS, the arguments given by the user, will
   become arguments to the specified interpreter.  ENV is the environment
   to pass to the interpreter.

   The word immediately following the #! is the interpreter to execute.
   A single argument to the interpreter is allowed. */

static int
execute_shell_script (sample, sample_len, command, args, env)
     char *sample;
     int sample_len;
     char *command;
     char **args, **env;
{
  char *execname, *firstarg;
  int i, start, size_increment, larry;

  /* Find the name of the interpreter to exec. */
  execname = getinterp (sample, sample_len, &i);
  size_increment = 1;

  /* Now the argument, if any. */
  for (firstarg = (char *)NULL, start = i; WHITECHAR(i); i++)
    ;

  /* If there is more text on the line, then it is an argument for the
     interpreter. */

  if (STRINGCHAR(i))  
    {
      for (start = i; STRINGCHAR(i); i++)
	;
      firstarg = substring ((char *)sample, start, i);
      size_increment = 2;
    }

  larry = strvec_len (args) + size_increment;
  args = strvec_resize (args, larry + 1);

  for (i = larry - 1; i; i--)
    args[i] = args[i - size_increment];

  args[0] = execname;
  if (firstarg)
    {
      args[1] = firstarg;
      args[2] = command;
    }
  else
    args[1] = command;

  args[larry] = (char *)NULL;

  return (shell_execve (execname, args, env));
}
#undef STRINGCHAR
#undef WHITECHAR

#endif /* !HAVE_HASH_BANG_EXEC */

static void
initialize_subshell ()
{
#if defined (ALIAS)
  /* Forget about any aliases that we knew of.  We are in a subshell. */
  delete_all_aliases ();
#endif /* ALIAS */

#if defined (HISTORY)
  /* Forget about the history lines we have read.  This is a non-interactive
     subshell. */
  history_lines_this_session = 0;
#endif

#if defined (JOB_CONTROL)
  /* Forget about the way job control was working. We are in a subshell. */
  without_job_control ();
  set_sigchld_handler ();
  init_job_stats ();
#endif /* JOB_CONTROL */

  /* Reset the values of the shell flags and options. */
  reset_shell_flags ();
  reset_shell_options ();
  reset_shopt_options ();

  /* Zero out builtin_env, since this could be a shell script run from a
     sourced file with a temporary environment supplied to the `source/.'
     builtin.  Such variables are not supposed to be exported (empirical
     testing with sh and ksh).  Just throw it away; don't worry about a
     memory leak. */
  if (vc_isbltnenv (shell_variables))
    shell_variables = shell_variables->down;

  clear_unwind_protect_list (0);
  /* XXX -- are there other things we should be resetting here? */
  parse_and_execute_level = 0;		/* nothing left to restore it */

  /* We're no longer inside a shell function. */
  variable_context = return_catch_flag = funcnest = 0;

  executing_list = 0;		/* XXX */

  /* If we're not interactive, close the file descriptor from which we're
     reading the current shell script. */
  if (interactive_shell == 0)
    unset_bash_input (0);
}

#if defined (HAVE_SETOSTYPE) && defined (_POSIX_SOURCE)
#  define SETOSTYPE(x)	__setostype(x)
#else
#  define SETOSTYPE(x)
#endif

#define READ_SAMPLE_BUF(file, buf, len) \
  do \
    { \
      fd = open(file, O_RDONLY); \
      if (fd >= 0) \
	{ \
	  len = read (fd, buf, 80); \
	  close (fd); \
	} \
      else \
	len = -1; \
    } \
  while (0)
      
/* Call execve (), handling interpreting shell scripts, and handling
   exec failures. */
int
shell_execve (command, args, env)
     char *command;
     char **args, **env;
{
  int larray, i, fd;
  char sample[80];
  int sample_len;

  SETOSTYPE (0);		/* Some systems use for USG/POSIX semantics */
  execve (command, args, env);
  i = errno;			/* error from execve() */
  CHECK_TERMSIG;
  SETOSTYPE (1);

  /* If we get to this point, then start checking out the file.
     Maybe it is something we can hack ourselves. */
  if (i != ENOEXEC)
    {
      if (file_isdir (command))
	internal_error (_("%s: is a directory"), command);
      else if (executable_file (command) == 0)
	{
	  errno = i;
	  file_error (command);
	}
      /* errors not involving the path argument to execve. */
      else if (i == E2BIG || i == ENOMEM)
	{
	  errno = i;
	  file_error (command);
	}
      else
	{
	  /* The file has the execute bits set, but the kernel refuses to
	     run it for some reason.  See why. */
#if defined (HAVE_HASH_BANG_EXEC)
	  READ_SAMPLE_BUF (command, sample, sample_len);
	  if (sample_len > 2 && sample[0] == '#' && sample[1] == '!')
	    {
	      char *interp;
	      int ilen;

	      interp = getinterp (sample, sample_len, (int *)NULL);
	      ilen = strlen (interp);
	      errno = i;
	      if (interp[ilen - 1] == '\r')
		{
		  interp = xrealloc (interp, ilen + 2);
		  interp[ilen - 1] = '^';
		  interp[ilen] = 'M';
		  interp[ilen + 1] = '\0';
		}
	      sys_error (_("%s: %s: bad interpreter"), command, interp ? interp : "");
	      FREE (interp);
	      return (EX_NOEXEC);
	    }
#endif
	  errno = i;
	  file_error (command);
	}
      return ((i == ENOENT) ? EX_NOTFOUND : EX_NOEXEC);	/* XXX Posix.2 says that exit status is 126 */
    }

  /* This file is executable.
     If it begins with #!, then help out people with losing operating
     systems.  Otherwise, check to see if it is a binary file by seeing
     if the contents of the first line (or up to 80 characters) are in the
     ASCII set.  If it's a text file, execute the contents as shell commands,
     otherwise return 126 (EX_BINARY_FILE). */
  READ_SAMPLE_BUF (command, sample, sample_len);

  if (sample_len == 0)
    return (EXECUTION_SUCCESS);

  /* Is this supposed to be an executable script?
     If so, the format of the line is "#! interpreter [argument]".
     A single argument is allowed.  The BSD kernel restricts
     the length of the entire line to 32 characters (32 bytes
     being the size of the BSD exec header), but we allow 80
     characters. */
  if (sample_len > 0)
    {
#if !defined (HAVE_HASH_BANG_EXEC)
      if (sample_len > 2 && sample[0] == '#' && sample[1] == '!')
	return (execute_shell_script (sample, sample_len, command, args, env));
      else
#endif
      if (check_binary_file (sample, sample_len))
	{
	  internal_error (_("%s: cannot execute binary file"), command);
	  return (EX_BINARY_FILE);
	}
    }

  /* We have committed to attempting to execute the contents of this file
     as shell commands. */

  initialize_subshell ();

  set_sigint_handler ();

  /* Insert the name of this shell into the argument list. */
  larray = strvec_len (args) + 1;
  args = strvec_resize (args, larray + 1);

  for (i = larray - 1; i; i--)
    args[i] = args[i - 1];

  args[0] = shell_name;
  args[1] = command;
  args[larray] = (char *)NULL;

  if (args[0][0] == '-')
    args[0]++;

#if defined (RESTRICTED_SHELL)
  if (restricted)
    change_flag ('r', FLAG_OFF);
#endif

  if (subshell_argv)
    {
      /* Can't free subshell_argv[0]; that is shell_name. */
      for (i = 1; i < subshell_argc; i++)
	free (subshell_argv[i]);
      free (subshell_argv);
    }

  dispose_command (currently_executing_command);	/* XXX */
  currently_executing_command = (COMMAND *)NULL;

  subshell_argc = larray;
  subshell_argv = args;
  subshell_envp = env;

  unbind_args ();	/* remove the positional parameters */

  longjmp (subshell_top_level, 1);
  /*NOTREACHED*/
}

static int
execute_intern_function (name, function)
     WORD_DESC *name;
     COMMAND *function;
{
  SHELL_VAR *var;

  if (check_identifier (name, posixly_correct) == 0)
    {
      if (posixly_correct && interactive_shell == 0)
	{
	  last_command_exit_value = EX_BADUSAGE;
	  jump_to_top_level (ERREXIT);
	}
      return (EXECUTION_FAILURE);
    }

  var = find_function (name->word);
  if (var && (readonly_p (var) || noassign_p (var)))
    {
      if (readonly_p (var))
	internal_error (_("%s: readonly function"), var->name);
      return (EXECUTION_FAILURE);
    }

  bind_function (name->word, function);
  return (EXECUTION_SUCCESS);
}

#if defined (INCLUDE_UNUSED)
#if defined (PROCESS_SUBSTITUTION)
void
close_all_files ()
{
  register int i, fd_table_size;

  fd_table_size = getdtablesize ();
  if (fd_table_size > 256)	/* clamp to a reasonable value */
    fd_table_size = 256;

  for (i = 3; i < fd_table_size; i++)
    close (i);
}
#endif /* PROCESS_SUBSTITUTION */
#endif

static void
close_pipes (in, out)
     int in, out;
{
  if (in >= 0)
    close (in);
  if (out >= 0)
    close (out);
}

static void
dup_error (oldd, newd)
     int oldd, newd;
{
  sys_error (_("cannot duplicate fd %d to fd %d"), oldd, newd);
}

/* Redirect input and output to be from and to the specified pipes.
   NO_PIPE and REDIRECT_BOTH are handled correctly. */
static void
do_piping (pipe_in, pipe_out)
     int pipe_in, pipe_out;
{
  if (pipe_in != NO_PIPE)
    {
      if (dup2 (pipe_in, 0) < 0)
	dup_error (pipe_in, 0);
      if (pipe_in > 0)
	close (pipe_in);
    }
  if (pipe_out != NO_PIPE)
    {
      if (pipe_out != REDIRECT_BOTH)
	{
	  if (dup2 (pipe_out, 1) < 0)
	    dup_error (pipe_out, 1);
	  if (pipe_out == 0 || pipe_out > 1)
	    close (pipe_out);
	}
      else
	{
	  if (dup2 (1, 2) < 0)
	    dup_error (1, 2);
	}
    }
}
