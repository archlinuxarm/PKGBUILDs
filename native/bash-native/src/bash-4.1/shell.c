/* shell.c -- GNU's idea of the POSIX shell specification. */

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

/*
  Birthdate:
  Sunday, January 10th, 1988.
  Initial author: Brian Fox
*/
#define INSTALL_DEBUG_MODE

#include "config.h"

#include "bashtypes.h"
#if !defined (_MINIX) && defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif
#include "posixstat.h"
#include "posixtime.h"
#include "bashansi.h"
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include "filecntl.h"
#include <pwd.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "bashintl.h"

#define NEED_SH_SETLINEBUF_DECL		/* used in externs.h */

#include "shell.h"
#include "flags.h"
#include "trap.h"
#include "mailcheck.h"
#include "builtins.h"
#include "builtins/common.h"

#if defined (JOB_CONTROL)
#include "jobs.h"
#endif /* JOB_CONTROL */

#include "input.h"
#include "execute_cmd.h"
#include "findcmd.h"

#if defined (USING_BASH_MALLOC) && defined (DEBUG) && !defined (DISABLE_MALLOC_WRAPPERS)
#  include <malloc/shmalloc.h>
#endif

#if defined (HISTORY)
#  include "bashhist.h"
#  include <readline/history.h>
#endif

#if defined (READLINE)
#  include "bashline.h"
#endif

#include <tilde/tilde.h>
#include <glob/strmatch.h>

#if defined (__OPENNT)
#  include <opennt/opennt.h>
#endif

#if !defined (HAVE_GETPW_DECLS)
extern struct passwd *getpwuid ();
#endif /* !HAVE_GETPW_DECLS */

#if !defined (errno)
extern int errno;
#endif

#if defined (NO_MAIN_ENV_ARG)
extern char **environ;	/* used if no third argument to main() */
#endif

extern char *dist_version, *release_status;
extern int patch_level, build_version;
extern int shell_level;
extern int subshell_environment;
extern int last_command_exit_value;
extern int line_number;
extern int expand_aliases;
extern int array_needs_making;
extern int gnu_error_format;
extern char *primary_prompt, *secondary_prompt;
extern char *this_command_name;

/* Non-zero means that this shell has already been run; i.e. you should
   call shell_reinitialize () if you need to start afresh. */
int shell_initialized = 0;

COMMAND *global_command = (COMMAND *)NULL;

/* Information about the current user. */
struct user_info current_user =
{
  (uid_t)-1, (uid_t)-1, (gid_t)-1, (gid_t)-1,
  (char *)NULL, (char *)NULL, (char *)NULL
};

/* The current host's name. */
char *current_host_name = (char *)NULL;

/* Non-zero means that this shell is a login shell.
   Specifically:
   0 = not login shell.
   1 = login shell from getty (or equivalent fake out)
  -1 = login shell from "--login" (or -l) flag.
  -2 = both from getty, and from flag.
 */
int login_shell = 0;

/* Non-zero means that at this moment, the shell is interactive.  In
   general, this means that the shell is at this moment reading input
   from the keyboard. */
int interactive = 0;

/* Non-zero means that the shell was started as an interactive shell. */
int interactive_shell = 0;

/* Non-zero means to send a SIGHUP to all jobs when an interactive login
   shell exits. */
int hup_on_exit = 0;

/* Non-zero means to list status of running and stopped jobs at shell exit */
int check_jobs_at_exit = 0;

/* Non-zero means to change to a directory name supplied as a command name */
int autocd = 0;

/* Tells what state the shell was in when it started:
	0 = non-interactive shell script
	1 = interactive
	2 = -c command
	3 = wordexp evaluation
   This is a superset of the information provided by interactive_shell.
*/
int startup_state = 0;

/* Special debugging helper. */
int debugging_login_shell = 0;

/* The environment that the shell passes to other commands. */
char **shell_environment;

/* Non-zero when we are executing a top-level command. */
int executing = 0;

/* The number of commands executed so far. */
int current_command_number = 1;

/* Non-zero is the recursion depth for commands. */
int indirection_level = 0;

/* The name of this shell, as taken from argv[0]. */
char *shell_name = (char *)NULL;

/* time in seconds when the shell was started */
time_t shell_start_time;

/* Are we running in an emacs shell window? */
int running_under_emacs;

/* Do we have /dev/fd? */
#ifdef HAVE_DEV_FD
int have_devfd = HAVE_DEV_FD;
#else
int have_devfd = 0;
#endif

/* The name of the .(shell)rc file. */
static char *bashrc_file = "~/.bashrc";

/* Non-zero means to act more like the Bourne shell on startup. */
static int act_like_sh;

/* Non-zero if this shell is being run by `su'. */
static int su_shell;

/* Non-zero if we have already expanded and sourced $ENV. */
static int sourced_env;

/* Is this shell running setuid? */
static int running_setuid;

/* Values for the long-winded argument names. */
static int debugging;			/* Do debugging things. */
static int no_rc;			/* Don't execute ~/.bashrc */
static int no_profile;			/* Don't execute .profile */
static int do_version;			/* Display interesting version info. */
static int make_login_shell;		/* Make this shell be a `-bash' shell. */
static int want_initial_help;		/* --help option */

int debugging_mode = 0;		/* In debugging mode with --debugger */
int no_line_editing = 0;	/* Don't do fancy line editing. */
int dump_translatable_strings;	/* Dump strings in $"...", don't execute. */
int dump_po_strings;		/* Dump strings in $"..." in po format */
int wordexp_only = 0;		/* Do word expansion only */
int protected_mode = 0;		/* No command substitution with --wordexp */

#if defined (STRICT_POSIX)
int posixly_correct = 1;	/* Non-zero means posix.2 superset. */
#else
int posixly_correct = 0;	/* Non-zero means posix.2 superset. */
#endif


/* Some long-winded argument names.  These are obviously new. */
#define Int 1
#define Charp 2
static const struct {
  const char *name;
  int type;
  int *int_value;
  char **char_value;
} long_args[] = {
  { "debug", Int, &debugging, (char **)0x0 },
#if defined (DEBUGGER)
  { "debugger", Int, &debugging_mode, (char **)0x0 },
#endif
  { "dump-po-strings", Int, &dump_po_strings, (char **)0x0 },
  { "dump-strings", Int, &dump_translatable_strings, (char **)0x0 },
  { "help", Int, &want_initial_help, (char **)0x0 },
  { "init-file", Charp, (int *)0x0, &bashrc_file },
  { "login", Int, &make_login_shell, (char **)0x0 },
  { "noediting", Int, &no_line_editing, (char **)0x0 },
  { "noprofile", Int, &no_profile, (char **)0x0 },
  { "norc", Int, &no_rc, (char **)0x0 },
  { "posix", Int, &posixly_correct, (char **)0x0 },
  { "protected", Int, &protected_mode, (char **)0x0 },
  { "rcfile", Charp, (int *)0x0, &bashrc_file },
#if defined (RESTRICTED_SHELL)
  { "restricted", Int, &restricted, (char **)0x0 },
#endif
  { "verbose", Int, &echo_input_at_read, (char **)0x0 },
  { "version", Int, &do_version, (char **)0x0 },
#if defined (WORDEXP_OPTION)
  { "wordexp", Int, &wordexp_only, (char **)0x0 },
#endif
  { (char *)0x0, Int, (int *)0x0, (char **)0x0 }
};

/* These are extern so execute_simple_command can set them, and then
   longjmp back to main to execute a shell script, instead of calling
   main () again and resulting in indefinite, possibly fatal, stack
   growth. */
procenv_t subshell_top_level;
int subshell_argc;
char **subshell_argv;
char **subshell_envp;

#if defined (BUFFERED_INPUT)
/* The file descriptor from which the shell is reading input. */
int default_buffered_input = -1;
#endif

/* The following two variables are not static so they can show up in $-. */
int read_from_stdin;		/* -s flag supplied */
int want_pending_command;	/* -c flag supplied */

/* This variable is not static so it can be bound to $BASH_EXECUTION_STRING */
char *command_execution_string;	/* argument to -c option */

int malloc_trace_at_exit = 0;

static int shell_reinitialized = 0;

static FILE *default_input;

static STRING_INT_ALIST *shopt_alist;
static int shopt_ind = 0, shopt_len = 0;

static int parse_long_options __P((char **, int, int));
static int parse_shell_options __P((char **, int, int));
static int bind_args __P((char **, int, int, int));

static void start_debugger __P((void));

static void add_shopt_to_alist __P((char *, int));
static void run_shopt_alist __P((void));

static void execute_env_file __P((char *));
static void run_startup_files __P((void));
static int open_shell_script __P((char *));
static void set_bash_input __P((void));
static int run_one_command __P((char *));
#if defined (WORDEXP_OPTION)
static int run_wordexp __P((char *));
#endif

static int uidget __P((void));

static void init_interactive __P((void));
static void init_noninteractive __P((void));
static void init_interactive_script __P((void));

static void set_shell_name __P((char *));
static void shell_initialize __P((void));
static void shell_reinitialize __P((void));

static void show_shell_usage __P((FILE *, int));

#ifdef __CYGWIN__
static void
_cygwin32_check_tmp ()
{
  struct stat sb;

  if (stat ("/tmp", &sb) < 0)
    internal_warning (_("could not find /tmp, please create!"));
  else
    {
      if (S_ISDIR (sb.st_mode) == 0)
	internal_warning (_("/tmp must be a valid directory name"));
    }
}
#endif /* __CYGWIN__ */

#if defined (NO_MAIN_ENV_ARG)
/* systems without third argument to main() */
int
main (argc, argv)
     int argc;
     char **argv;
#else /* !NO_MAIN_ENV_ARG */
int
main (argc, argv, env)
     int argc;
     char **argv, **env;
#endif /* !NO_MAIN_ENV_ARG */
{
  register int i;
  int code, old_errexit_flag;
#if defined (RESTRICTED_SHELL)
  int saverst;
#endif
  volatile int locally_skip_execution;
  volatile int arg_index, top_level_arg_index;
#ifdef __OPENNT
  char **env;

  env = environ;
#endif /* __OPENNT */

  USE_VAR(argc);
  USE_VAR(argv);
  USE_VAR(env);
  USE_VAR(code);
  USE_VAR(old_errexit_flag);
#if defined (RESTRICTED_SHELL)
  USE_VAR(saverst);
#endif

  /* Catch early SIGINTs. */
  code = setjmp (top_level);
  if (code)
    exit (2);

  xtrace_init ();

#if defined (USING_BASH_MALLOC) && defined (DEBUG) && !defined (DISABLE_MALLOC_WRAPPERS)
#  if 1
  malloc_set_register (1);
#  endif
#endif

  check_dev_tty ();

#ifdef __CYGWIN__
  _cygwin32_check_tmp ();
#endif /* __CYGWIN__ */

  /* Wait forever if we are debugging a login shell. */
  while (debugging_login_shell) sleep (3);

  set_default_locale ();

  running_setuid = uidget ();

  if (getenv ("POSIXLY_CORRECT") || getenv ("POSIX_PEDANTIC"))
    posixly_correct = 1;

#if defined (USE_GNU_MALLOC_LIBRARY)
  mcheck (programming_error, (void (*) ())0);
#endif /* USE_GNU_MALLOC_LIBRARY */

  if (setjmp (subshell_top_level))
    {
      argc = subshell_argc;
      argv = subshell_argv;
      env = subshell_envp;
      sourced_env = 0;
    }

  shell_reinitialized = 0;

  /* Initialize `local' variables for all `invocations' of main (). */
  arg_index = 1;
  if (arg_index > argc)
    arg_index = argc;
  command_execution_string = (char *)NULL;
  want_pending_command = locally_skip_execution = read_from_stdin = 0;
  default_input = stdin;
#if defined (BUFFERED_INPUT)
  default_buffered_input = -1;
#endif

  /* Fix for the `infinite process creation' bug when running shell scripts
     from startup files on System V. */
  login_shell = make_login_shell = 0;

  /* If this shell has already been run, then reinitialize it to a
     vanilla state. */
  if (shell_initialized || shell_name)
    {
      /* Make sure that we do not infinitely recurse as a login shell. */
      if (*shell_name == '-')
	shell_name++;

      shell_reinitialize ();
      if (setjmp (top_level))
	exit (2);
    }

  shell_environment = env;
  set_shell_name (argv[0]);
  shell_start_time = NOW;	/* NOW now defined in general.h */

  /* Parse argument flags from the input line. */

  /* Find full word arguments first. */
  arg_index = parse_long_options (argv, arg_index, argc);

  if (want_initial_help)
    {
      show_shell_usage (stdout, 1);
      exit (EXECUTION_SUCCESS);
    }

  if (do_version)
    {
      show_shell_version (1);
      exit (EXECUTION_SUCCESS);
    }

  /* All done with full word options; do standard shell option parsing.*/
  this_command_name = shell_name;	/* for error reporting */
  arg_index = parse_shell_options (argv, arg_index, argc);

  /* If user supplied the "--login" (or -l) flag, then set and invert
     LOGIN_SHELL. */
  if (make_login_shell)
    {
      login_shell++;
      login_shell = -login_shell;
    }

  set_login_shell ("login_shell", login_shell != 0);

  if (dump_po_strings)
    dump_translatable_strings = 1;

  if (dump_translatable_strings)
    read_but_dont_execute = 1;

  if (running_setuid && privileged_mode == 0)
    disable_priv_mode ();

  /* Need to get the argument to a -c option processed in the
     above loop.  The next arg is a command to execute, and the
     following args are $0...$n respectively. */
  if (want_pending_command)
    {
      command_execution_string = argv[arg_index];
      if (command_execution_string == 0)
	{
	  report_error (_("%s: option requires an argument"), "-c");
	  exit (EX_BADUSAGE);
	}
      arg_index++;
    }
  this_command_name = (char *)NULL;

  cmd_init();		/* initialize the command object caches */

  /* First, let the outside world know about our interactive status.
     A shell is interactive if the `-i' flag was given, or if all of
     the following conditions are met:
	no -c command
	no arguments remaining or the -s flag given
	standard input is a terminal
	standard error is a terminal
     Refer to Posix.2, the description of the `sh' utility. */

  if (forced_interactive ||		/* -i flag */
      (!command_execution_string &&	/* No -c command and ... */
       wordexp_only == 0 &&		/* No --wordexp and ... */
       ((arg_index == argc) ||		/*   no remaining args or... */
	read_from_stdin) &&		/*   -s flag with args, and */
       isatty (fileno (stdin)) &&	/* Input is a terminal and */
       isatty (fileno (stderr))))	/* error output is a terminal. */
    init_interactive ();
  else
    init_noninteractive ();

#define CLOSE_FDS_AT_LOGIN
#if defined (CLOSE_FDS_AT_LOGIN)
  /*
   * Some systems have the bad habit of starting login shells with lots of open
   * file descriptors.  For instance, most systems that have picked up the
   * pre-4.0 Sun YP code leave a file descriptor open each time you call one
   * of the getpw* functions, and it's set to be open across execs.  That
   * means one for login, one for xterm, one for shelltool, etc.
   */
  if (login_shell && interactive_shell)
    {
      for (i = 3; i < 20; i++)
	close (i);
    }
#endif /* CLOSE_FDS_AT_LOGIN */

  /* If we're in a strict Posix.2 mode, turn on interactive comments,
     alias expansion in non-interactive shells, and other Posix.2 things. */
  if (posixly_correct)
    {
      bind_variable ("POSIXLY_CORRECT", "y", 0);
      sv_strict_posix ("POSIXLY_CORRECT");
    }

  /* Now we run the shopt_alist and process the options. */
  if (shopt_alist)
    run_shopt_alist ();

  /* From here on in, the shell must be a normal functioning shell.
     Variables from the environment are expected to be set, etc. */
  shell_initialize ();

  set_default_lang ();
  set_default_locale_vars ();

  /*
   * M-x term -> TERM=eterm EMACS=22.1 (term:0.96)	(eterm)
   * M-x shell -> TERM=dumb EMACS=t			(no line editing)
   * M-x terminal -> TERM=emacs-em7955 EMACS=		(line editing)
   */
  if (interactive_shell)
    {
      char *term, *emacs;

      term = get_string_value ("TERM");
      emacs = get_string_value ("EMACS");

      /* Not sure any emacs terminal emulator sets TERM=emacs any more */
      no_line_editing |= term && (STREQ (term, "emacs"));
      no_line_editing |= emacs && emacs[0] == 't' && emacs[1] == '\0' && STREQ (term, "dumb");

      /* running_under_emacs == 2 for `eterm' */
      running_under_emacs = (emacs != 0) || (term && STREQN (term, "emacs", 5));
      running_under_emacs += term && STREQN (term, "eterm", 5) && emacs && strstr (emacs, "term");

      if (running_under_emacs)
	gnu_error_format = 1;
    }

  top_level_arg_index = arg_index;
  old_errexit_flag = exit_immediately_on_error;

  /* Give this shell a place to longjmp to before executing the
     startup files.  This allows users to press C-c to abort the
     lengthy startup. */
  code = setjmp (top_level);
  if (code)
    {
      if (code == EXITPROG || code == ERREXIT)
	exit_shell (last_command_exit_value);
      else
	{
#if defined (JOB_CONTROL)
	  /* Reset job control, since run_startup_files turned it off. */
	  set_job_control (interactive_shell);
#endif
	  /* Reset value of `set -e', since it's turned off before running
	     the startup files. */
	  exit_immediately_on_error += old_errexit_flag;
	  locally_skip_execution++;
	}
    }

  arg_index = top_level_arg_index;

  /* Execute the start-up scripts. */

  if (interactive_shell == 0)
    {
      unbind_variable ("PS1");
      unbind_variable ("PS2");
      interactive = 0;
#if 0
      /* This has already been done by init_noninteractive */
      expand_aliases = posixly_correct;
#endif
    }
  else
    {
      change_flag ('i', FLAG_ON);
      interactive = 1;
    }

#if defined (RESTRICTED_SHELL)
  /* Set restricted_shell based on whether the basename of $0 indicates that
     the shell should be restricted or if the `-r' option was supplied at
     startup. */
  restricted_shell = shell_is_restricted (shell_name);

  /* If the `-r' option is supplied at invocation, make sure that the shell
     is not in restricted mode when running the startup files. */
  saverst = restricted;
  restricted = 0;
#endif

  /* The startup files are run with `set -e' temporarily disabled. */
  if (locally_skip_execution == 0 && running_setuid == 0)
    {
      old_errexit_flag = exit_immediately_on_error;
      exit_immediately_on_error = 0;

      run_startup_files ();
      exit_immediately_on_error += old_errexit_flag;
    }

  /* If we are invoked as `sh', turn on Posix mode. */
  if (act_like_sh)
    {
      bind_variable ("POSIXLY_CORRECT", "y", 0);
      sv_strict_posix ("POSIXLY_CORRECT");
    }

#if defined (RESTRICTED_SHELL)
  /* Turn on the restrictions after executing the startup files.  This
     means that `bash -r' or `set -r' invoked from a startup file will
     turn on the restrictions after the startup files are executed. */
  restricted = saverst || restricted;
  if (shell_reinitialized == 0)
    maybe_make_restricted (shell_name);
#endif /* RESTRICTED_SHELL */

#if defined (WORDEXP_OPTION)
  if (wordexp_only)
    {
      startup_state = 3;
      last_command_exit_value = run_wordexp (argv[arg_index]);
      exit_shell (last_command_exit_value);
    }
#endif

  if (command_execution_string)
    {
      arg_index = bind_args (argv, arg_index, argc, 0);
      startup_state = 2;

      if (debugging_mode)
	start_debugger ();

#if defined (ONESHOT)
      executing = 1;
      run_one_command (command_execution_string);
      exit_shell (last_command_exit_value);
#else /* ONESHOT */
      with_input_from_string (command_execution_string, "-c");
      goto read_and_execute;
#endif /* !ONESHOT */
    }

  /* Get possible input filename and set up default_buffered_input or
     default_input as appropriate. */
  if (arg_index != argc && read_from_stdin == 0)
    {
      open_shell_script (argv[arg_index]);
      arg_index++;
    }
  else if (interactive == 0)
    /* In this mode, bash is reading a script from stdin, which is a
       pipe or redirected file. */
#if defined (BUFFERED_INPUT)
    default_buffered_input = fileno (stdin);	/* == 0 */
#else
    setbuf (default_input, (char *)NULL);
#endif /* !BUFFERED_INPUT */

  set_bash_input ();

  /* Bind remaining args to $1 ... $n */
  arg_index = bind_args (argv, arg_index, argc, 1);

  if (debugging_mode && locally_skip_execution == 0 && running_setuid == 0)
    start_debugger ();

  /* Do the things that should be done only for interactive shells. */
  if (interactive_shell)
    {
      /* Set up for checking for presence of mail. */
      reset_mail_timer ();
      init_mail_dates ();

#if defined (HISTORY)
      /* Initialize the interactive history stuff. */
      bash_initialize_history ();
      /* Don't load the history from the history file if we've already
	 saved some lines in this session (e.g., by putting `history -s xx'
	 into one of the startup files). */
      if (shell_initialized == 0 && history_lines_this_session == 0)
	load_history ();
#endif /* HISTORY */

      /* Initialize terminal state for interactive shells after the
	 .bash_profile and .bashrc are interpreted. */
      get_tty_state ();
    }

#if !defined (ONESHOT)
 read_and_execute:
#endif /* !ONESHOT */

  shell_initialized = 1;

  /* Read commands until exit condition. */
  reader_loop ();
  exit_shell (last_command_exit_value);
}

static int
parse_long_options (argv, arg_start, arg_end)
     char **argv;
     int arg_start, arg_end;
{
  int arg_index, longarg, i;
  char *arg_string;

  arg_index = arg_start;
  while ((arg_index != arg_end) && (arg_string = argv[arg_index]) &&
	 (*arg_string == '-'))
    {
      longarg = 0;

      /* Make --login equivalent to -login. */
      if (arg_string[1] == '-' && arg_string[2])
	{
	  longarg = 1;
	  arg_string++;
	}

      for (i = 0; long_args[i].name; i++)
	{
	  if (STREQ (arg_string + 1, long_args[i].name))
	    {
	      if (long_args[i].type == Int)
		*long_args[i].int_value = 1;
	      else if (argv[++arg_index] == 0)
		{
		  report_error (_("%s: option requires an argument"), long_args[i].name);
		  exit (EX_BADUSAGE);
		}
	      else
		*long_args[i].char_value = argv[arg_index];

	      break;
	    }
	}
      if (long_args[i].name == 0)
	{
	  if (longarg)
	    {
	      report_error (_("%s: invalid option"), argv[arg_index]);
	      show_shell_usage (stderr, 0);
	      exit (EX_BADUSAGE);
	    }
	  break;		/* No such argument.  Maybe flag arg. */
	}

      arg_index++;
    }

  return (arg_index);
}

static int
parse_shell_options (argv, arg_start, arg_end)
     char **argv;
     int arg_start, arg_end;
{
  int arg_index;
  int arg_character, on_or_off, next_arg, i;
  char *o_option, *arg_string;

  arg_index = arg_start;
  while (arg_index != arg_end && (arg_string = argv[arg_index]) &&
	 (*arg_string == '-' || *arg_string == '+'))
    {
      /* There are flag arguments, so parse them. */
      next_arg = arg_index + 1;

      /* A single `-' signals the end of options.  From the 4.3 BSD sh.
	 An option `--' means the same thing; this is the standard
	 getopt(3) meaning. */
      if (arg_string[0] == '-' &&
	   (arg_string[1] == '\0' ||
	     (arg_string[1] == '-' && arg_string[2] == '\0')))
	return (next_arg);

      i = 1;
      on_or_off = arg_string[0];
      while (arg_character = arg_string[i++])
	{
	  switch (arg_character)
	    {
	    case 'c':
	      want_pending_command = 1;
	      break;

	    case 'l':
	      make_login_shell = 1;
	      break;

	    case 's':
	      read_from_stdin = 1;
	      break;

	    case 'o':
	      o_option = argv[next_arg];
	      if (o_option == 0)
		{
		  list_minus_o_opts (-1, (on_or_off == '-') ? 0 : 1);
		  break;
		}
	      if (set_minus_o_option (on_or_off, o_option) != EXECUTION_SUCCESS)
		exit (EX_BADUSAGE);
	      next_arg++;
	      break;

	    case 'O':
	      /* Since some of these can be overridden by the normal
		 interactive/non-interactive shell initialization or
		 initializing posix mode, we save the options and process
		 them after initialization. */
	      o_option = argv[next_arg];
	      if (o_option == 0)
		{
		  shopt_listopt (o_option, (on_or_off == '-') ? 0 : 1);
		  break;
		}
	      add_shopt_to_alist (o_option, on_or_off);
	      next_arg++;
	      break;

	    case 'D':
	      dump_translatable_strings = 1;
	      break;

	    default:
	      if (change_flag (arg_character, on_or_off) == FLAG_ERROR)
		{
		  report_error (_("%c%c: invalid option"), on_or_off, arg_character);
		  show_shell_usage (stderr, 0);
		  exit (EX_BADUSAGE);
		}
	    }
	}
      /* Can't do just a simple increment anymore -- what about
	 "bash -abouo emacs ignoreeof -hP"? */
      arg_index = next_arg;
    }

  return (arg_index);
}

/* Exit the shell with status S. */
void
exit_shell (s)
     int s;
{
  fflush (stdout);		/* XXX */
  fflush (stderr);

  /* Do trap[0] if defined.  Allow it to override the exit status
     passed to us. */
  if (signal_is_trapped (0))
    s = run_exit_trap ();

#if defined (PROCESS_SUBSTITUTION)
  unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */

#if defined (HISTORY)
  if (interactive_shell)
    maybe_save_shell_history ();
#endif /* HISTORY */

#if defined (COPROCESS_SUPPORT)
  coproc_flush ();
#endif

#if defined (JOB_CONTROL)
  /* If the user has run `shopt -s huponexit', hangup all jobs when we exit
     an interactive login shell.  ksh does this unconditionally. */
  if (interactive_shell && login_shell && hup_on_exit)
    hangup_all_jobs ();

  /* If this shell is interactive, terminate all stopped jobs and
     restore the original terminal process group.  Don't do this if we're
     in a subshell and calling exit_shell after, for example, a failed
     word expansion. */
  if (subshell_environment == 0)
    end_job_control ();
#endif /* JOB_CONTROL */

  /* Always return the exit status of the last command to our parent. */
  sh_exit (s);
}

/* A wrapper for exit that (optionally) can do other things, like malloc
   statistics tracing. */
void
sh_exit (s)
     int s;
{
#if defined (MALLOC_DEBUG) && defined (USING_BASH_MALLOC)
  if (malloc_trace_at_exit)
    trace_malloc_stats (get_name_for_error (), (char *)NULL);
#endif

  exit (s);
}

/* Source the bash startup files.  If POSIXLY_CORRECT is non-zero, we obey
   the Posix.2 startup file rules:  $ENV is expanded, and if the file it
   names exists, that file is sourced.  The Posix.2 rules are in effect
   for interactive shells only. (section 4.56.5.3) */

/* Execute ~/.bashrc for most shells.  Never execute it if
   ACT_LIKE_SH is set, or if NO_RC is set.

   If the executable file "/usr/gnu/src/bash/foo" contains:

   #!/usr/gnu/bin/bash
   echo hello

   then:

	 COMMAND	    EXECUTE BASHRC
	 --------------------------------
	 bash -c foo		NO
	 bash foo		NO
	 foo			NO
	 rsh machine ls		YES (for rsh, which calls `bash -c')
	 rsh machine foo	YES (for shell started by rsh) NO (for foo!)
	 echo ls | bash		NO
	 login			NO
	 bash			YES
*/

static void
execute_env_file (env_file)
      char *env_file;
{
  char *fn;

  if (env_file && *env_file)
    {
      fn = expand_string_unsplit_to_string (env_file, Q_DOUBLE_QUOTES);
      if (fn && *fn)
	maybe_execute_file (fn, 1);
      FREE (fn);
    }
}

static void
run_startup_files ()
{
#if defined (JOB_CONTROL)
  int old_job_control;
#endif
  int sourced_login, run_by_ssh;

  /* get the rshd/sshd case out of the way first. */
  if (interactive_shell == 0 && no_rc == 0 && login_shell == 0 &&
      act_like_sh == 0 && command_execution_string)
    {
#ifdef SSH_SOURCE_BASHRC
      run_by_ssh = (find_variable ("SSH_CLIENT") != (SHELL_VAR *)0) ||
		   (find_variable ("SSH2_CLIENT") != (SHELL_VAR *)0);
#else
      run_by_ssh = 0;
#endif

      /* If we were run by sshd or we think we were run by rshd, execute
	 ~/.bashrc if we are a top-level shell. */
      if ((run_by_ssh || isnetconn (fileno (stdin))) && shell_level < 2)
	{
#ifdef SYS_BASHRC
#  if defined (__OPENNT)
	  maybe_execute_file (_prefixInstallPath(SYS_BASHRC, NULL, 0), 1);
#  else
	  maybe_execute_file (SYS_BASHRC, 1);
#  endif
#endif
	  maybe_execute_file (bashrc_file, 1);
	  return;
	}
    }

#if defined (JOB_CONTROL)
  /* Startup files should be run without job control enabled. */
  old_job_control = interactive_shell ? set_job_control (0) : 0;
#endif

  sourced_login = 0;

  /* A shell begun with the --login (or -l) flag that is not in posix mode
     runs the login shell startup files, no matter whether or not it is
     interactive.  If NON_INTERACTIVE_LOGIN_SHELLS is defined, run the
     startup files if argv[0][0] == '-' as well. */
#if defined (NON_INTERACTIVE_LOGIN_SHELLS)
  if (login_shell && posixly_correct == 0)
#else
  if (login_shell < 0 && posixly_correct == 0)
#endif
    {
      /* We don't execute .bashrc for login shells. */
      no_rc++;

      /* Execute /etc/profile and one of the personal login shell
	 initialization files. */
      if (no_profile == 0)
	{
	  maybe_execute_file (SYS_PROFILE, 1);

	  if (act_like_sh)	/* sh */
	    maybe_execute_file ("~/.profile", 1);
	  else if ((maybe_execute_file ("~/.bash_profile", 1) == 0) &&
		   (maybe_execute_file ("~/.bash_login", 1) == 0))	/* bash */
	    maybe_execute_file ("~/.profile", 1);
	}

      sourced_login = 1;
    }

  /* A non-interactive shell not named `sh' and not in posix mode reads and
     executes commands from $BASH_ENV.  If `su' starts a shell with `-c cmd'
     and `-su' as the name of the shell, we want to read the startup files.
     No other non-interactive shells read any startup files. */
  if (interactive_shell == 0 && !(su_shell && login_shell))
    {
      if (posixly_correct == 0 && act_like_sh == 0 && privileged_mode == 0 &&
	    sourced_env++ == 0)
	execute_env_file (get_string_value ("BASH_ENV"));
      return;
    }

  /* Interactive shell or `-su' shell. */
  if (posixly_correct == 0)		  /* bash, sh */
    {
      if (login_shell && sourced_login++ == 0)
	{
	  /* We don't execute .bashrc for login shells. */
	  no_rc++;

	  /* Execute /etc/profile and one of the personal login shell
	     initialization files. */
	  if (no_profile == 0)
	    {
	      maybe_execute_file (SYS_PROFILE, 1);

	      if (act_like_sh)	/* sh */
		maybe_execute_file ("~/.profile", 1);
	      else if ((maybe_execute_file ("~/.bash_profile", 1) == 0) &&
		       (maybe_execute_file ("~/.bash_login", 1) == 0))	/* bash */
		maybe_execute_file ("~/.profile", 1);
	    }
	}

      /* bash */
      if (act_like_sh == 0 && no_rc == 0)
	{
#ifdef SYS_BASHRC
#  if defined (__OPENNT)
	  maybe_execute_file (_prefixInstallPath(SYS_BASHRC, NULL, 0), 1);
#  else
	  maybe_execute_file (SYS_BASHRC, 1);
#  endif
#endif
	  maybe_execute_file (bashrc_file, 1);
	}
      /* sh */
      else if (act_like_sh && privileged_mode == 0 && sourced_env++ == 0)
	execute_env_file (get_string_value ("ENV"));
    }
  else		/* bash --posix, sh --posix */
    {
      /* bash and sh */
      if (interactive_shell && privileged_mode == 0 && sourced_env++ == 0)
	execute_env_file (get_string_value ("ENV"));
    }

#if defined (JOB_CONTROL)
  set_job_control (old_job_control);
#endif
}

#if defined (RESTRICTED_SHELL)
/* Return 1 if the shell should be a restricted one based on NAME or the
   value of `restricted'.  Don't actually do anything, just return a
   boolean value. */
int
shell_is_restricted (name)
     char *name;
{
  char *temp;

  if (restricted)
    return 1;
  temp = base_pathname (name);
  if (*temp == '-')
    temp++;
  return (STREQ (temp, RESTRICTED_SHELL_NAME));
}

/* Perhaps make this shell a `restricted' one, based on NAME.  If the
   basename of NAME is "rbash", then this shell is restricted.  The
   name of the restricted shell is a configurable option, see config.h.
   In a restricted shell, PATH, SHELL, ENV, and BASH_ENV are read-only
   and non-unsettable.
   Do this also if `restricted' is already set to 1; maybe the shell was
   started with -r. */
int
maybe_make_restricted (name)
     char *name;
{
  char *temp;

  temp = base_pathname (name);
  if (*temp == '-')
    temp++;
  if (restricted || (STREQ (temp, RESTRICTED_SHELL_NAME)))
    {
      set_var_read_only ("PATH");
      set_var_read_only ("SHELL");
      set_var_read_only ("ENV");
      set_var_read_only ("BASH_ENV");
      restricted = 1;
    }
  return (restricted);
}
#endif /* RESTRICTED_SHELL */

/* Fetch the current set of uids and gids and return 1 if we're running
   setuid or setgid. */
static int
uidget ()
{
  uid_t u;

  u = getuid ();
  if (current_user.uid != u)
    {
      FREE (current_user.user_name);
      FREE (current_user.shell);
      FREE (current_user.home_dir);
      current_user.user_name = current_user.shell = current_user.home_dir = (char *)NULL;
    }
  current_user.uid = u;
  current_user.gid = getgid ();
  current_user.euid = geteuid ();
  current_user.egid = getegid ();

  /* See whether or not we are running setuid or setgid. */
  return (current_user.uid != current_user.euid) ||
	   (current_user.gid != current_user.egid);
}

void
disable_priv_mode ()
{
  setuid (current_user.uid);
  setgid (current_user.gid);
  current_user.euid = current_user.uid;
  current_user.egid = current_user.gid;
}

#if defined (WORDEXP_OPTION)
static int
run_wordexp (words)
     char *words;
{
  int code, nw, nb;
  WORD_LIST *wl, *tl, *result;

  code = setjmp (top_level);

  if (code != NOT_JUMPED)
    {
      switch (code)
	{
	  /* Some kind of throw to top_level has occured. */
	case FORCE_EOF:
	  return last_command_exit_value = 127;
	case ERREXIT:
	case EXITPROG:
	  return last_command_exit_value;
	case DISCARD:
	  return last_command_exit_value = 1;
	default:
	  command_error ("run_wordexp", CMDERR_BADJUMP, code, 0);
	}
    }

  /* Run it through the parser to get a list of words and expand them */
  if (words && *words)
    {
      with_input_from_string (words, "--wordexp");
      if (parse_command () != 0)
	return (126);
      if (global_command == 0)
	{
	  printf ("0\n0\n");
	  return (0);
	}
      if (global_command->type != cm_simple)
	return (126);
      wl = global_command->value.Simple->words;
      if (protected_mode)
	for (tl = wl; tl; tl = tl->next)
	  tl->word->flags |= W_NOCOMSUB|W_NOPROCSUB;
      result = wl ? expand_words_no_vars (wl) : (WORD_LIST *)0;
    }
  else
    result = (WORD_LIST *)0;

  last_command_exit_value = 0;

  if (result == 0)
    {
      printf ("0\n0\n");
      return (0);
    }

  /* Count up the number of words and bytes, and print them.  Don't count
     the trailing NUL byte. */
  for (nw = nb = 0, wl = result; wl; wl = wl->next)
    {
      nw++;
      nb += strlen (wl->word->word);
    }
  printf ("%u\n%u\n", nw, nb);
  /* Print each word on a separate line.  This will have to be changed when
     the interface to glibc is completed. */
  for (wl = result; wl; wl = wl->next)
    printf ("%s\n", wl->word->word);

  return (0);
}
#endif

#if defined (ONESHOT)
/* Run one command, given as the argument to the -c option.  Tell
   parse_and_execute not to fork for a simple command. */
static int
run_one_command (command)
     char *command;
{
  int code;

  code = setjmp (top_level);

  if (code != NOT_JUMPED)
    {
#if defined (PROCESS_SUBSTITUTION)
      unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */
      switch (code)
	{
	  /* Some kind of throw to top_level has occured. */
	case FORCE_EOF:
	  return last_command_exit_value = 127;
	case ERREXIT:
	case EXITPROG:
	  return last_command_exit_value;
	case DISCARD:
	  return last_command_exit_value = 1;
	default:
	  command_error ("run_one_command", CMDERR_BADJUMP, code, 0);
	}
    }
   return (parse_and_execute (savestring (command), "-c", SEVAL_NOHIST));
}
#endif /* ONESHOT */

static int
bind_args (argv, arg_start, arg_end, start_index)
     char **argv;
     int arg_start, arg_end, start_index;
{
  register int i;
  WORD_LIST *args;

  for (i = arg_start, args = (WORD_LIST *)NULL; i < arg_end; i++)
    args = make_word_list (make_word (argv[i]), args);
  if (args)
    {
      args = REVERSE_LIST (args, WORD_LIST *);
      if (start_index == 0)	/* bind to $0...$n for sh -c command */
	{
	  /* Posix.2 4.56.3 says that the first argument after sh -c command
	     becomes $0, and the rest of the arguments become $1...$n */
	  shell_name = savestring (args->word->word);
	  FREE (dollar_vars[0]);
	  dollar_vars[0] = savestring (args->word->word);
	  remember_args (args->next, 1);
	  push_args (args->next);	/* BASH_ARGV and BASH_ARGC */
	}
      else			/* bind to $1...$n for shell script */
        {
	  remember_args (args, 1);
	  push_args (args);		/* BASH_ARGV and BASH_ARGC */
        }

      dispose_words (args);
    }

  return (i);
}

void
unbind_args ()
{
  remember_args ((WORD_LIST *)NULL, 1);
  pop_args ();				/* Reset BASH_ARGV and BASH_ARGC */
}

static void
start_debugger ()
{
#if defined (DEBUGGER) && defined (DEBUGGER_START_FILE)
  int old_errexit;

  old_errexit = exit_immediately_on_error;
  exit_immediately_on_error = 0;

  maybe_execute_file (DEBUGGER_START_FILE, 1);
  function_trace_mode = 1;

  exit_immediately_on_error += old_errexit;
#endif
}

static int
open_shell_script (script_name)
     char *script_name;
{
  int fd, e, fd_is_tty;
  char *filename, *path_filename, *t;
  char sample[80];
  int sample_len;
  struct stat sb;
#if defined (ARRAY_VARS)
  SHELL_VAR *funcname_v, *bash_source_v, *bash_lineno_v;
  ARRAY *funcname_a, *bash_source_a, *bash_lineno_a;
#endif

  filename = savestring (script_name);

  fd = open (filename, O_RDONLY);
  if ((fd < 0) && (errno == ENOENT) && (absolute_program (filename) == 0))
    {
      e = errno;
      /* If it's not in the current directory, try looking through PATH
	 for it. */
      path_filename = find_path_file (script_name);
      if (path_filename)
	{
	  free (filename);
	  filename = path_filename;
	  fd = open (filename, O_RDONLY);
	}
      else
	errno = e;
    }

  if (fd < 0)
    {
      e = errno;
      file_error (filename);
      exit ((e == ENOENT) ? EX_NOTFOUND : EX_NOINPUT);
    }

  free (dollar_vars[0]);
  dollar_vars[0] = savestring (script_name);

#if defined (ARRAY_VARS)
  GET_ARRAY_FROM_VAR ("FUNCNAME", funcname_v, funcname_a);
  GET_ARRAY_FROM_VAR ("BASH_SOURCE", bash_source_v, bash_source_a);
  GET_ARRAY_FROM_VAR ("BASH_LINENO", bash_lineno_v, bash_lineno_a);

  array_push (bash_source_a, filename);
  if (bash_lineno_a)
    {
      t = itos (executing_line_number ());
      array_push (bash_lineno_a, t);
      free (t);
    }
  array_push (funcname_a, "main");
#endif

#ifdef HAVE_DEV_FD
  fd_is_tty = isatty (fd);
#else
  fd_is_tty = 0;
#endif

  /* Only do this with non-tty file descriptors we can seek on. */
  if (fd_is_tty == 0 && (lseek (fd, 0L, 1) != -1))
    {
      /* Check to see if the `file' in `bash file' is a binary file
	 according to the same tests done by execute_simple_command (),
	 and report an error and exit if it is. */
      sample_len = read (fd, sample, sizeof (sample));
      if (sample_len < 0)
	{
	  e = errno;
	  if ((fstat (fd, &sb) == 0) && S_ISDIR (sb.st_mode))
	    internal_error (_("%s: is a directory"), filename);
	  else
	    {
	      errno = e;
	      file_error (filename);
	    }
	  exit (EX_NOEXEC);
	}
      else if (sample_len > 0 && (check_binary_file (sample, sample_len)))
	{
	  internal_error (_("%s: cannot execute binary file"), filename);
	  exit (EX_BINARY_FILE);
	}
      /* Now rewind the file back to the beginning. */
      lseek (fd, 0L, 0);
    }

  /* Open the script.  But try to move the file descriptor to a randomly
     large one, in the hopes that any descriptors used by the script will
     not match with ours. */
  fd = move_to_high_fd (fd, 1, -1);

#if defined (__CYGWIN__) && defined (O_TEXT)
  setmode (fd, O_TEXT);
#endif

#if defined (BUFFERED_INPUT)
  default_buffered_input = fd;
  SET_CLOSE_ON_EXEC (default_buffered_input);
#else /* !BUFFERED_INPUT */
  default_input = fdopen (fd, "r");

  if (default_input == 0)
    {
      file_error (filename);
      exit (EX_NOTFOUND);
    }

  SET_CLOSE_ON_EXEC (fd);
  if (fileno (default_input) != fd)
    SET_CLOSE_ON_EXEC (fileno (default_input));
#endif /* !BUFFERED_INPUT */

  /* Just about the only way for this code to be executed is if something
     like `bash -i /dev/stdin' is executed. */
  if (interactive_shell && fd_is_tty)
    {
      dup2 (fd, 0);
      close (fd);
      fd = 0;
#if defined (BUFFERED_INPUT)
      default_buffered_input = 0;
#else
      fclose (default_input);
      default_input = stdin;
#endif
    }
  else if (forced_interactive && fd_is_tty == 0)
    /* But if a script is called with something like `bash -i scriptname',
       we need to do a non-interactive setup here, since we didn't do it
       before. */
    init_interactive_script ();

  free (filename);
  return (fd);
}

/* Initialize the input routines for the parser. */
static void
set_bash_input ()
{
  /* Make sure the fd from which we are reading input is not in
     no-delay mode. */
#if defined (BUFFERED_INPUT)
  if (interactive == 0)
    sh_unset_nodelay_mode (default_buffered_input);
  else
#endif /* !BUFFERED_INPUT */
    sh_unset_nodelay_mode (fileno (stdin));

  /* with_input_from_stdin really means `with_input_from_readline' */
  if (interactive && no_line_editing == 0)
    with_input_from_stdin ();
#if defined (BUFFERED_INPUT)
  else if (interactive == 0)
    with_input_from_buffered_stream (default_buffered_input, dollar_vars[0]);
#endif /* BUFFERED_INPUT */
  else
    with_input_from_stream (default_input, dollar_vars[0]);
}

/* Close the current shell script input source and forget about it.  This is
   extern so execute_cmd.c:initialize_subshell() can call it.  If CHECK_ZERO
   is non-zero, we close default_buffered_input even if it's the standard
   input (fd 0). */
void
unset_bash_input (check_zero)
     int check_zero;
{
#if defined (BUFFERED_INPUT)
  if ((check_zero && default_buffered_input >= 0) ||
      (check_zero == 0 && default_buffered_input > 0))
    {
      close_buffered_fd (default_buffered_input);
      default_buffered_input = bash_input.location.buffered_fd = -1;
      bash_input.type = st_none;		/* XXX */
    }
#else /* !BUFFERED_INPUT */
  if (default_input)
    {
      fclose (default_input);
      default_input = (FILE *)NULL;
    }
#endif /* !BUFFERED_INPUT */
}
      

#if !defined (PROGRAM)
#  define PROGRAM "bash"
#endif

static void
set_shell_name (argv0)
     char *argv0;
{
  /* Here's a hack.  If the name of this shell is "sh", then don't do
     any startup files; just try to be more like /bin/sh. */
  shell_name = argv0 ? base_pathname (argv0) : PROGRAM;

  if (argv0 && *argv0 == '-')
    {
      if (*shell_name == '-')
	shell_name++;
      login_shell++;
    }

  if (shell_name[0] == 's' && shell_name[1] == 'h' && shell_name[2] == '\0')
    act_like_sh++;
  if (shell_name[0] == 's' && shell_name[1] == 'u' && shell_name[2] == '\0')
    su_shell++;

  shell_name = argv0 ? argv0 : PROGRAM;
  FREE (dollar_vars[0]);
  dollar_vars[0] = savestring (shell_name);

  /* A program may start an interactive shell with
	  "execl ("/bin/bash", "-", NULL)".
     If so, default the name of this shell to our name. */
  if (!shell_name || !*shell_name || (shell_name[0] == '-' && !shell_name[1]))
    shell_name = PROGRAM;
}

static void
init_interactive ()
{
  expand_aliases = interactive_shell = startup_state = 1;
  interactive = 1;
}

static void
init_noninteractive ()
{
#if defined (HISTORY)
  bash_history_reinit (0);
#endif /* HISTORY */
  interactive_shell = startup_state = interactive = 0;
  expand_aliases = posixly_correct;	/* XXX - was 0 not posixly_correct */
  no_line_editing = 1;
#if defined (JOB_CONTROL)
  set_job_control (0);
#endif /* JOB_CONTROL */
}

static void
init_interactive_script ()
{
  init_noninteractive ();
  expand_aliases = interactive_shell = startup_state = 1;
}

void
get_current_user_info ()
{
  struct passwd *entry;

  /* Don't fetch this more than once. */
  if (current_user.user_name == 0)
    {
      entry = getpwuid (current_user.uid);
      if (entry)
	{
	  current_user.user_name = savestring (entry->pw_name);
	  current_user.shell = (entry->pw_shell && entry->pw_shell[0])
				? savestring (entry->pw_shell)
				: savestring ("/bin/sh");
	  current_user.home_dir = savestring (entry->pw_dir);
	}
      else
	{
	  current_user.user_name = _("I have no name!");
	  current_user.user_name = savestring (current_user.user_name);
	  current_user.shell = savestring ("/bin/sh");
	  current_user.home_dir = savestring ("/");
	}
      endpwent ();
    }
}

/* Do whatever is necessary to initialize the shell.
   Put new initializations in here. */
static void
shell_initialize ()
{
  char hostname[256];

  /* Line buffer output for stderr and stdout. */
  if (shell_initialized == 0)
    {
      sh_setlinebuf (stderr);
      sh_setlinebuf (stdout);
    }

  /* Sort the array of shell builtins so that the binary search in
     find_shell_builtin () works correctly. */
  initialize_shell_builtins ();

  /* Initialize the trap signal handlers before installing our own
     signal handlers.  traps.c:restore_original_signals () is responsible
     for restoring the original default signal handlers.  That function
     is called when we make a new child. */
  initialize_traps ();
  initialize_signals (0);

  /* It's highly unlikely that this will change. */
  if (current_host_name == 0)
    {
      /* Initialize current_host_name. */
      if (gethostname (hostname, 255) < 0)
	current_host_name = "??host??";
      else
	current_host_name = savestring (hostname);
    }

  /* Initialize the stuff in current_user that comes from the password
     file.  We don't need to do this right away if the shell is not
     interactive. */
  if (interactive_shell)
    get_current_user_info ();

  /* Initialize our interface to the tilde expander. */
  tilde_initialize ();

  /* Initialize internal and environment variables.  Don't import shell
     functions from the environment if we are running in privileged or
     restricted mode or if the shell is running setuid. */
#if defined (RESTRICTED_SHELL)
  initialize_shell_variables (shell_environment, privileged_mode||restricted||running_setuid);
#else
  initialize_shell_variables (shell_environment, privileged_mode||running_setuid);
#endif

  /* Initialize the data structures for storing and running jobs. */
  initialize_job_control (0);

  /* Initialize input streams to null. */
  initialize_bash_input ();

  initialize_flags ();

  /* Initialize the shell options.  Don't import the shell options
     from the environment variable $SHELLOPTS if we are running in
     privileged or restricted mode or if the shell is running setuid. */
#if defined (RESTRICTED_SHELL)
  initialize_shell_options (privileged_mode||restricted||running_setuid);
  initialize_bashopts (privileged_mode||restricted||running_setuid);
#else
  initialize_shell_options (privileged_mode||running_setuid);
  initialize_bashopts (privileged_mode||running_setuid);
#endif
}

/* Function called by main () when it appears that the shell has already
   had some initialization performed.  This is supposed to reset the world
   back to a pristine state, as if we had been exec'ed. */
static void
shell_reinitialize ()
{
  /* The default shell prompts. */
  primary_prompt = PPROMPT;
  secondary_prompt = SPROMPT;

  /* Things that get 1. */
  current_command_number = 1;

  /* We have decided that the ~/.bashrc file should not be executed
     for the invocation of each shell script.  If the variable $ENV
     (or $BASH_ENV) is set, its value is used as the name of a file
     to source. */
  no_rc = no_profile = 1;

  /* Things that get 0. */
  login_shell = make_login_shell = interactive = executing = 0;
  debugging = do_version = line_number = last_command_exit_value = 0;
  forced_interactive = interactive_shell = subshell_environment = 0;
  expand_aliases = 0;

#if defined (HISTORY)
  bash_history_reinit (0);
#endif /* HISTORY */

#if defined (RESTRICTED_SHELL)
  restricted = 0;
#endif /* RESTRICTED_SHELL */

  /* Ensure that the default startup file is used.  (Except that we don't
     execute this file for reinitialized shells). */
  bashrc_file = "~/.bashrc";

  /* Delete all variables and functions.  They will be reinitialized when
     the environment is parsed. */
  delete_all_contexts (shell_variables);
  delete_all_variables (shell_functions);

  reinit_special_variables ();

#if defined (READLINE)
  bashline_reinitialize ();
#endif

  shell_reinitialized = 1;
}

static void
show_shell_usage (fp, extra)
     FILE *fp;
     int extra;
{
  int i;
  char *set_opts, *s, *t;

  if (extra)
    fprintf (fp, _("GNU bash, version %s-(%s)\n"), shell_version_string (), MACHTYPE);
  fprintf (fp, _("Usage:\t%s [GNU long option] [option] ...\n\t%s [GNU long option] [option] script-file ...\n"),
	     shell_name, shell_name);
  fputs (_("GNU long options:\n"), fp);
  for (i = 0; long_args[i].name; i++)
    fprintf (fp, "\t--%s\n", long_args[i].name);

  fputs (_("Shell options:\n"), fp);
  fputs (_("\t-irsD or -c command or -O shopt_option\t\t(invocation only)\n"), fp);

  for (i = 0, set_opts = 0; shell_builtins[i].name; i++)
    if (STREQ (shell_builtins[i].name, "set"))
      set_opts = savestring (shell_builtins[i].short_doc);
  if (set_opts)
    {
      s = strchr (set_opts, '[');
      if (s == 0)
	s = set_opts;
      while (*++s == '-')
	;
      t = strchr (s, ']');
      if (t)
	*t = '\0';
      fprintf (fp, _("\t-%s or -o option\n"), s);
      free (set_opts);
    }

  if (extra)
    {
      fprintf (fp, _("Type `%s -c \"help set\"' for more information about shell options.\n"), shell_name);
      fprintf (fp, _("Type `%s -c help' for more information about shell builtin commands.\n"), shell_name);
      fprintf (fp, _("Use the `bashbug' command to report bugs.\n"));
    }
}

static void
add_shopt_to_alist (opt, on_or_off)
     char *opt;
     int on_or_off;
{
  if (shopt_ind >= shopt_len)
    {
      shopt_len += 8;
      shopt_alist = (STRING_INT_ALIST *)xrealloc (shopt_alist, shopt_len * sizeof (shopt_alist[0]));
    }
  shopt_alist[shopt_ind].word = opt;
  shopt_alist[shopt_ind].token = on_or_off;
  shopt_ind++;
}

static void
run_shopt_alist ()
{
  register int i;

  for (i = 0; i < shopt_ind; i++)
    if (shopt_setopt (shopt_alist[i].word, (shopt_alist[i].token == '-')) != EXECUTION_SUCCESS)
      exit (EX_BADUSAGE);
  free (shopt_alist);
  shopt_alist = 0;
  shopt_ind = shopt_len = 0;
}
