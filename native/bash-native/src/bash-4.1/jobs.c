/* jobs.c - functions that make children, remember them, and handle their termination. */

/* This file works with both POSIX and BSD systems.  It implements job
   control. */

/* Copyright (C) 1989-2009 Free Software Foundation, Inc.

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
#include "trap.h"
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "posixtime.h"

#if defined (HAVE_SYS_RESOURCE_H) && defined (HAVE_WAIT3) && !defined (_POSIX_VERSION) && !defined (RLIMTYPE)
#  include <sys/resource.h>
#endif /* !_POSIX_VERSION && HAVE_SYS_RESOURCE_H && HAVE_WAIT3 && !RLIMTYPE */

#if defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif

#include "filecntl.h"
#include <sys/ioctl.h>
#include <sys/param.h>

#if defined (BUFFERED_INPUT)
#  include "input.h"
#endif

/* Need to include this up here for *_TTY_DRIVER definitions. */
#include "shtty.h"

/* Define this if your output is getting swallowed.  It's a no-op on
   machines with the termio or termios tty drivers. */
/* #define DRAIN_OUTPUT */

/* For the TIOCGPGRP and TIOCSPGRP ioctl parameters on HP-UX */
#if defined (hpux) && !defined (TERMIOS_TTY_DRIVER)
#  include <bsdtty.h>
#endif /* hpux && !TERMIOS_TTY_DRIVER */

#include "bashansi.h"
#include "bashintl.h"
#include "shell.h"
#include "jobs.h"
#include "execute_cmd.h"
#include "flags.h"

#include "builtins/builtext.h"
#include "builtins/common.h"

#if !defined (errno)
extern int errno;
#endif /* !errno */

#define DEFAULT_CHILD_MAX 32
#if !defined (DEBUG)
#define MAX_JOBS_IN_ARRAY 4096		/* production */
#else
#define MAX_JOBS_IN_ARRAY 128		/* testing */
#endif

/* Flag values for second argument to delete_job */
#define DEL_WARNSTOPPED		1	/* warn about deleting stopped jobs */
#define DEL_NOBGPID		2	/* don't add pgrp leader to bgpids */

/* Take care of system dependencies that must be handled when waiting for
   children.  The arguments to the WAITPID macro match those to the Posix.1
   waitpid() function. */

#if defined (ultrix) && defined (mips) && defined (_POSIX_VERSION)
#  define WAITPID(pid, statusp, options) \
	wait3 ((union wait *)statusp, options, (struct rusage *)0)
#else
#  if defined (_POSIX_VERSION) || defined (HAVE_WAITPID)
#    define WAITPID(pid, statusp, options) \
	waitpid ((pid_t)pid, statusp, options)
#  else
#    if defined (HAVE_WAIT3)
#      define WAITPID(pid, statusp, options) \
	wait3 (statusp, options, (struct rusage *)0)
#    else
#      define WAITPID(pid, statusp, options) \
	wait3 (statusp, options, (int *)0)
#    endif /* HAVE_WAIT3 */
#  endif /* !_POSIX_VERSION && !HAVE_WAITPID*/
#endif /* !(Ultrix && mips && _POSIX_VERSION) */

/* getpgrp () varies between systems.  Even systems that claim to be
   Posix.1 compatible lie sometimes (Ultrix, SunOS4, apollo). */
#if defined (GETPGRP_VOID)
#  define getpgid(p) getpgrp ()
#else
#  define getpgid(p) getpgrp (p)
#endif /* !GETPGRP_VOID */

/* If the system needs it, REINSTALL_SIGCHLD_HANDLER will reinstall the
   handler for SIGCHLD. */
#if defined (MUST_REINSTALL_SIGHANDLERS)
#  define REINSTALL_SIGCHLD_HANDLER signal (SIGCHLD, sigchld_handler)
#else
#  define REINSTALL_SIGCHLD_HANDLER
#endif /* !MUST_REINSTALL_SIGHANDLERS */

/* Some systems let waitpid(2) tell callers about stopped children. */
#if !defined (WCONTINUED) || defined (WCONTINUED_BROKEN)
#  undef WCONTINUED
#  define WCONTINUED 0
#endif
#if !defined (WIFCONTINUED)
#  define WIFCONTINUED(s)	(0)
#endif

/* The number of additional slots to allocate when we run out. */
#define JOB_SLOTS 8

typedef int sh_job_map_func_t __P((JOB *, int, int, int));

/* Variables used here but defined in other files. */
extern int subshell_environment, line_number;
extern int posixly_correct, shell_level;
extern int last_command_exit_value, last_command_exit_signal;
extern int loop_level, breaking;
extern int executing_list;
extern int sourcelevel;
extern int running_trap;
extern sh_builtin_func_t *this_shell_builtin;
extern char *shell_name, *this_command_name;
extern sigset_t top_level_mask;
extern procenv_t wait_intr_buf;
extern int wait_signal_received;
extern WORD_LIST *subst_assign_varlist;

static struct jobstats zerojs = { -1L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NO_JOB, NO_JOB, 0, 0 };
struct jobstats js = { -1L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NO_JOB, NO_JOB, 0, 0 };

struct bgpids bgpids = { 0, 0, 0 };

/* The array of known jobs. */
JOB **jobs = (JOB **)NULL;

#if 0
/* The number of slots currently allocated to JOBS. */
int job_slots = 0;
#endif

/* The controlling tty for this shell. */
int shell_tty = -1;

/* The shell's process group. */
pid_t shell_pgrp = NO_PID;

/* The terminal's process group. */
pid_t terminal_pgrp = NO_PID;

/* The process group of the shell's parent. */
pid_t original_pgrp = NO_PID;

/* The process group of the pipeline currently being made. */
pid_t pipeline_pgrp = (pid_t)0;

#if defined (PGRP_PIPE)
/* Pipes which each shell uses to communicate with the process group leader
   until all of the processes in a pipeline have been started.  Then the
   process leader is allowed to continue. */
int pgrp_pipe[2] = { -1, -1 };
#endif

#if 0
/* The job which is current; i.e. the one that `%+' stands for. */
int current_job = NO_JOB;

/* The previous job; i.e. the one that `%-' stands for. */
int previous_job = NO_JOB;
#endif

/* Last child made by the shell.  */
pid_t last_made_pid = NO_PID;

/* Pid of the last asynchronous child. */
pid_t last_asynchronous_pid = NO_PID;

/* The pipeline currently being built. */
PROCESS *the_pipeline = (PROCESS *)NULL;

/* If this is non-zero, do job control. */
int job_control = 1;

/* Call this when you start making children. */
int already_making_children = 0;

/* If this is non-zero, $LINES and $COLUMNS are reset after every process
   exits from get_tty_state(). */
int check_window_size;

/* Functions local to this file. */

static sighandler wait_sigint_handler __P((int));
static sighandler sigchld_handler __P((int));
static sighandler sigcont_sighandler __P((int));
static sighandler sigstop_sighandler __P((int));

static int waitchld __P((pid_t, int));

static PROCESS *find_pipeline __P((pid_t, int, int *));
static PROCESS *find_process __P((pid_t, int, int *));

static char *current_working_directory __P((void));
static char *job_working_directory __P((void));
static char *j_strsignal __P((int));
static char *printable_job_status __P((int, PROCESS *, int));

static PROCESS *find_last_proc __P((int, int));
static pid_t find_last_pid __P((int, int));

static int set_new_line_discipline __P((int));
static int map_over_jobs __P((sh_job_map_func_t *, int, int));
static int job_last_stopped __P((int));
static int job_last_running __P((int));
static int most_recent_job_in_state __P((int, JOB_STATE));
static int find_job __P((pid_t, int, PROCESS **));
static int print_job __P((JOB *, int, int, int));
static int process_exit_status __P((WAIT));
static int process_exit_signal __P((WAIT));
static int job_exit_status __P((int));
static int job_exit_signal __P((int));
static int set_job_status_and_cleanup __P((int));

static WAIT job_signal_status __P((int));
static WAIT raw_job_exit_status __P((int));

static void notify_of_job_status __P((void));
static void reset_job_indices __P((void));
static void cleanup_dead_jobs __P((void));
static int processes_in_job __P((int));
static void realloc_jobs_list __P((void));
static int compact_jobs_list __P((int));
static int discard_pipeline __P((PROCESS *));
static void add_process __P((char *, pid_t));
static void print_pipeline __P((PROCESS *, int, int, FILE *));
static void pretty_print_job __P((int, int, FILE *));
static void set_current_job __P((int));
static void reset_current __P((void));
static void set_job_running __P((int));
static void setjstatus __P((int));
static int maybe_give_terminal_to __P((pid_t, pid_t, int));
static void mark_all_jobs_as_dead __P((void));
static void mark_dead_jobs_as_notified __P((int));
static void restore_sigint_handler __P((void));
#if defined (PGRP_PIPE)
static void pipe_read __P((int *));
#endif

static struct pidstat *bgp_alloc __P((pid_t, int));
static struct pidstat *bgp_add __P((pid_t, int));
static int bgp_delete __P((pid_t));
static void bgp_clear __P((void));
static int bgp_search __P((pid_t));
static void bgp_prune __P((void));

#if defined (ARRAY_VARS)
static int *pstatuses;		/* list of pipeline statuses */
static int statsize;
#endif

/* Used to synchronize between wait_for and other functions and the SIGCHLD
   signal handler. */
static int sigchld;
static int queue_sigchld;

#define QUEUE_SIGCHLD(os)	(os) = sigchld, queue_sigchld++

#define UNQUEUE_SIGCHLD(os) \
	do { \
	  queue_sigchld--; \
	  if (queue_sigchld == 0 && os != sigchld) \
	    waitchld (-1, 0); \
	} while (0)

static SigHandler *old_tstp, *old_ttou, *old_ttin;
static SigHandler *old_cont = (SigHandler *)SIG_DFL;

/* A place to temporarily save the current pipeline. */
static PROCESS *saved_pipeline;
static int saved_already_making_children;

/* Set this to non-zero whenever you don't want the jobs list to change at
   all: no jobs deleted and no status change notifications.  This is used,
   for example, when executing SIGCHLD traps, which may run arbitrary
   commands. */
static int jobs_list_frozen;

static char retcode_name_buffer[64];

/* flags to detect pid wraparound */
static pid_t first_pid = NO_PID;
static int pid_wrap = -1;

#if !defined (_POSIX_VERSION)

/* These are definitions to map POSIX 1003.1 functions onto existing BSD
   library functions and system calls. */
#define setpgid(pid, pgrp)	setpgrp (pid, pgrp)
#define tcsetpgrp(fd, pgrp)	ioctl ((fd), TIOCSPGRP, &(pgrp))

pid_t
tcgetpgrp (fd)
     int fd;
{
  pid_t pgrp;

  /* ioctl will handle setting errno correctly. */
  if (ioctl (fd, TIOCGPGRP, &pgrp) < 0)
    return (-1);
  return (pgrp);
}

#endif /* !_POSIX_VERSION */

/* Initialize the global job stats structure and other bookkeeping variables */
void
init_job_stats ()
{
  js = zerojs;
  first_pid = NO_PID;
  pid_wrap = -1;
}

/* Return the working directory for the current process.  Unlike
   job_working_directory, this does not call malloc (), nor do any
   of the functions it calls.  This is so that it can safely be called
   from a signal handler. */
static char *
current_working_directory ()
{
  char *dir;
  static char d[PATH_MAX];

  dir = get_string_value ("PWD");

  if (dir == 0 && the_current_working_directory && no_symbolic_links)
    dir = the_current_working_directory;

  if (dir == 0)
    {
      dir = getcwd (d, sizeof(d));
      if (dir)
	dir = d;
    }

  return (dir == 0) ? "<unknown>" : dir;
}

/* Return the working directory for the current process. */
static char *
job_working_directory ()
{
  char *dir;

  dir = get_string_value ("PWD");
  if (dir)
    return (savestring (dir));

  dir = get_working_directory ("job-working-directory");
  if (dir)
    return (dir);

  return (savestring ("<unknown>"));
}

void
making_children ()
{
  if (already_making_children)
    return;

  already_making_children = 1;
  start_pipeline ();
}

void
stop_making_children ()
{
  already_making_children = 0;
}

void
cleanup_the_pipeline ()
{
  PROCESS *disposer;
  sigset_t set, oset;

  BLOCK_CHILD (set, oset);
  disposer = the_pipeline;
  the_pipeline = (PROCESS *)NULL;
  UNBLOCK_CHILD (oset);

  if (disposer)
    discard_pipeline (disposer);
}

void
save_pipeline (clear)
     int clear;
{
  saved_pipeline = the_pipeline;
  if (clear)
    the_pipeline = (PROCESS *)NULL;
  saved_already_making_children = already_making_children;
}

void
restore_pipeline (discard)
     int discard;
{
  PROCESS *old_pipeline;

  old_pipeline = the_pipeline;
  the_pipeline = saved_pipeline;
  already_making_children = saved_already_making_children;
  if (discard && old_pipeline)
    discard_pipeline (old_pipeline);
}

/* Start building a pipeline.  */
void
start_pipeline ()
{
  if (the_pipeline)
    {
      cleanup_the_pipeline ();
      pipeline_pgrp = 0;
#if defined (PGRP_PIPE)
      sh_closepipe (pgrp_pipe);
#endif
    }

#if defined (PGRP_PIPE)
  if (job_control)
    {
      if (pipe (pgrp_pipe) == -1)
	sys_error (_("start_pipeline: pgrp pipe"));
    }
#endif
}

/* Stop building a pipeline.  Install the process list in the job array.
   This returns the index of the newly installed job.
   DEFERRED is a command structure to be executed upon satisfactory
   execution exit of this pipeline. */
int
stop_pipeline (async, deferred)
     int async;
     COMMAND *deferred;
{
  register int i, j;
  JOB *newjob;
  sigset_t set, oset;

  BLOCK_CHILD (set, oset);

#if defined (PGRP_PIPE)
  /* The parent closes the process group synchronization pipe. */
  sh_closepipe (pgrp_pipe);
#endif

  cleanup_dead_jobs ();

  if (js.j_jobslots == 0)
    {
      js.j_jobslots = JOB_SLOTS;
      jobs = (JOB **)xmalloc (js.j_jobslots * sizeof (JOB *));

      /* Now blank out these new entries. */
      for (i = 0; i < js.j_jobslots; i++)
	jobs[i] = (JOB *)NULL;

      js.j_firstj = js.j_lastj = js.j_njobs = 0;
    }

  /* Scan from the last slot backward, looking for the next free one. */
  /* XXX - revisit this interactive assumption */
  /* XXX - this way for now */
  if (interactive)
    {
      for (i = js.j_jobslots; i; i--)
	if (jobs[i - 1])
	  break;
    }
  else
    {
#if 0
      /* This wraps around, but makes it inconvenient to extend the array */
      for (i = js.j_lastj+1; i != js.j_lastj; i++)
	{
	  if (i >= js.j_jobslots)
	    i = 0;
	  if (jobs[i] == 0)
	    break;
	}	
      if (i == js.j_lastj)
        i = js.j_jobslots;
#else
      /* This doesn't wrap around yet. */
      for (i = js.j_lastj ? js.j_lastj + 1 : js.j_lastj; i < js.j_jobslots; i++)
	if (jobs[i] == 0)
	  break;
#endif
    }

  /* Do we need more room? */

  /* First try compaction */
  if ((interactive_shell == 0 || subshell_environment) && i == js.j_jobslots && js.j_jobslots >= MAX_JOBS_IN_ARRAY)
    i = compact_jobs_list (0);

  /* If we can't compact, reallocate */
  if (i == js.j_jobslots)
    {
      js.j_jobslots += JOB_SLOTS;
      jobs = (JOB **)xrealloc (jobs, (js.j_jobslots * sizeof (JOB *)));

      for (j = i; j < js.j_jobslots; j++)
	jobs[j] = (JOB *)NULL;
    }

  /* Add the current pipeline to the job list. */
  if (the_pipeline)
    {
      register PROCESS *p;
      int any_running, any_stopped, n;

      newjob = (JOB *)xmalloc (sizeof (JOB));

      for (n = 1, p = the_pipeline; p->next != the_pipeline; n++, p = p->next)
	;
      p->next = (PROCESS *)NULL;
      newjob->pipe = REVERSE_LIST (the_pipeline, PROCESS *);
      for (p = newjob->pipe; p->next; p = p->next)
	;
      p->next = newjob->pipe;

      the_pipeline = (PROCESS *)NULL;
      newjob->pgrp = pipeline_pgrp;
      pipeline_pgrp = 0;

      newjob->flags = 0;

      /* Flag to see if in another pgrp. */
      if (job_control)
	newjob->flags |= J_JOBCONTROL;

      /* Set the state of this pipeline. */
      p = newjob->pipe;
      any_running = any_stopped = 0;
      do
	{
	  any_running |= PRUNNING (p);
	  any_stopped |= PSTOPPED (p);
	  p = p->next;
	}
      while (p != newjob->pipe);

      newjob->state = any_running ? JRUNNING : (any_stopped ? JSTOPPED : JDEAD);
      newjob->wd = job_working_directory ();
      newjob->deferred = deferred;

      newjob->j_cleanup = (sh_vptrfunc_t *)NULL;
      newjob->cleanarg = (PTR_T) NULL;

      jobs[i] = newjob;
      if (newjob->state == JDEAD && (newjob->flags & J_FOREGROUND))
	setjstatus (i);
      if (newjob->state == JDEAD)
	{
	  js.c_reaped += n;	/* wouldn't have been done since this was not part of a job */
	  js.j_ndead++;
	}
      js.c_injobs += n;

      js.j_lastj = i;
      js.j_njobs++;
    }
  else
    newjob = (JOB *)NULL;

  if (newjob)
    js.j_lastmade = newjob;

  if (async)
    {
      if (newjob)
	{
	  newjob->flags &= ~J_FOREGROUND;
	  newjob->flags |= J_ASYNC;
	  js.j_lastasync = newjob;
	}
      reset_current ();
    }
  else
    {
      if (newjob)
	{
	  newjob->flags |= J_FOREGROUND;
	  /*
	   *		!!!!! NOTE !!!!!  (chet@ins.cwru.edu)
	   *
	   * The currently-accepted job control wisdom says to set the
	   * terminal's process group n+1 times in an n-step pipeline:
	   * once in the parent and once in each child.  This is where
	   * the parent gives it away.
	   *
	   * Don't give the terminal away if this shell is an asynchronous
	   * subshell.
	   *
	   */
	  if (job_control && newjob->pgrp && (subshell_environment&SUBSHELL_ASYNC) == 0)
	    maybe_give_terminal_to (shell_pgrp, newjob->pgrp, 0);
	}
    }

  stop_making_children ();
  UNBLOCK_CHILD (oset);
  return (js.j_current);
}

/* Functions to manage the list of exited background pids whose status has
   been saved. */

static struct pidstat *
bgp_alloc (pid, status)
     pid_t pid;
     int status;
{
  struct pidstat *ps;

  ps = (struct pidstat *)xmalloc (sizeof (struct pidstat));
  ps->pid = pid;
  ps->status = status;
  ps->next = (struct pidstat *)0;
  return ps;
}

static struct pidstat *
bgp_add (pid, status)
     pid_t pid;
     int status;
{
  struct pidstat *ps;

  ps = bgp_alloc (pid, status);

  if (bgpids.list == 0)
    {
      bgpids.list = bgpids.end = ps;
      bgpids.npid = 0;			/* just to make sure */
    }
  else
    {
      bgpids.end->next = ps;
      bgpids.end = ps;
    }
  bgpids.npid++;

  if (bgpids.npid > js.c_childmax)
    bgp_prune ();

  return ps;
}

static int
bgp_delete (pid)
     pid_t pid;
{
  struct pidstat *prev, *p;

  for (prev = p = bgpids.list; p; prev = p, p = p->next)
    if (p->pid == pid)
      {
	prev->next = p->next;	/* remove from list */
	break;
      }

  if (p == 0)
    return 0;		/* not found */

#if defined (DEBUG)
  itrace("bgp_delete: deleting %d", pid);
#endif

  /* Housekeeping in the border cases. */
  if (p == bgpids.list)
    bgpids.list = bgpids.list->next;
  else if (p == bgpids.end)
    bgpids.end = prev;

  bgpids.npid--;
  if (bgpids.npid == 0)
    bgpids.list = bgpids.end = 0;
  else if (bgpids.npid == 1)
    bgpids.end = bgpids.list;		/* just to make sure */

  free (p);
  return 1;
}

/* Clear out the list of saved statuses */
static void
bgp_clear ()
{
  struct pidstat *ps, *p;

  for (ps = bgpids.list; ps; )
    {
      p = ps;
      ps = ps->next;
      free (p);
    }
  bgpids.list = bgpids.end = 0;
  bgpids.npid = 0;
}

/* Search for PID in the list of saved background pids; return its status if
   found.  If not found, return -1. */
static int
bgp_search (pid)
     pid_t pid;
{
  struct pidstat *ps;

  for (ps = bgpids.list ; ps; ps = ps->next)
    if (ps->pid == pid)
      return ps->status;
  return -1;
}

static void
bgp_prune ()
{
  struct pidstat *ps;

  while (bgpids.npid > js.c_childmax)
    {
      ps = bgpids.list;
      bgpids.list = bgpids.list->next;
      free (ps);
      bgpids.npid--;
    }
}

/* Reset the values of js.j_lastj and js.j_firstj after one or both have
   been deleted.  The caller should check whether js.j_njobs is 0 before
   calling this.  This wraps around, but the rest of the code does not.  At
   this point, it should not matter. */
static void
reset_job_indices ()
{
  int old;

  if (jobs[js.j_firstj] == 0)
    {
      old = js.j_firstj++;
      if (old >= js.j_jobslots)
	old = js.j_jobslots - 1;
      while (js.j_firstj != old)
	{
	  if (js.j_firstj >= js.j_jobslots)
	    js.j_firstj = 0;
	  if (jobs[js.j_firstj] || js.j_firstj == old)	/* needed if old == 0 */
	    break;
	  js.j_firstj++;
	}
      if (js.j_firstj == old)
        js.j_firstj = js.j_lastj = js.j_njobs = 0;
    }
  if (jobs[js.j_lastj] == 0)
    {
      old = js.j_lastj--;
      if (old < 0)
	old = 0;
      while (js.j_lastj != old)
	{
	  if (js.j_lastj < 0)
	    js.j_lastj = js.j_jobslots - 1;
	  if (jobs[js.j_lastj] || js.j_lastj == old)	/* needed if old == js.j_jobslots */
	    break;
	  js.j_lastj--;
	}
      if (js.j_lastj == old)
        js.j_firstj = js.j_lastj = js.j_njobs = 0;
    }
}
      
/* Delete all DEAD jobs that the user had received notification about. */
static void
cleanup_dead_jobs ()
{
  register int i;
  int os;

  if (js.j_jobslots == 0 || jobs_list_frozen)
    return;

  QUEUE_SIGCHLD(os);

  /* XXX could use js.j_firstj and js.j_lastj here */
  for (i = 0; i < js.j_jobslots; i++)
    {
#if defined (DEBUG)
      if (i < js.j_firstj && jobs[i])
	itrace("cleanup_dead_jobs: job %d non-null before js.j_firstj (%d)", i, js.j_firstj);
      if (i > js.j_lastj && jobs[i])
	itrace("cleanup_dead_jobs: job %d non-null after js.j_lastj (%d)", i, js.j_lastj);
#endif

      if (jobs[i] && DEADJOB (i) && IS_NOTIFIED (i))
	delete_job (i, 0);
    }

#if defined (COPROCESS_SUPPORT)
  coproc_reap ();
#endif

  UNQUEUE_SIGCHLD(os);
}

static int
processes_in_job (job)
     int job;
{
  int nproc;
  register PROCESS *p;

  nproc = 0;
  p = jobs[job]->pipe;
  do
    {
      p = p->next;
      nproc++;
    }
  while (p != jobs[job]->pipe);

  return nproc;
}

static void
delete_old_job (pid)
     pid_t pid;
{
  PROCESS *p;
  int job;

  job = find_job (pid, 0, &p);
  if (job != NO_JOB)
    {
#ifdef DEBUG
      itrace ("delete_old_job: found pid %d in job %d with state %d", pid, job, jobs[job]->state);
#endif
      if (JOBSTATE (job) == JDEAD)
	delete_job (job, DEL_NOBGPID);
      else
	{
	  internal_warning (_("forked pid %d appears in running job %d"), pid, job);
	  if (p)
	    p->pid = 0;
	}
    }
}

/* Reallocate and compress the jobs list.  This returns with a jobs array
   whose size is a multiple of JOB_SLOTS and can hold the current number of
   jobs.  Heuristics are used to minimize the number of new reallocs. */
static void
realloc_jobs_list ()
{
  sigset_t set, oset;
  int nsize, i, j, ncur, nprev;
  JOB **nlist;

  ncur = nprev = NO_JOB;
  nsize = ((js.j_njobs + JOB_SLOTS - 1) / JOB_SLOTS);
  nsize *= JOB_SLOTS;
  i = js.j_njobs % JOB_SLOTS;
  if (i == 0 || i > (JOB_SLOTS >> 1))
    nsize += JOB_SLOTS;

  BLOCK_CHILD (set, oset);
  nlist = (js.j_jobslots == nsize) ? jobs : (JOB **) xmalloc (nsize * sizeof (JOB *));

  js.c_reaped = js.j_ndead = 0;
  for (i = j = 0; i < js.j_jobslots; i++)
    if (jobs[i])
      {
	if (i == js.j_current)
	  ncur = j;
	if (i == js.j_previous)
	  nprev = j;
	nlist[j++] = jobs[i];
	if (jobs[i]->state == JDEAD)
	  {
	    js.j_ndead++;
	    js.c_reaped += processes_in_job (i);
	  }
      }

#if defined (DEBUG)
  itrace ("realloc_jobs_list: resize jobs list from %d to %d", js.j_jobslots, nsize);
  itrace ("realloc_jobs_list: j_lastj changed from %d to %d", js.j_lastj, (j > 0) ? j - 1 : 0);
  itrace ("realloc_jobs_list: j_njobs changed from %d to %d", js.j_njobs, j);
  itrace ("realloc_jobs_list: js.j_ndead %d js.c_reaped %d", js.j_ndead, js.c_reaped);
#endif

  js.j_firstj = 0;
  js.j_lastj = (j > 0) ? j - 1 : 0;
  js.j_njobs = j;
  js.j_jobslots = nsize;

  /* Zero out remaining slots in new jobs list */
  for ( ; j < nsize; j++)
    nlist[j] = (JOB *)NULL;

  if (jobs != nlist)
    {
      free (jobs);
      jobs = nlist;
    }

  if (ncur != NO_JOB)
    js.j_current = ncur;
  if (nprev != NO_JOB)
    js.j_previous = nprev;

  /* Need to reset these */
  if (js.j_current == NO_JOB || js.j_previous == NO_JOB || js.j_current > js.j_lastj || js.j_previous > js.j_lastj)
    reset_current ();

#ifdef DEBUG
  itrace ("realloc_jobs_list: reset js.j_current (%d) and js.j_previous (%d)", js.j_current, js.j_previous);
#endif

  UNBLOCK_CHILD (oset);
}

/* Compact the jobs list by removing dead jobs.  Assumed that we have filled
   the jobs array to some predefined maximum.  Called when the shell is not
   the foreground process (subshell_environment != 0).  Returns the first
   available slot in the compacted list.  If that value is js.j_jobslots, then
   the list needs to be reallocated.  The jobs array may be in new memory if
   this returns > 0 and < js.j_jobslots.  FLAGS is reserved for future use. */
static int
compact_jobs_list (flags)
     int flags;
{
  if (js.j_jobslots == 0 || jobs_list_frozen)
    return js.j_jobslots;

  reap_dead_jobs ();
  realloc_jobs_list ();

#ifdef DEBUG
  itrace("compact_jobs_list: returning %d", (js.j_lastj || jobs[js.j_lastj]) ? js.j_lastj + 1 : 0);
#endif

  return ((js.j_lastj || jobs[js.j_lastj]) ? js.j_lastj + 1 : 0);
}

/* Delete the job at INDEX from the job list.  Must be called
   with SIGCHLD blocked. */
void
delete_job (job_index, dflags)
     int job_index, dflags;
{
  register JOB *temp;
  PROCESS *proc;
  int ndel;

  if (js.j_jobslots == 0 || jobs_list_frozen)
    return;

  if ((dflags & DEL_WARNSTOPPED) && subshell_environment == 0 && STOPPED (job_index))
    internal_warning (_("deleting stopped job %d with process group %ld"), job_index+1, (long)jobs[job_index]->pgrp);
  temp = jobs[job_index];
  if (temp == 0)
    return;

  if ((dflags & DEL_NOBGPID) == 0)
    {
      proc = find_last_proc (job_index, 0);
      /* Could do this just for J_ASYNC jobs, but we save all. */
      if (proc)
	bgp_add (proc->pid, process_exit_status (proc->status));
    }

  jobs[job_index] = (JOB *)NULL;
  if (temp == js.j_lastmade)
    js.j_lastmade = 0;
  else if (temp == js.j_lastasync)
    js.j_lastasync = 0;

  free (temp->wd);
  ndel = discard_pipeline (temp->pipe);

  js.c_injobs -= ndel;
  if (temp->state == JDEAD)
    {
      js.c_reaped -= ndel;
      js.j_ndead--;
      if (js.c_reaped < 0)
	{
#ifdef DEBUG
	  itrace("delete_job (%d pgrp %d): js.c_reaped (%d) < 0 ndel = %d js.j_ndead = %d", job_index, temp->pgrp, js.c_reaped, ndel, js.j_ndead);
#endif
	  js.c_reaped = 0;
	}
    }

  if (temp->deferred)
    dispose_command (temp->deferred);

  free (temp);

  js.j_njobs--;
  if (js.j_njobs == 0)
    js.j_firstj = js.j_lastj = 0;
  else if (jobs[js.j_firstj] == 0 || jobs[js.j_lastj] == 0)
    reset_job_indices ();

  if (job_index == js.j_current || job_index == js.j_previous)
    reset_current ();
}

/* Must be called with SIGCHLD blocked. */
void
nohup_job (job_index)
     int job_index;
{
  register JOB *temp;

  if (js.j_jobslots == 0)
    return;

  if (temp = jobs[job_index])
    temp->flags |= J_NOHUP;
}

/* Get rid of the data structure associated with a process chain. */
static int
discard_pipeline (chain)
     register PROCESS *chain;
{
  register PROCESS *this, *next;
  int n;

  this = chain;
  n = 0;
  do
    {
      next = this->next;
      FREE (this->command);
      free (this);
      n++;
      this = next;
    }
  while (this != chain);

  return n;
}

/* Add this process to the chain being built in the_pipeline.
   NAME is the command string that will be exec'ed later.
   PID is the process id of the child. */
static void
add_process (name, pid)
     char *name;
     pid_t pid;
{
  PROCESS *t, *p;

#if defined (RECYCLES_PIDS)
  int j;
  p = find_process (pid, 0, &j);
  if (p)
    {
#  ifdef DEBUG
      if (j == NO_JOB)
	internal_warning (_("add_process: process %5ld (%s) in the_pipeline"), (long)p->pid, p->command);
#  endif
      if (PALIVE (p))
        internal_warning (_("add_process: pid %5ld (%s) marked as still alive"), (long)p->pid, p->command);
      p->running = PS_RECYCLED;		/* mark as recycled */
    }
#endif

  t = (PROCESS *)xmalloc (sizeof (PROCESS));
  t->next = the_pipeline;
  t->pid = pid;
  WSTATUS (t->status) = 0;
  t->running = PS_RUNNING;
  t->command = name;
  the_pipeline = t;

  if (t->next == 0)
    t->next = t;
  else
    {
      p = t->next;
      while (p->next != t->next)
	p = p->next;
      p->next = t;
    }
}

#if 0
/* Take the last job and make it the first job.  Must be called with
   SIGCHLD blocked. */
int
rotate_the_pipeline ()
{
  PROCESS *p;

  if (the_pipeline->next == the_pipeline)
    return;
  for (p = the_pipeline; p->next != the_pipeline; p = p->next)
    ;
  the_pipeline = p;
}

/* Reverse the order of the processes in the_pipeline.  Must be called with
   SIGCHLD blocked. */
int
reverse_the_pipeline ()
{
  PROCESS *p, *n;

  if (the_pipeline->next == the_pipeline)
    return;

  for (p = the_pipeline; p->next != the_pipeline; p = p->next)
    ;
  p->next = (PROCESS *)NULL;

  n = REVERSE_LIST (the_pipeline, PROCESS *);

  the_pipeline = n;
  for (p = the_pipeline; p->next; p = p->next)
    ;
  p->next = the_pipeline;
}
#endif

/* Map FUNC over the list of jobs.  If FUNC returns non-zero,
   then it is time to stop mapping, and that is the return value
   for map_over_jobs.  FUNC is called with a JOB, arg1, arg2,
   and INDEX. */
static int
map_over_jobs (func, arg1, arg2)
     sh_job_map_func_t *func;
     int arg1, arg2;
{
  register int i;
  int result;
  sigset_t set, oset;

  if (js.j_jobslots == 0)
    return 0;

  BLOCK_CHILD (set, oset);

  /* XXX could use js.j_firstj here */
  for (i = result = 0; i < js.j_jobslots; i++)
    {
#if defined (DEBUG)
      if (i < js.j_firstj && jobs[i])
	itrace("map_over_jobs: job %d non-null before js.j_firstj (%d)", i, js.j_firstj);
      if (i > js.j_lastj && jobs[i])
	itrace("map_over_jobs: job %d non-null after js.j_lastj (%d)", i, js.j_lastj);
#endif
      if (jobs[i])
	{
	  result = (*func)(jobs[i], arg1, arg2, i);
	  if (result)
	    break;
	}
    }

  UNBLOCK_CHILD (oset);

  return (result);
}

/* Cause all the jobs in the current pipeline to exit. */
void
terminate_current_pipeline ()
{
  if (pipeline_pgrp && pipeline_pgrp != shell_pgrp)
    {
      killpg (pipeline_pgrp, SIGTERM);
      killpg (pipeline_pgrp, SIGCONT);
    }
}

/* Cause all stopped jobs to exit. */
void
terminate_stopped_jobs ()
{
  register int i;

  /* XXX could use js.j_firstj here */
  for (i = 0; i < js.j_jobslots; i++)
    {
      if (jobs[i] && STOPPED (i))
	{
	  killpg (jobs[i]->pgrp, SIGTERM);
	  killpg (jobs[i]->pgrp, SIGCONT);
	}
    }
}

/* Cause all jobs, running or stopped, to receive a hangup signal.  If
   a job is marked J_NOHUP, don't send the SIGHUP. */
void
hangup_all_jobs ()
{
  register int i;

  /* XXX could use js.j_firstj here */
  for (i = 0; i < js.j_jobslots; i++)
    {
      if (jobs[i])
	{
	  if  (jobs[i]->flags & J_NOHUP)
	    continue;
	  killpg (jobs[i]->pgrp, SIGHUP);
	  if (STOPPED (i))
	    killpg (jobs[i]->pgrp, SIGCONT);
	}
    }
}

void
kill_current_pipeline ()
{
  stop_making_children ();
  start_pipeline ();
}

/* Return the pipeline that PID belongs to.  Note that the pipeline
   doesn't have to belong to a job.  Must be called with SIGCHLD blocked.
   If JOBP is non-null, return the index of the job containing PID.  */
static PROCESS *
find_pipeline (pid, alive_only, jobp)
     pid_t pid;
     int alive_only;
     int *jobp;		/* index into jobs list or NO_JOB */
{
  int job;
  PROCESS *p;

  /* See if this process is in the pipeline that we are building. */
  if (jobp)
    *jobp = NO_JOB;
  if (the_pipeline)
    {
      p = the_pipeline;
      do
	{
	  /* Return it if we found it.  Don't ever return a recycled pid. */
	  if (p->pid == pid && ((alive_only == 0 && PRECYCLED(p) == 0) || PALIVE(p)))
	    return (p);

	  p = p->next;
	}
      while (p != the_pipeline);
    }

  job = find_job (pid, alive_only, &p);
  if (jobp)
    *jobp = job;
  return (job == NO_JOB) ? (PROCESS *)NULL : jobs[job]->pipe;
}

/* Return the PROCESS * describing PID.  If JOBP is non-null return the index
   into the jobs array of the job containing PID.  Must be called with
   SIGCHLD blocked. */
static PROCESS *
find_process (pid, alive_only, jobp)
     pid_t pid;
     int alive_only;
     int *jobp;		/* index into jobs list or NO_JOB */
{
  PROCESS *p;

  p = find_pipeline (pid, alive_only, jobp);
  while (p && p->pid != pid)
    p = p->next;
  return p;
}

/* Return the job index that PID belongs to, or NO_JOB if it doesn't
   belong to any job.  Must be called with SIGCHLD blocked. */
static int
find_job (pid, alive_only, procp)
     pid_t pid;
     int alive_only;
     PROCESS **procp;
{
  register int i;
  PROCESS *p;

  /* XXX could use js.j_firstj here, and should check js.j_lastj */
  for (i = 0; i < js.j_jobslots; i++)
    {
#if defined (DEBUG)
      if (i < js.j_firstj && jobs[i])
	itrace("find_job: job %d non-null before js.j_firstj (%d)", i, js.j_firstj);
      if (i > js.j_lastj && jobs[i])
	itrace("find_job: job %d non-null after js.j_lastj (%d)", i, js.j_lastj);
#endif
      if (jobs[i])
	{
	  p = jobs[i]->pipe;

	  do
	    {
	      if (p->pid == pid && ((alive_only == 0 && PRECYCLED(p) == 0) || PALIVE(p)))
		{
		  if (procp)
		    *procp = p;
		  return (i);
		}

	      p = p->next;
	    }
	  while (p != jobs[i]->pipe);
	}
    }

  return (NO_JOB);
}

/* Find a job given a PID.  If BLOCK is non-zero, block SIGCHLD as
   required by find_job. */
int
get_job_by_pid (pid, block)
     pid_t pid;
     int block;
{
  int job;
  sigset_t set, oset;

  if (block)
    BLOCK_CHILD (set, oset);

  job = find_job (pid, 0, NULL);

  if (block)
    UNBLOCK_CHILD (oset);

  return job;
}

/* Print descriptive information about the job with leader pid PID. */
void
describe_pid (pid)
     pid_t pid;
{
  int job;
  sigset_t set, oset;

  BLOCK_CHILD (set, oset);

  job = find_job (pid, 0, NULL);

  if (job != NO_JOB)
    fprintf (stderr, "[%d] %ld\n", job + 1, (long)pid);
  else
    programming_error (_("describe_pid: %ld: no such pid"), (long)pid);

  UNBLOCK_CHILD (oset);
}

static char *
j_strsignal (s)
     int s;
{
  char *x;

  x = strsignal (s);
  if (x == 0)
    {
      x = retcode_name_buffer;
      sprintf (x, _("Signal %d"), s);
    }
  return x;
}

static char *
printable_job_status (j, p, format)
     int j;
     PROCESS *p;
     int format;
{
  static char *temp;
  int es;

  temp = _("Done");

  if (STOPPED (j) && format == 0)
    {
      if (posixly_correct == 0 || p == 0 || (WIFSTOPPED (p->status) == 0))
	temp = _("Stopped");
      else
	{
	  temp = retcode_name_buffer;
	  sprintf (temp, _("Stopped(%s)"), signal_name (WSTOPSIG (p->status)));
	}
    }
  else if (RUNNING (j))
    temp = _("Running");
  else
    {
      if (WIFSTOPPED (p->status))
	temp = j_strsignal (WSTOPSIG (p->status));
      else if (WIFSIGNALED (p->status))
	temp = j_strsignal (WTERMSIG (p->status));
      else if (WIFEXITED (p->status))
	{
	  temp = retcode_name_buffer;
	  es = WEXITSTATUS (p->status);
	  if (es == 0)
	    strcpy (temp, _("Done"));
	  else if (posixly_correct)
	    sprintf (temp, _("Done(%d)"), es);
	  else
	    sprintf (temp, _("Exit %d"), es);
	}
      else
	temp = _("Unknown status");
    }

  return temp;
}

/* This is the way to print out information on a job if you
   know the index.  FORMAT is:

    JLIST_NORMAL)   [1]+ Running	   emacs
    JLIST_LONG  )   [1]+ 2378 Running      emacs
    -1	  )   [1]+ 2378	      emacs

    JLIST_NORMAL)   [1]+ Stopped	   ls | more
    JLIST_LONG  )   [1]+ 2369 Stopped      ls
			 2367	    | more
    JLIST_PID_ONLY)
	Just list the pid of the process group leader (really
	the process group).
    JLIST_CHANGED_ONLY)
	Use format JLIST_NORMAL, but list only jobs about which
	the user has not been notified. */

/* Print status for pipeline P.  If JOB_INDEX is >= 0, it is the index into
   the JOBS array corresponding to this pipeline.  FORMAT is as described
   above.  Must be called with SIGCHLD blocked.

   If you're printing a pipeline that's not in the jobs array, like the
   current pipeline as it's being created, pass -1 for JOB_INDEX */
static void
print_pipeline (p, job_index, format, stream)
     PROCESS *p;
     int job_index, format;
     FILE *stream;
{
  PROCESS *first, *last, *show;
  int es, name_padding;
  char *temp;

  if (p == 0)
    return;

  first = last = p;
  while (last->next != first)
    last = last->next;

  for (;;)
    {
      if (p != first)
	fprintf (stream, format ? "     " : " |");

      if (format != JLIST_STANDARD)
	fprintf (stream, "%5ld", (long)p->pid);

      fprintf (stream, " ");

      if (format > -1 && job_index >= 0)
	{
	  show = format ? p : last;
	  temp = printable_job_status (job_index, show, format);

	  if (p != first)
	    {
	      if (format)
		{
		  if (show->running == first->running &&
		      WSTATUS (show->status) == WSTATUS (first->status))
		    temp = "";
		}
	      else
		temp = (char *)NULL;
	    }

	  if (temp)
	    {
	      fprintf (stream, "%s", temp);

	      es = STRLEN (temp);
	      if (es == 0)
		es = 2;	/* strlen ("| ") */
	      name_padding = LONGEST_SIGNAL_DESC - es;

	      fprintf (stream, "%*s", name_padding, "");

	      if ((WIFSTOPPED (show->status) == 0) &&
		  (WIFCONTINUED (show->status) == 0) &&
		  WIFCORED (show->status))
		fprintf (stream, _("(core dumped) "));
	    }
	}

      if (p != first && format)
	fprintf (stream, "| ");

      if (p->command)
	fprintf (stream, "%s", p->command);

      if (p == last && job_index >= 0)
	{
	  temp = current_working_directory ();

	  if (RUNNING (job_index) && (IS_FOREGROUND (job_index) == 0))
	    fprintf (stream, " &");

	  if (strcmp (temp, jobs[job_index]->wd) != 0)
	    fprintf (stream,
	      _("  (wd: %s)"), polite_directory_format (jobs[job_index]->wd));
	}

      if (format || (p == last))
	{
	  /* We need to add a CR only if this is an interactive shell, and
	     we're reporting the status of a completed job asynchronously.
	     We can't really check whether this particular job is being
	     reported asynchronously, so just add the CR if the shell is
	     currently interactive and asynchronous notification is enabled. */
	  if (asynchronous_notification && interactive)
	    fprintf (stream, "\r\n");
	  else
	    fprintf (stream, "\n");
	}

      if (p == last)
	break;
      p = p->next;
    }
  fflush (stream);
}

/* Print information to STREAM about jobs[JOB_INDEX] according to FORMAT.
   Must be called with SIGCHLD blocked or queued with queue_sigchld */
static void
pretty_print_job (job_index, format, stream)
     int job_index, format;
     FILE *stream;
{
  register PROCESS *p;

  /* Format only pid information about the process group leader? */
  if (format == JLIST_PID_ONLY)
    {
      fprintf (stream, "%ld\n", (long)jobs[job_index]->pipe->pid);
      return;
    }

  if (format == JLIST_CHANGED_ONLY)
    {
      if (IS_NOTIFIED (job_index))
	return;
      format = JLIST_STANDARD;
    }

  if (format != JLIST_NONINTERACTIVE)
    fprintf (stream, "[%d]%c ", job_index + 1,
	      (job_index == js.j_current) ? '+':
		(job_index == js.j_previous) ? '-' : ' ');

  if (format == JLIST_NONINTERACTIVE)
    format = JLIST_LONG;

  p = jobs[job_index]->pipe;

  print_pipeline (p, job_index, format, stream);

  /* We have printed information about this job.  When the job's
     status changes, waitchld () sets the notification flag to 0. */
  jobs[job_index]->flags |= J_NOTIFIED;
}

static int
print_job (job, format, state, job_index)
     JOB *job;
     int format, state, job_index;
{
  if (state == -1 || (JOB_STATE)state == job->state)
    pretty_print_job (job_index, format, stdout);
  return (0);
}

void
list_one_job (job, format, ignore, job_index)
     JOB *job;
     int format, ignore, job_index;
{
  pretty_print_job (job_index, format, stdout);
}

void
list_stopped_jobs (format)
     int format;
{
  cleanup_dead_jobs ();
  map_over_jobs (print_job, format, (int)JSTOPPED);
}

void
list_running_jobs (format)
     int format;
{
  cleanup_dead_jobs ();
  map_over_jobs (print_job, format, (int)JRUNNING);
}

/* List jobs.  If FORMAT is non-zero, then the long form of the information
   is printed, else just a short version. */
void
list_all_jobs (format)
     int format;
{
  cleanup_dead_jobs ();
  map_over_jobs (print_job, format, -1);
}

/* Fork, handling errors.  Returns the pid of the newly made child, or 0.
   COMMAND is just for remembering the name of the command; we don't do
   anything else with it.  ASYNC_P says what to do with the tty.  If
   non-zero, then don't give it away. */
pid_t
make_child (command, async_p)
     char *command;
     int async_p;
{
  int forksleep;
  sigset_t set, oset;
  pid_t pid;

  sigemptyset (&set);
  sigaddset (&set, SIGCHLD);
  sigaddset (&set, SIGINT);
  sigemptyset (&oset);
  sigprocmask (SIG_BLOCK, &set, &oset);

  making_children ();

  forksleep = 1;

#if defined (BUFFERED_INPUT)
  /* If default_buffered_input is active, we are reading a script.  If
     the command is asynchronous, we have already duplicated /dev/null
     as fd 0, but have not changed the buffered stream corresponding to
     the old fd 0.  We don't want to sync the stream in this case. */
  if (default_buffered_input != -1 &&
      (!async_p || default_buffered_input > 0))
    sync_buffered_stream (default_buffered_input);
#endif /* BUFFERED_INPUT */

  /* Create the child, handle severe errors.  Retry on EAGAIN. */
  while ((pid = fork ()) < 0 && errno == EAGAIN && forksleep < FORKSLEEP_MAX)
    {
#if 0		/* for bash-4.2 */
      /* If we can't create any children, try to reap some dead ones. */
      waitchld (-1, 0);
#endif
      sys_error ("fork: retry");
      if (sleep (forksleep) != 0)
	break;
      forksleep <<= 1;
    }

  if (pid < 0)
    {
      sys_error ("fork");

      /* Kill all of the processes in the current pipeline. */
      terminate_current_pipeline ();

      /* Discard the current pipeline, if any. */
      if (the_pipeline)
	kill_current_pipeline ();

      last_command_exit_value = EX_NOEXEC;
      throw_to_top_level ();	/* Reset signals, etc. */
    }

  if (pid == 0)
    {
      /* In the child.  Give this child the right process group, set the
	 signals to the default state for a new process. */
      pid_t mypid;

      mypid = getpid ();
#if defined (BUFFERED_INPUT)
      /* Close default_buffered_input if it's > 0.  We don't close it if it's
	 0 because that's the file descriptor used when redirecting input,
	 and it's wrong to close the file in that case. */
      unset_bash_input (0);
#endif /* BUFFERED_INPUT */

      /* Restore top-level signal mask. */
      sigprocmask (SIG_SETMASK, &top_level_mask, (sigset_t *)NULL);

      if (job_control)
	{
	  /* All processes in this pipeline belong in the same
	     process group. */

	  if (pipeline_pgrp == 0)	/* This is the first child. */
	    pipeline_pgrp = mypid;

	  /* Check for running command in backquotes. */
	  if (pipeline_pgrp == shell_pgrp)
	    ignore_tty_job_signals ();
	  else
	    default_tty_job_signals ();

	  /* Set the process group before trying to mess with the terminal's
	     process group.  This is mandated by POSIX. */
	  /* This is in accordance with the Posix 1003.1 standard,
	     section B.7.2.4, which says that trying to set the terminal
	     process group with tcsetpgrp() to an unused pgrp value (like
	     this would have for the first child) is an error.  Section
	     B.4.3.3, p. 237 also covers this, in the context of job control
	     shells. */
	  if (setpgid (mypid, pipeline_pgrp) < 0)
	    sys_error (_("child setpgid (%ld to %ld)"), (long)mypid, (long)pipeline_pgrp);

	  /* By convention (and assumption above), if
	     pipeline_pgrp == shell_pgrp, we are making a child for
	     command substitution.
	     In this case, we don't want to give the terminal to the
	     shell's process group (we could be in the middle of a
	     pipeline, for example). */
	  if (async_p == 0 && pipeline_pgrp != shell_pgrp && ((subshell_environment&SUBSHELL_ASYNC) == 0))
	    give_terminal_to (pipeline_pgrp, 0);

#if defined (PGRP_PIPE)
	  if (pipeline_pgrp == mypid)
	    pipe_read (pgrp_pipe);
#endif
	}
      else			/* Without job control... */
	{
	  if (pipeline_pgrp == 0)
	    pipeline_pgrp = shell_pgrp;

	  /* If these signals are set to SIG_DFL, we encounter the curious
	     situation of an interactive ^Z to a running process *working*
	     and stopping the process, but being unable to do anything with
	     that process to change its state.  On the other hand, if they
	     are set to SIG_IGN, jobs started from scripts do not stop when
	     the shell running the script gets a SIGTSTP and stops. */

	  default_tty_job_signals ();
	}

#if defined (PGRP_PIPE)
      /* Release the process group pipe, since our call to setpgid ()
	 is done.  The last call to sh_closepipe is done in stop_pipeline. */
      sh_closepipe (pgrp_pipe);
#endif /* PGRP_PIPE */

#if 0
      /* Don't set last_asynchronous_pid in the child */
      if (async_p)
	last_asynchronous_pid = mypid;		/* XXX */
      else
#endif
#if defined (RECYCLES_PIDS)
      if (last_asynchronous_pid == mypid)
        /* Avoid pid aliasing.  1 seems like a safe, unusual pid value. */
	last_asynchronous_pid = 1;
#endif
    }
  else
    {
      /* In the parent.  Remember the pid of the child just created
	 as the proper pgrp if this is the first child. */

      if (first_pid == NO_PID)
	first_pid = pid;
      else if (pid_wrap == -1 && pid < first_pid)
	pid_wrap = 0;
      else if (pid_wrap == 0 && pid >= first_pid)
	pid_wrap = 1;

      if (job_control)
	{
	  if (pipeline_pgrp == 0)
	    {
	      pipeline_pgrp = pid;
	      /* Don't twiddle terminal pgrps in the parent!  This is the bug,
		 not the good thing of twiddling them in the child! */
	      /* give_terminal_to (pipeline_pgrp, 0); */
	    }
	  /* This is done on the recommendation of the Rationale section of
	     the POSIX 1003.1 standard, where it discusses job control and
	     shells.  It is done to avoid possible race conditions. (Ref.
	     1003.1 Rationale, section B.4.3.3, page 236). */
	  setpgid (pid, pipeline_pgrp);
	}
      else
	{
	  if (pipeline_pgrp == 0)
	    pipeline_pgrp = shell_pgrp;
	}

      /* Place all processes into the jobs array regardless of the
	 state of job_control. */
      add_process (command, pid);

      if (async_p)
	last_asynchronous_pid = pid;
#if defined (RECYCLES_PIDS)
      else if (last_asynchronous_pid == pid)
        /* Avoid pid aliasing.  1 seems like a safe, unusual pid value. */
	last_asynchronous_pid = 1;
#endif

      if (pid_wrap > 0)
	delete_old_job (pid);

#if !defined (RECYCLES_PIDS)
      /* Only check for saved status if we've saved more than CHILD_MAX
	 statuses, unless the system recycles pids. */
      if ((js.c_reaped + bgpids.npid) >= js.c_childmax)
#endif
	bgp_delete (pid);		/* new process, discard any saved status */

      last_made_pid = pid;

      /* keep stats */
      js.c_totforked++;
      js.c_living++;

      /* Unblock SIGINT and SIGCHLD unless creating a pipeline, in which case
	 SIGCHLD remains blocked until all commands in the pipeline have been
	 created. */
      sigprocmask (SIG_SETMASK, &oset, (sigset_t *)NULL);
    }

  return (pid);
}

/* These two functions are called only in child processes. */
void
ignore_tty_job_signals ()
{
  set_signal_handler (SIGTSTP, SIG_IGN);
  set_signal_handler (SIGTTIN, SIG_IGN);
  set_signal_handler (SIGTTOU, SIG_IGN);
}

void
default_tty_job_signals ()
{
  set_signal_handler (SIGTSTP, SIG_DFL);
  set_signal_handler (SIGTTIN, SIG_DFL);
  set_signal_handler (SIGTTOU, SIG_DFL);
}

/* When we end a job abnormally, or if we stop a job, we set the tty to the
   state kept in here.  When a job ends normally, we set the state in here
   to the state of the tty. */

static TTYSTRUCT shell_tty_info;

#if defined (NEW_TTY_DRIVER)
static struct tchars shell_tchars;
static struct ltchars shell_ltchars;
#endif /* NEW_TTY_DRIVER */

#if defined (NEW_TTY_DRIVER) && defined (DRAIN_OUTPUT)
/* Since the BSD tty driver does not allow us to change the tty modes
   while simultaneously waiting for output to drain and preserving
   typeahead, we have to drain the output ourselves before calling
   ioctl.  We cheat by finding the length of the output queue, and
   using select to wait for an appropriate length of time.  This is
   a hack, and should be labeled as such (it's a hastily-adapted
   mutation of a `usleep' implementation).  It's only reason for
   existing is the flaw in the BSD tty driver. */

static int ttspeeds[] =
{
  0, 50, 75, 110, 134, 150, 200, 300, 600, 1200,
  1800, 2400, 4800, 9600, 19200, 38400
};

static void
draino (fd, ospeed)
     int fd, ospeed;
{
  register int delay = ttspeeds[ospeed];
  int n;

  if (!delay)
    return;

  while ((ioctl (fd, TIOCOUTQ, &n) == 0) && n)
    {
      if (n > (delay / 100))
	{
	  struct timeval tv;

	  n *= 10;		/* 2 bits more for conservativeness. */
	  tv.tv_sec = n / delay;
	  tv.tv_usec = ((n % delay) * 1000000) / delay;
	  select (fd, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv);
	}
      else
	break;
    }
}
#endif /* NEW_TTY_DRIVER && DRAIN_OUTPUT */

/* Return the fd from which we are actually getting input. */
#define input_tty() (shell_tty != -1) ? shell_tty : fileno (stderr)

/* Fill the contents of shell_tty_info with the current tty info. */
int
get_tty_state ()
{
  int tty;

  tty = input_tty ();
  if (tty != -1)
    {
#if defined (NEW_TTY_DRIVER)
      ioctl (tty, TIOCGETP, &shell_tty_info);
      ioctl (tty, TIOCGETC, &shell_tchars);
      ioctl (tty, TIOCGLTC, &shell_ltchars);
#endif /* NEW_TTY_DRIVER */

#if defined (TERMIO_TTY_DRIVER)
      ioctl (tty, TCGETA, &shell_tty_info);
#endif /* TERMIO_TTY_DRIVER */

#if defined (TERMIOS_TTY_DRIVER)
      if (tcgetattr (tty, &shell_tty_info) < 0)
	{
#if 0
	  /* Only print an error message if we're really interactive at
	     this time. */
	  if (interactive)
	    sys_error ("[%ld: %d (%d)] tcgetattr", (long)getpid (), shell_level, tty);
#endif
	  return -1;
	}
#endif /* TERMIOS_TTY_DRIVER */
      if (check_window_size)
	get_new_window_size (0, (int *)0, (int *)0);
    }
  return 0;
}

/* Make the current tty use the state in shell_tty_info. */
int
set_tty_state ()
{
  int tty;

  tty = input_tty ();
  if (tty != -1)
    {
#if defined (NEW_TTY_DRIVER)
#  if defined (DRAIN_OUTPUT)
      draino (tty, shell_tty_info.sg_ospeed);
#  endif /* DRAIN_OUTPUT */
      ioctl (tty, TIOCSETN, &shell_tty_info);
      ioctl (tty, TIOCSETC, &shell_tchars);
      ioctl (tty, TIOCSLTC, &shell_ltchars);
#endif /* NEW_TTY_DRIVER */

#if defined (TERMIO_TTY_DRIVER)
      ioctl (tty, TCSETAW, &shell_tty_info);
#endif /* TERMIO_TTY_DRIVER */

#if defined (TERMIOS_TTY_DRIVER)
      if (tcsetattr (tty, TCSADRAIN, &shell_tty_info) < 0)
	{
	  /* Only print an error message if we're really interactive at
	     this time. */
	  if (interactive)
	    sys_error ("[%ld: %d (%d)] tcsetattr", (long)getpid (), shell_level, tty);
	  return -1;
	}
#endif /* TERMIOS_TTY_DRIVER */
    }
  return 0;
}

/* Given an index into the jobs array JOB, return the PROCESS struct of the last
   process in that job's pipeline.  This is the one whose exit status
   counts.  Must be called with SIGCHLD blocked or queued. */
static PROCESS *
find_last_proc (job, block)
     int job;
     int block;
{
  register PROCESS *p;
  sigset_t set, oset;

  if (block)
    BLOCK_CHILD (set, oset);

  p = jobs[job]->pipe;
  while (p && p->next != jobs[job]->pipe)
    p = p->next;

  if (block)
    UNBLOCK_CHILD (oset);

  return (p);
}

static pid_t
find_last_pid (job, block)
     int job;
     int block;
{
  PROCESS *p;

  p = find_last_proc (job, block);
  /* Possible race condition here. */
  return p->pid;
}     

/* Wait for a particular child of the shell to finish executing.
   This low-level function prints an error message if PID is not
   a child of this shell.  It returns -1 if it fails, or whatever
   wait_for returns otherwise.  If the child is not found in the
   jobs table, it returns 127. */
int
wait_for_single_pid (pid)
     pid_t pid;
{
  register PROCESS *child;
  sigset_t set, oset;
  int r, job;

  BLOCK_CHILD (set, oset);
  child = find_pipeline (pid, 0, (int *)NULL);
  UNBLOCK_CHILD (oset);

  if (child == 0)
    {
      r = bgp_search (pid);
      if (r >= 0)
	return r;
    }

  if (child == 0)
    {
      internal_error (_("wait: pid %ld is not a child of this shell"), (long)pid);
      return (127);
    }

  r = wait_for (pid);

  /* POSIX.2: if we just waited for a job, we can remove it from the jobs
     table. */
  BLOCK_CHILD (set, oset);
  job = find_job (pid, 0, NULL);
  if (job != NO_JOB && jobs[job] && DEADJOB (job))
    jobs[job]->flags |= J_NOTIFIED;
  UNBLOCK_CHILD (oset);

  /* If running in posix mode, remove the job from the jobs table immediately */
  if (posixly_correct)
    {
      cleanup_dead_jobs ();
      bgp_delete (pid);
    }

  return r;
}

/* Wait for all of the backgrounds of this shell to finish. */
void
wait_for_background_pids ()
{
  register int i, r, waited_for;
  sigset_t set, oset;
  pid_t pid;

  for (waited_for = 0;;)
    {
      BLOCK_CHILD (set, oset);

      /* find first running job; if none running in foreground, break */
      /* XXX could use js.j_firstj and js.j_lastj here */
      for (i = 0; i < js.j_jobslots; i++)
	{
#if defined (DEBUG)
	  if (i < js.j_firstj && jobs[i])
	    itrace("wait_for_background_pids: job %d non-null before js.j_firstj (%d)", i, js.j_firstj);
	  if (i > js.j_lastj && jobs[i])
	    itrace("wait_for_background_pids: job %d non-null after js.j_lastj (%d)", i, js.j_lastj);
#endif
	  if (jobs[i] && RUNNING (i) && IS_FOREGROUND (i) == 0)
	    break;
	}
      if (i == js.j_jobslots)
	{
	  UNBLOCK_CHILD (oset);
	  break;
	}

      /* now wait for the last pid in that job. */
      pid = find_last_pid (i, 0);
      UNBLOCK_CHILD (oset);
      QUIT;
      errno = 0;		/* XXX */
      r = wait_for_single_pid (pid);
      if (r == -1)
	{
	  /* If we're mistaken about job state, compensate. */
	  if (errno == ECHILD)
	    mark_all_jobs_as_dead ();
	}
      else
	waited_for++;
    }

  /* POSIX.2 says the shell can discard the statuses of all completed jobs if
     `wait' is called with no arguments. */
  mark_dead_jobs_as_notified (1);
  cleanup_dead_jobs ();
  bgp_clear ();
}

/* Make OLD_SIGINT_HANDLER the SIGINT signal handler. */
#define INVALID_SIGNAL_HANDLER (SigHandler *)wait_for_background_pids
static SigHandler *old_sigint_handler = INVALID_SIGNAL_HANDLER;

static void
restore_sigint_handler ()
{
  if (old_sigint_handler != INVALID_SIGNAL_HANDLER)
    {
      set_signal_handler (SIGINT, old_sigint_handler);
      old_sigint_handler = INVALID_SIGNAL_HANDLER;
    }
}

static int wait_sigint_received;

/* Handle SIGINT while we are waiting for children in a script to exit.
   The `wait' builtin should be interruptible, but all others should be
   effectively ignored (i.e. not cause the shell to exit). */
static sighandler
wait_sigint_handler (sig)
     int sig;
{
  SigHandler *sigint_handler;

  if (interrupt_immediately ||
      (this_shell_builtin && this_shell_builtin == wait_builtin))
    {
      last_command_exit_value = EXECUTION_FAILURE;
      restore_sigint_handler ();
      /* If we got a SIGINT while in `wait', and SIGINT is trapped, do
	 what POSIX.2 says (see builtins/wait.def for more info). */
      if (this_shell_builtin && this_shell_builtin == wait_builtin &&
	  signal_is_trapped (SIGINT) &&
	  ((sigint_handler = trap_to_sighandler (SIGINT)) == trap_handler))
	{
	  interrupt_immediately = 0;
	  trap_handler (SIGINT);	/* set pending_traps[SIGINT] */
	  wait_signal_received = SIGINT;
	  longjmp (wait_intr_buf, 1);
	}
      
      ADDINTERRUPT;
      QUIT;
    }

  /* XXX - should this be interrupt_state?  If it is, the shell will act
     as if it got the SIGINT interrupt. */
  wait_sigint_received = 1;

  /* Otherwise effectively ignore the SIGINT and allow the running job to
     be killed. */
  SIGRETURN (0);
}

static int
process_exit_signal (status)
     WAIT status;
{
  return (WIFSIGNALED (status) ? WTERMSIG (status) : 0);
}

static int
process_exit_status (status)
     WAIT status;
{
  if (WIFSIGNALED (status))
    return (128 + WTERMSIG (status));
  else if (WIFSTOPPED (status) == 0)
    return (WEXITSTATUS (status));
  else
    return (EXECUTION_SUCCESS);
}

static WAIT
job_signal_status (job)
     int job;
{
  register PROCESS *p;
  WAIT s;

  p = jobs[job]->pipe;
  do
    {
      s = p->status;
      if (WIFSIGNALED(s) || WIFSTOPPED(s))
	break;
      p = p->next;
    }
  while (p != jobs[job]->pipe);

  return s;
}
  
/* Return the exit status of the last process in the pipeline for job JOB.
   This is the exit status of the entire job. */
static WAIT
raw_job_exit_status (job)
     int job;
{
  register PROCESS *p;
  int fail;
  WAIT ret;

  if (pipefail_opt)
    {
      fail = 0;
      p = jobs[job]->pipe;
      do
	{
	  if (WSTATUS (p->status) != EXECUTION_SUCCESS)
	    fail = WSTATUS(p->status);
	  p = p->next;
	}
      while (p != jobs[job]->pipe);
      WSTATUS (ret) = fail;
      return ret;
    }

  for (p = jobs[job]->pipe; p->next != jobs[job]->pipe; p = p->next)
    ;
  return (p->status);
}

/* Return the exit status of job JOB.  This is the exit status of the last
   (rightmost) process in the job's pipeline, modified if the job was killed
   by a signal or stopped. */
static int
job_exit_status (job)
     int job;
{
  return (process_exit_status (raw_job_exit_status (job)));
}

static int
job_exit_signal (job)
     int job;
{
  return (process_exit_signal (raw_job_exit_status (job)));
}

#define FIND_CHILD(pid, child) \
  do \
    { \
      child = find_pipeline (pid, 0, (int *)NULL); \
      if (child == 0) \
	{ \
	  give_terminal_to (shell_pgrp, 0); \
	  UNBLOCK_CHILD (oset); \
	  internal_error (_("wait_for: No record of process %ld"), (long)pid); \
	  restore_sigint_handler (); \
	  return (termination_state = 127); \
	} \
    } \
  while (0)

/* Wait for pid (one of our children) to terminate, then
   return the termination state.  Returns 127 if PID is not found in
   the jobs table.  Returns -1 if waitchld() returns -1, indicating
   that there are no unwaited-for child processes. */
int
wait_for (pid)
     pid_t pid;
{
  int job, termination_state, r;
  WAIT s;
  register PROCESS *child;
  sigset_t set, oset;
  register PROCESS *p;

  /* In the case that this code is interrupted, and we longjmp () out of it,
     we are relying on the code in throw_to_top_level () to restore the
     top-level signal mask. */
  BLOCK_CHILD (set, oset);

  /* Ignore interrupts while waiting for a job run without job control
     to finish.  We don't want the shell to exit if an interrupt is
     received, only if one of the jobs run is killed via SIGINT.  If
     job control is not set, the job will be run in the same pgrp as
     the shell, and the shell will see any signals the job gets.  In
     fact, we want this set every time the waiting shell and the waited-
     for process are in the same process group, including command
     substitution. */

  /* This is possibly a race condition -- should it go in stop_pipeline? */
  wait_sigint_received = 0;
  if (job_control == 0 || (subshell_environment&SUBSHELL_COMSUB))
    {
      old_sigint_handler = set_signal_handler (SIGINT, wait_sigint_handler);
      if (old_sigint_handler == SIG_IGN)
	set_signal_handler (SIGINT, old_sigint_handler);
    }

  termination_state = last_command_exit_value;

  if (interactive && job_control == 0)
    QUIT;
  /* Check for terminating signals and exit the shell if we receive one */
  CHECK_TERMSIG;

  /* If we say wait_for (), then we have a record of this child somewhere.
     If it and none of its peers are running, don't call waitchld(). */

  job = NO_JOB;
  do
    {
      FIND_CHILD (pid, child);

      /* If this child is part of a job, then we are really waiting for the
	 job to finish.  Otherwise, we are waiting for the child to finish.
	 We check for JDEAD in case the job state has been set by waitchld
	 after receipt of a SIGCHLD. */
      if (job == NO_JOB)
	job = find_job (pid, 0, NULL);

      /* waitchld() takes care of setting the state of the job.  If the job
	 has already exited before this is called, sigchld_handler will have
	 called waitchld and the state will be set to JDEAD. */

      if (PRUNNING(child) || (job != NO_JOB && RUNNING (job)))
	{
#if defined (WAITPID_BROKEN)    /* SCOv4 */
	  sigset_t suspend_set;
	  sigemptyset (&suspend_set);
	  sigsuspend (&suspend_set);
#else /* !WAITPID_BROKEN */
#  if defined (MUST_UNBLOCK_CHLD)
	  struct sigaction act, oact;
	  sigset_t nullset, chldset;

	  sigemptyset (&nullset);
	  sigemptyset (&chldset);
	  sigprocmask (SIG_SETMASK, &nullset, &chldset);
	  act.sa_handler = SIG_DFL;
	  sigemptyset (&act.sa_mask);
	  sigemptyset (&oact.sa_mask);
	  act.sa_flags = 0;
	  sigaction (SIGCHLD, &act, &oact);
#  endif
	  queue_sigchld = 1;
	  r = waitchld (pid, 1);
#  if defined (MUST_UNBLOCK_CHLD)
	  sigaction (SIGCHLD, &oact, (struct sigaction *)NULL);
	  sigprocmask (SIG_SETMASK, &chldset, (sigset_t *)NULL);
#  endif
	  queue_sigchld = 0;
	  if (r == -1 && errno == ECHILD && this_shell_builtin == wait_builtin)
	    {
	      termination_state = -1;
	      goto wait_for_return;
	    }

	  /* If child is marked as running, but waitpid() returns -1/ECHILD,
	     there is something wrong.  Somewhere, wait should have returned
	     that child's pid.  Mark the child as not running and the job,
	     if it exists, as JDEAD. */
	  if (r == -1 && errno == ECHILD)
	    {
	      child->running = PS_DONE;
	      WSTATUS (child->status) = 0;	/* XXX -- can't find true status */
	      js.c_living = 0;		/* no living child processes */
	      if (job != NO_JOB)
		{
		  jobs[job]->state = JDEAD;
		  js.c_reaped++;
		  js.j_ndead++;
		}
	    }
#endif /* WAITPID_BROKEN */
	}

      /* If the shell is interactive, and job control is disabled, see
	 if the foreground process has died due to SIGINT and jump out
	 of the wait loop if it has.  waitchld has already restored the
	 old SIGINT signal handler. */
      if (interactive && job_control == 0)
	QUIT;
      /* Check for terminating signals and exit the shell if we receive one */
      CHECK_TERMSIG;
    }
  while (PRUNNING (child) || (job != NO_JOB && RUNNING (job)));

  /* The exit state of the command is either the termination state of the
     child, or the termination state of the job.  If a job, the status
     of the last child in the pipeline is the significant one.  If the command
     or job was terminated by a signal, note that value also. */
  termination_state = (job != NO_JOB) ? job_exit_status (job)
				      : process_exit_status (child->status);
  last_command_exit_signal = (job != NO_JOB) ? job_exit_signal (job)
					     : process_exit_signal (child->status);

  /* XXX */
  if ((job != NO_JOB && JOBSTATE (job) == JSTOPPED) || WIFSTOPPED (child->status))
    termination_state = 128 + WSTOPSIG (child->status);

  if (job == NO_JOB || IS_JOBCONTROL (job))
    {
      /* XXX - under what circumstances is a job not present in the jobs
	 table (job == NO_JOB)?
	 	1.  command substitution

	 In the case of command substitution, at least, it's probably not
	 the right thing to give the terminal to the shell's process group,
	 even though there is code in subst.c:command_substitute to work
	 around it.

	 Things that don't:
		$PROMPT_COMMAND execution
		process substitution
       */
#if 0
if (job == NO_JOB)
  itrace("wait_for: job == NO_JOB, giving the terminal to shell_pgrp (%ld)", (long)shell_pgrp);
#endif
      give_terminal_to (shell_pgrp, 0);
    }

  /* If the command did not exit cleanly, or the job is just
     being stopped, then reset the tty state back to what it
     was before this command.  Reset the tty state and notify
     the user of the job termination only if the shell is
     interactive.  Clean up any dead jobs in either case. */
  if (job != NO_JOB)
    {
      if (interactive_shell && subshell_environment == 0)
	{
	  /* This used to use `child->status'.  That's wrong, however, for
	     pipelines.  `child' is the first process in the pipeline.  It's
	     likely that the process we want to check for abnormal termination
	     or stopping is the last process in the pipeline, especially if
	     it's long-lived and the first process is short-lived.  Since we
	     know we have a job here, we can check all the processes in this
	     job's pipeline and see if one of them stopped or terminated due
	     to a signal.  We might want to change this later to just check
	     the last process in the pipeline.  If no process exits due to a
	     signal, S is left as the status of the last job in the pipeline. */
	  s = job_signal_status (job);

	  if (WIFSIGNALED (s) || WIFSTOPPED (s))
	    {
	      set_tty_state ();

	      /* If the current job was stopped or killed by a signal, and
		 the user has requested it, get a possibly new window size */
	      if (check_window_size && (job == js.j_current || IS_FOREGROUND (job)))
		get_new_window_size (0, (int *)0, (int *)0);
	    }
	  else
	    get_tty_state ();

	  /* If job control is enabled, the job was started with job
	     control, the job was the foreground job, and it was killed
	     by SIGINT, then print a newline to compensate for the kernel
	     printing the ^C without a trailing newline. */
	  if (job_control && IS_JOBCONTROL (job) && IS_FOREGROUND (job) &&
		WIFSIGNALED (s) && WTERMSIG (s) == SIGINT)
	    {
	      /* If SIGINT is not trapped and the shell is in a for, while,
		 or until loop, act as if the shell received SIGINT as
		 well, so the loop can be broken.  This doesn't call the
		 SIGINT signal handler; maybe it should. */
	      if (signal_is_trapped (SIGINT) == 0 && (loop_level || (shell_compatibility_level > 32 && executing_list)))
		ADDINTERRUPT;
	      else
		{
		  putchar ('\n');
		  fflush (stdout);
		}
	    }
	}
      else if ((subshell_environment & SUBSHELL_COMSUB) && wait_sigint_received)
	{
	  /* If waiting for a job in a subshell started to do command
	     substitution, simulate getting and being killed by the SIGINT to
	     pass the status back to our parent. */
	  s = job_signal_status (job);
	
	  if (WIFSIGNALED (s) && WTERMSIG (s) == SIGINT && signal_is_trapped (SIGINT) == 0)
	    {
	      UNBLOCK_CHILD (oset);
	      restore_sigint_handler ();
	      old_sigint_handler = set_signal_handler (SIGINT, SIG_DFL);
	      if (old_sigint_handler == SIG_IGN)
		restore_sigint_handler ();
	      else
		kill (getpid (), SIGINT);
	    }
	}

      /* Moved here from set_job_status_and_cleanup, which is in the SIGCHLD
         signal handler path */
      if (DEADJOB (job) && IS_FOREGROUND (job) /*&& subshell_environment == 0*/)
	setjstatus (job);

      /* If this job is dead, notify the user of the status.  If the shell
	 is interactive, this will display a message on the terminal.  If
	 the shell is not interactive, make sure we turn on the notify bit
	 so we don't get an unwanted message about the job's termination,
	 and so delete_job really clears the slot in the jobs table. */
      notify_and_cleanup ();
    }

wait_for_return:

  UNBLOCK_CHILD (oset);

  /* Restore the original SIGINT signal handler before we return. */
  restore_sigint_handler ();

  return (termination_state);
}

/* Wait for the last process in the pipeline for JOB.  Returns whatever
   wait_for returns: the last process's termination state or -1 if there
   are no unwaited-for child processes or an error occurs. */
int
wait_for_job (job)
     int job;
{
  pid_t pid;
  int r;
  sigset_t set, oset;

  BLOCK_CHILD(set, oset);
  if (JOBSTATE (job) == JSTOPPED)
    internal_warning (_("wait_for_job: job %d is stopped"), job+1);

  pid = find_last_pid (job, 0);
  UNBLOCK_CHILD(oset);
  r = wait_for (pid);

  /* POSIX.2: we can remove the job from the jobs table if we just waited
     for it. */
  BLOCK_CHILD (set, oset);
  if (job != NO_JOB && jobs[job] && DEADJOB (job))
    jobs[job]->flags |= J_NOTIFIED;
  UNBLOCK_CHILD (oset);

  return r;
}

/* Print info about dead jobs, and then delete them from the list
   of known jobs.  This does not actually delete jobs when the
   shell is not interactive, because the dead jobs are not marked
   as notified. */
void
notify_and_cleanup ()
{
  if (jobs_list_frozen)
    return;

  if (interactive || interactive_shell == 0 || sourcelevel)
    notify_of_job_status ();

  cleanup_dead_jobs ();
}

/* Make dead jobs disappear from the jobs array without notification.
   This is used when the shell is not interactive. */
void
reap_dead_jobs ()
{
  mark_dead_jobs_as_notified (0);
  cleanup_dead_jobs ();
}

/* Return the next closest (chronologically) job to JOB which is in
   STATE.  STATE can be JSTOPPED, JRUNNING.  NO_JOB is returned if
   there is no next recent job. */
static int
most_recent_job_in_state (job, state)
     int job;
     JOB_STATE state;
{
  register int i, result;
  sigset_t set, oset;

  BLOCK_CHILD (set, oset);

  for (result = NO_JOB, i = job - 1; i >= 0; i--)
    {
      if (jobs[i] && (JOBSTATE (i) == state))
	{
	  result = i;
	  break;
	}
    }

  UNBLOCK_CHILD (oset);

  return (result);
}

/* Return the newest *stopped* job older than JOB, or NO_JOB if not
   found. */
static int
job_last_stopped (job)
     int job;
{
  return (most_recent_job_in_state (job, JSTOPPED));
}

/* Return the newest *running* job older than JOB, or NO_JOB if not
   found. */
static int
job_last_running (job)
     int job;
{
  return (most_recent_job_in_state (job, JRUNNING));
}

/* Make JOB be the current job, and make previous be useful.  Must be
   called with SIGCHLD blocked. */
static void
set_current_job (job)
     int job;
{
  int candidate;

  if (js.j_current != job)
    {
      js.j_previous = js.j_current;
      js.j_current = job;
    }

  /* First choice for previous job is the old current job. */
  if (js.j_previous != js.j_current &&
      js.j_previous != NO_JOB &&
      jobs[js.j_previous] &&
      STOPPED (js.j_previous))
    return;

  /* Second choice:  Newest stopped job that is older than
     the current job. */
  candidate = NO_JOB;
  if (STOPPED (js.j_current))
    {
      candidate = job_last_stopped (js.j_current);

      if (candidate != NO_JOB)
	{
	  js.j_previous = candidate;
	  return;
	}
    }

  /* If we get here, there is either only one stopped job, in which case it is
     the current job and the previous job should be set to the newest running
     job, or there are only running jobs and the previous job should be set to
     the newest running job older than the current job.  We decide on which
     alternative to use based on whether or not JOBSTATE(js.j_current) is
     JSTOPPED. */

  candidate = RUNNING (js.j_current) ? job_last_running (js.j_current)
				    : job_last_running (js.j_jobslots);

  if (candidate != NO_JOB)
    {
      js.j_previous = candidate;
      return;
    }

  /* There is only a single job, and it is both `+' and `-'. */
  js.j_previous = js.j_current;
}

/* Make current_job be something useful, if it isn't already. */

/* Here's the deal:  The newest non-running job should be `+', and the
   next-newest non-running job should be `-'.  If there is only a single
   stopped job, the js.j_previous is the newest non-running job.  If there
   are only running jobs, the newest running job is `+' and the
   next-newest running job is `-'.  Must be called with SIGCHLD blocked. */

static void
reset_current ()
{
  int candidate;

  if (js.j_jobslots && js.j_current != NO_JOB && jobs[js.j_current] && STOPPED (js.j_current))
    candidate = js.j_current;
  else
    {
      candidate = NO_JOB;

      /* First choice: the previous job. */
      if (js.j_previous != NO_JOB && jobs[js.j_previous] && STOPPED (js.j_previous))
	candidate = js.j_previous;

      /* Second choice: the most recently stopped job. */
      if (candidate == NO_JOB)
	candidate = job_last_stopped (js.j_jobslots);

      /* Third choice: the newest running job. */
      if (candidate == NO_JOB)
	candidate = job_last_running (js.j_jobslots);
    }

  /* If we found a job to use, then use it.  Otherwise, there
     are no jobs period. */
  if (candidate != NO_JOB)
    set_current_job (candidate);
  else
    js.j_current = js.j_previous = NO_JOB;
}

/* Set up the job structures so we know the job and its processes are
   all running. */
static void
set_job_running (job)
     int job;
{
  register PROCESS *p;

  /* Each member of the pipeline is now running. */
  p = jobs[job]->pipe;

  do
    {
      if (WIFSTOPPED (p->status))
	p->running = PS_RUNNING;	/* XXX - could be PS_STOPPED */
      p = p->next;
    }
  while (p != jobs[job]->pipe);

  /* This means that the job is running. */
  JOBSTATE (job) = JRUNNING;
}

/* Start a job.  FOREGROUND if non-zero says to do that.  Otherwise,
   start the job in the background.  JOB is a zero-based index into
   JOBS.  Returns -1 if it is unable to start a job, and the return
   status of the job otherwise. */
int
start_job (job, foreground)
     int job, foreground;
{
  register PROCESS *p;
  int already_running;
  sigset_t set, oset;
  char *wd, *s;
  static TTYSTRUCT save_stty;

  BLOCK_CHILD (set, oset);

  if (DEADJOB (job))
    {
      internal_error (_("%s: job has terminated"), this_command_name);
      UNBLOCK_CHILD (oset);
      return (-1);
    }

  already_running = RUNNING (job);

  if (foreground == 0 && already_running)
    {
      internal_error (_("%s: job %d already in background"), this_command_name, job + 1);
      UNBLOCK_CHILD (oset);
      return (0);		/* XPG6/SUSv3 says this is not an error */
    }

  wd = current_working_directory ();

  /* You don't know about the state of this job.  Do you? */
  jobs[job]->flags &= ~J_NOTIFIED;

  if (foreground)
    {
      set_current_job (job);
      jobs[job]->flags |= J_FOREGROUND;
    }

  /* Tell the outside world what we're doing. */
  p = jobs[job]->pipe;

  if (foreground == 0)
    {
      /* POSIX.2 says `bg' doesn't give any indication about current or
	 previous job. */
      if (posixly_correct == 0)
	s = (job == js.j_current) ? "+ ": ((job == js.j_previous) ? "- " : " ");       
      else
	s = " ";
      printf ("[%d]%s", job + 1, s);
    }

  do
    {
      printf ("%s%s",
	       p->command ? p->command : "",
	       p->next != jobs[job]->pipe? " | " : "");
      p = p->next;
    }
  while (p != jobs[job]->pipe);

  if (foreground == 0)
    printf (" &");

  if (strcmp (wd, jobs[job]->wd) != 0)
    printf ("	(wd: %s)", polite_directory_format (jobs[job]->wd));

  printf ("\n");

  /* Run the job. */
  if (already_running == 0)
    set_job_running (job);

  /* Save the tty settings before we start the job in the foreground. */
  if (foreground)
    {
      get_tty_state ();
      save_stty = shell_tty_info;
      /* Give the terminal to this job. */
      if (IS_JOBCONTROL (job))
	give_terminal_to (jobs[job]->pgrp, 0);
    }
  else
    jobs[job]->flags &= ~J_FOREGROUND;

  /* If the job is already running, then don't bother jump-starting it. */
  if (already_running == 0)
    {
      jobs[job]->flags |= J_NOTIFIED;
      killpg (jobs[job]->pgrp, SIGCONT);
    }

  if (foreground)
    {
      pid_t pid;
      int st;

      pid = find_last_pid (job, 0);
      UNBLOCK_CHILD (oset);
      st = wait_for (pid);
      shell_tty_info = save_stty;
      set_tty_state ();
      return (st);
    }
  else
    {
      reset_current ();
      UNBLOCK_CHILD (oset);
      return (0);
    }
}

/* Give PID SIGNAL.  This determines what job the pid belongs to (if any).
   If PID does belong to a job, and the job is stopped, then CONTinue the
   job after giving it SIGNAL.  Returns -1 on failure.  If GROUP is non-null,
   then kill the process group associated with PID. */
int
kill_pid (pid, sig, group)
     pid_t pid;
     int sig, group;
{
  register PROCESS *p;
  int job, result, negative;
  sigset_t set, oset;

  if (pid < -1)
    {
      pid = -pid;
      group = negative = 1;
    }
  else
    negative = 0;

  result = EXECUTION_SUCCESS;
  if (group)
    {
      BLOCK_CHILD (set, oset);
      p = find_pipeline (pid, 0, &job);

      if (job != NO_JOB)
	{
	  jobs[job]->flags &= ~J_NOTIFIED;

	  /* Kill process in backquotes or one started without job control? */

	  /* If we're passed a pid < -1, just call killpg and see what happens  */
	  if (negative && jobs[job]->pgrp == shell_pgrp)
	    result = killpg (pid, sig);
	  /* If we're killing using job control notification, for example,
	     without job control active, we have to do things ourselves. */
	  else if (jobs[job]->pgrp == shell_pgrp)
	    {
	      p = jobs[job]->pipe;
	      do
		{
		  if (PALIVE (p) == 0)
		    continue;		/* avoid pid recycling problem */
		  kill (p->pid, sig);
		  if (PEXITED (p) && (sig == SIGTERM || sig == SIGHUP))
		    kill (p->pid, SIGCONT);
		  p = p->next;
		}
	      while  (p != jobs[job]->pipe);
	    }
	  else
	    {
	      result = killpg (jobs[job]->pgrp, sig);
	      if (p && STOPPED (job) && (sig == SIGTERM || sig == SIGHUP))
		killpg (jobs[job]->pgrp, SIGCONT);
	      /* If we're continuing a stopped job via kill rather than bg or
		 fg, emulate the `bg' behavior. */
	      if (p && STOPPED (job) && (sig == SIGCONT))
		{
		  set_job_running (job);
		  jobs[job]->flags &= ~J_FOREGROUND;
		  jobs[job]->flags |= J_NOTIFIED;
		}
	    }
	}
      else
	result = killpg (pid, sig);

      UNBLOCK_CHILD (oset);
    }
  else
    result = kill (pid, sig);

  return (result);
}

/* sigchld_handler () flushes at least one of the children that we are
   waiting for.  It gets run when we have gotten a SIGCHLD signal. */
static sighandler
sigchld_handler (sig)
     int sig;
{
  int n, oerrno;

  oerrno = errno;
  REINSTALL_SIGCHLD_HANDLER;
  sigchld++;
  n = 0;
  if (queue_sigchld == 0)
    n = waitchld (-1, 0);
  errno = oerrno;
  SIGRETURN (n);
}

/* waitchld() reaps dead or stopped children.  It's called by wait_for and
   sigchld_handler, and runs until there aren't any children terminating any
   more.
   If BLOCK is 1, this is to be a blocking wait for a single child, although
   an arriving SIGCHLD could cause the wait to be non-blocking.  It returns
   the number of children reaped, or -1 if there are no unwaited-for child
   processes. */
static int
waitchld (wpid, block)
     pid_t wpid;
     int block;
{
  WAIT status;
  PROCESS *child;
  pid_t pid;
  int call_set_current, last_stopped_job, job, children_exited, waitpid_flags;
  static int wcontinued = WCONTINUED;	/* run-time fix for glibc problem */

  call_set_current = children_exited = 0;
  last_stopped_job = NO_JOB;

  do
    {
      /* We don't want to be notified about jobs stopping if job control
	 is not active.  XXX - was interactive_shell instead of job_control */
      waitpid_flags = (job_control && subshell_environment == 0)
			? (WUNTRACED|wcontinued)
			: 0;
      if (sigchld || block == 0)
	waitpid_flags |= WNOHANG;
      /* Check for terminating signals and exit the shell if we receive one */
      CHECK_TERMSIG;

      if (block == 1 && queue_sigchld == 0 && (waitpid_flags & WNOHANG) == 0)
	{
	  internal_warning (_("waitchld: turning on WNOHANG to avoid indefinite block"));
	  waitpid_flags |= WNOHANG;
	}

      pid = WAITPID (-1, &status, waitpid_flags);

      /* WCONTINUED may be rejected by waitpid as invalid even when defined */
      if (wcontinued && pid < 0 && errno == EINVAL)
	{
	  wcontinued = 0;
	  continue;	/* jump back to the test and retry without WCONTINUED */
	}

      /* The check for WNOHANG is to make sure we decrement sigchld only
	 if it was non-zero before we called waitpid. */
      if (sigchld > 0 && (waitpid_flags & WNOHANG))
	sigchld--;
  
      /* If waitpid returns -1 with errno == ECHILD, there are no more
	 unwaited-for child processes of this shell. */
      if (pid < 0 && errno == ECHILD)
	{
	  if (children_exited == 0)
	    return -1;
	  else
	    break;
	}

      /* If waitpid returns 0, there are running children.  If it returns -1,
	 the only other error POSIX says it can return is EINTR. */
      CHECK_TERMSIG;
      if (pid <= 0)
	continue;	/* jumps right to the test */

      /* children_exited is used to run traps on SIGCHLD.  We don't want to
         run the trap if a process is just being continued. */
      if (WIFCONTINUED(status) == 0)
	{
	  children_exited++;
	  js.c_living--;
	}

      /* Locate our PROCESS for this pid. */
      child = find_process (pid, 1, &job);	/* want living procs only */

#if defined (COPROCESS_SUPPORT)
      coproc_pidchk (pid, status);
#endif

      /* It is not an error to have a child terminate that we did
	 not have a record of.  This child could have been part of
	 a pipeline in backquote substitution.  Even so, I'm not
	 sure child is ever non-zero. */
      if (child == 0)
	{
	  if (WIFEXITED (status) || WIFSIGNALED (status))
	    js.c_reaped++;
	  continue;
	}

      /* Remember status, and whether or not the process is running. */
      child->status = status;
      child->running = WIFCONTINUED(status) ? PS_RUNNING : PS_DONE;

      if (PEXITED (child))
	{
	  js.c_totreaped++;
	  if (job != NO_JOB)
	    js.c_reaped++;
	}
        
      if (job == NO_JOB)
	continue;

      call_set_current += set_job_status_and_cleanup (job);

      if (STOPPED (job))
	last_stopped_job = job;
      else if (DEADJOB (job) && last_stopped_job == job)
	last_stopped_job = NO_JOB;
    }
  while ((sigchld || block == 0) && pid > (pid_t)0);

  /* If a job was running and became stopped, then set the current
     job.  Otherwise, don't change a thing. */
  if (call_set_current)
    {
      if (last_stopped_job != NO_JOB)
	set_current_job (last_stopped_job);
      else
	reset_current ();
    }

  /* Call a SIGCHLD trap handler for each child that exits, if one is set. */
  if (job_control && signal_is_trapped (SIGCHLD) && children_exited &&
      trap_list[SIGCHLD] != (char *)IGNORE_SIG)
    {
      if (posixly_correct && this_shell_builtin && this_shell_builtin == wait_builtin)
	{
	  interrupt_immediately = 0;
	  trap_handler (SIGCHLD);	/* set pending_traps[SIGCHLD] */
	  wait_signal_received = SIGCHLD;
	  longjmp (wait_intr_buf, 1);
	}

      run_sigchld_trap (children_exited);
    }

  /* We have successfully recorded the useful information about this process
     that has just changed state.  If we notify asynchronously, and the job
     that this process belongs to is no longer running, then notify the user
     of that fact now. */
  if (asynchronous_notification && interactive)
    notify_of_job_status ();

  return (children_exited);
}

/* Set the status of JOB and perform any necessary cleanup if the job is
   marked as JDEAD.

   Currently, the cleanup activity is restricted to handling any SIGINT
   received while waiting for a foreground job to finish. */
static int
set_job_status_and_cleanup (job)
     int job;
{
  PROCESS *child;
  int tstatus, job_state, any_stopped, any_tstped, call_set_current;
  SigHandler *temp_handler;

  child = jobs[job]->pipe;
  jobs[job]->flags &= ~J_NOTIFIED;

  call_set_current = 0;

  /*
   * COMPUTE JOB STATUS
   */

  /* If all children are not running, but any of them is  stopped, then
     the job is stopped, not dead. */
  job_state = any_stopped = any_tstped = 0;
  do
    {
      job_state |= PRUNNING (child);
#if 0
      if (PEXITED (child) && (WIFSTOPPED (child->status)))
#else
      /* Only checking for WIFSTOPPED now, not for PS_DONE */
      if (PSTOPPED (child))
#endif
	{
	  any_stopped = 1;
	  any_tstped |= interactive && job_control &&
			    (WSTOPSIG (child->status) == SIGTSTP);
	}
      child = child->next;
    }
  while (child != jobs[job]->pipe);

  /* If job_state != 0, the job is still running, so don't bother with
     setting the process exit status and job state unless we're
     transitioning from stopped to running. */
  if (job_state != 0 && JOBSTATE(job) != JSTOPPED)
    return 0;

  /*
   * SET JOB STATUS
   */

  /* The job is either stopped or dead.  Set the state of the job accordingly. */
  if (any_stopped)
    {
      jobs[job]->state = JSTOPPED;
      jobs[job]->flags &= ~J_FOREGROUND;
      call_set_current++;
      /* Suspending a job with SIGTSTP breaks all active loops. */
      if (any_tstped && loop_level)
	breaking = loop_level;
    }
  else if (job_state != 0)	/* was stopped, now running */
    {
      jobs[job]->state = JRUNNING;
      call_set_current++;
    }
  else
    {
      jobs[job]->state = JDEAD;
      js.j_ndead++;

#if 0
      if (IS_FOREGROUND (job))
	setjstatus (job);
#endif

      /* If this job has a cleanup function associated with it, call it
	 with `cleanarg' as the single argument, then set the function
	 pointer to NULL so it is not inadvertently called twice.  The
	 cleanup function is responsible for deallocating cleanarg. */
      if (jobs[job]->j_cleanup)
	{
	  (*jobs[job]->j_cleanup) (jobs[job]->cleanarg);
	  jobs[job]->j_cleanup = (sh_vptrfunc_t *)NULL;
	}
    }

  /*
   * CLEANUP
   *
   * Currently, we just do special things if we got a SIGINT while waiting
   * for a foreground job to complete
   */

  if (JOBSTATE (job) == JDEAD)
    {
      /* If we're running a shell script and we get a SIGINT with a
	 SIGINT trap handler, but the foreground job handles it and
	 does not exit due to SIGINT, run the trap handler but do not
	 otherwise act as if we got the interrupt. */
      if (wait_sigint_received && interactive_shell == 0 &&
	  WIFSIGNALED (child->status) == 0 && IS_FOREGROUND (job) &&
	  signal_is_trapped (SIGINT))
	{
	  int old_frozen;
	  wait_sigint_received = 0;
	  last_command_exit_value = process_exit_status (child->status);

	  old_frozen = jobs_list_frozen;
	  jobs_list_frozen = 1;
	  tstatus = maybe_call_trap_handler (SIGINT);
	  jobs_list_frozen = old_frozen;
	}

      /* If the foreground job is killed by SIGINT when job control is not
	 active, we need to perform some special handling.

	 The check of wait_sigint_received is a way to determine if the
	 SIGINT came from the keyboard (in which case the shell has already
	 seen it, and wait_sigint_received is non-zero, because keyboard
	 signals are sent to process groups) or via kill(2) to the foreground
	 process by another process (or itself).  If the shell did receive the
	 SIGINT, it needs to perform normal SIGINT processing. */
      else if (wait_sigint_received && (WTERMSIG (child->status) == SIGINT) &&
	      IS_FOREGROUND (job) && IS_JOBCONTROL (job) == 0)
	{
	  int old_frozen;

	  wait_sigint_received = 0;

	  /* If SIGINT is trapped, set the exit status so that the trap
	     handler can see it. */
	  if (signal_is_trapped (SIGINT))
	    last_command_exit_value = process_exit_status (child->status);

	  /* If the signal is trapped, let the trap handler get it no matter
	     what and simply return if the trap handler returns.
	    maybe_call_trap_handler() may cause dead jobs to be removed from
	    the job table because of a call to execute_command.  We work
	    around this by setting JOBS_LIST_FROZEN. */
	  old_frozen = jobs_list_frozen;
	  jobs_list_frozen = 1;
	  tstatus = maybe_call_trap_handler (SIGINT);
	  jobs_list_frozen = old_frozen;
	  if (tstatus == 0 && old_sigint_handler != INVALID_SIGNAL_HANDLER)
	    {
	      /* wait_sigint_handler () has already seen SIGINT and
		 allowed the wait builtin to jump out.  We need to
		 call the original SIGINT handler, if necessary.  If
		 the original handler is SIG_DFL, we need to resend
		 the signal to ourselves. */

	      temp_handler = old_sigint_handler;

	      /* Bogus.  If we've reset the signal handler as the result
		 of a trap caught on SIGINT, then old_sigint_handler
		 will point to trap_handler, which now knows nothing about
		 SIGINT (if we reset the sighandler to the default).
		 In this case, we have to fix things up.  What a crock. */
	      if (temp_handler == trap_handler && signal_is_trapped (SIGINT) == 0)
		  temp_handler = trap_to_sighandler (SIGINT);
		restore_sigint_handler ();
	      if (temp_handler == SIG_DFL)
		termsig_handler (SIGINT);
	      else if (temp_handler != SIG_IGN)
		(*temp_handler) (SIGINT);
	    }
	}
    }

  return call_set_current;
}

/* Build the array of values for the $PIPESTATUS variable from the set of
   exit statuses of all processes in the job J. */
static void
setjstatus (j)
     int j;
{
#if defined (ARRAY_VARS)
  register int i;
  register PROCESS *p;

  for (i = 1, p = jobs[j]->pipe; p->next != jobs[j]->pipe; p = p->next, i++)
    ;
  i++;
  if (statsize < i)
    {
      pstatuses = (int *)xrealloc (pstatuses, i * sizeof (int));
      statsize = i;
    }
  i = 0;
  p = jobs[j]->pipe;
  do
    {
      pstatuses[i++] = process_exit_status (p->status);
      p = p->next;
    }
  while (p != jobs[j]->pipe);

  pstatuses[i] = -1;	/* sentinel */
  set_pipestatus_array (pstatuses, i);
#endif
}

void
run_sigchld_trap (nchild)
     int nchild;
{
  char *trap_command;
  int i;

  /* Turn off the trap list during the call to parse_and_execute ()
     to avoid potentially infinite recursive calls.  Preserve the
     values of last_command_exit_value, last_made_pid, and the_pipeline
     around the execution of the trap commands. */
  trap_command = savestring (trap_list[SIGCHLD]);

  begin_unwind_frame ("SIGCHLD trap");
  unwind_protect_int (last_command_exit_value);
  unwind_protect_int (last_command_exit_signal);
  unwind_protect_var (last_made_pid);
  unwind_protect_int (interrupt_immediately);
  unwind_protect_int (jobs_list_frozen);
  unwind_protect_pointer (the_pipeline);
  unwind_protect_pointer (subst_assign_varlist);

  /* We have to add the commands this way because they will be run
     in reverse order of adding.  We don't want maybe_set_sigchld_trap ()
     to reference freed memory. */
  add_unwind_protect (xfree, trap_command);
  add_unwind_protect (maybe_set_sigchld_trap, trap_command);

  subst_assign_varlist = (WORD_LIST *)NULL;
  the_pipeline = (PROCESS *)NULL;

  set_impossible_sigchld_trap ();
  jobs_list_frozen = 1;
  for (i = 0; i < nchild; i++)
    {
      interrupt_immediately = 1;
      parse_and_execute (savestring (trap_command), "trap", SEVAL_NOHIST|SEVAL_RESETLINE);
    }

  run_unwind_frame ("SIGCHLD trap");
}

/* Function to call when you want to notify people of changes
   in job status.  This prints out all jobs which are pending
   notification to stderr, and marks those printed as already
   notified, thus making them candidates for cleanup. */
static void
notify_of_job_status ()
{
  register int job, termsig;
  char *dir;
  sigset_t set, oset;
  WAIT s;

  if (jobs == 0 || js.j_jobslots == 0)
    return;

  if (old_ttou != 0)
    {
      sigemptyset (&set);
      sigaddset (&set, SIGCHLD);
      sigaddset (&set, SIGTTOU);
      sigemptyset (&oset);
      sigprocmask (SIG_BLOCK, &set, &oset);
    }
  else
    queue_sigchld++;

  /* XXX could use js.j_firstj here */
  for (job = 0, dir = (char *)NULL; job < js.j_jobslots; job++)
    {
      if (jobs[job] && IS_NOTIFIED (job) == 0)
	{
	  s = raw_job_exit_status (job);
	  termsig = WTERMSIG (s);

	  /* POSIX.2 says we have to hang onto the statuses of at most the
	     last CHILD_MAX background processes if the shell is running a
	     script.  If the shell is running a script, either from a file
	     or standard input, don't print anything unless the job was
	     killed by a signal. */
	  if (startup_state == 0 && WIFSIGNALED (s) == 0 &&
		((DEADJOB (job) && IS_FOREGROUND (job) == 0) || STOPPED (job)))
	    continue;
	  
#if 0
	  /* If job control is disabled, don't print the status messages.
	     Mark dead jobs as notified so that they get cleaned up.  If
	     startup_state == 2, we were started to run `-c command', so
	     don't print anything. */
	  if ((job_control == 0 && interactive_shell) || startup_state == 2)
#else
	  /* If job control is disabled, don't print the status messages.
	     Mark dead jobs as notified so that they get cleaned up.  If
	     startup_state == 2 and subshell_environment has the
	     SUBSHELL_COMSUB bit turned on, we were started to run a command
	     substitution, so don't print anything. */
	  if ((job_control == 0 && interactive_shell) ||
	      (startup_state == 2 && (subshell_environment & SUBSHELL_COMSUB)))
#endif
	    {
	      /* POSIX.2 compatibility:  if the shell is not interactive,
		 hang onto the job corresponding to the last asynchronous
		 pid until the user has been notified of its status or does
		 a `wait'. */
	      if (DEADJOB (job) && (interactive_shell || (find_last_pid (job, 0) != last_asynchronous_pid)))
		jobs[job]->flags |= J_NOTIFIED;
	      continue;
	    }

	  /* Print info on jobs that are running in the background,
	     and on foreground jobs that were killed by anything
	     except SIGINT (and possibly SIGPIPE). */
	  switch (JOBSTATE (job))
	    {
	    case JDEAD:
	      if (interactive_shell == 0 && termsig && WIFSIGNALED (s) &&
		  termsig != SIGINT &&
#if defined (DONT_REPORT_SIGPIPE)
		  termsig != SIGPIPE &&
#endif
		  signal_is_trapped (termsig) == 0)
		{
		  /* Don't print `0' for a line number. */
		  fprintf (stderr, _("%s: line %d: "), get_name_for_error (), (line_number == 0) ? 1 : line_number);
		  pretty_print_job (job, JLIST_NONINTERACTIVE, stderr);
		}
	      else if (IS_FOREGROUND (job))
		{
#if !defined (DONT_REPORT_SIGPIPE)
		  if (termsig && WIFSIGNALED (s) && termsig != SIGINT)
#else
		  if (termsig && WIFSIGNALED (s) && termsig != SIGINT && termsig != SIGPIPE)
#endif
		    {
		      fprintf (stderr, "%s", j_strsignal (termsig));

		      if (WIFCORED (s))
			fprintf (stderr, _(" (core dumped)"));

		      fprintf (stderr, "\n");
		    }
		}
	      else if (job_control)	/* XXX job control test added */
		{
		  if (dir == 0)
		    dir = current_working_directory ();
		  pretty_print_job (job, JLIST_STANDARD, stderr);
		  if (dir && strcmp (dir, jobs[job]->wd) != 0)
		    fprintf (stderr,
			     _("(wd now: %s)\n"), polite_directory_format (dir));
		}

	      jobs[job]->flags |= J_NOTIFIED;
	      break;

	    case JSTOPPED:
	      fprintf (stderr, "\n");
	      if (dir == 0)
		dir = current_working_directory ();
	      pretty_print_job (job, JLIST_STANDARD, stderr);
	      if (dir && (strcmp (dir, jobs[job]->wd) != 0))
		fprintf (stderr,
			 _("(wd now: %s)\n"), polite_directory_format (dir));
	      jobs[job]->flags |= J_NOTIFIED;
	      break;

	    case JRUNNING:
	    case JMIXED:
	      break;

	    default:
	      programming_error ("notify_of_job_status");
	    }
	}
    }
  if (old_ttou != 0)
    sigprocmask (SIG_SETMASK, &oset, (sigset_t *)NULL);
  else
    queue_sigchld--;
}

/* Initialize the job control mechanism, and set up the tty stuff. */
int
initialize_job_control (force)
     int force;
{
  pid_t t;
  int t_errno;

  t_errno = -1;
  shell_pgrp = getpgid (0);

  if (shell_pgrp == -1)
    {
      sys_error (_("initialize_job_control: getpgrp failed"));
      exit (1);
    }

  /* We can only have job control if we are interactive. */
  if (interactive == 0)
    {
      job_control = 0;
      original_pgrp = NO_PID;
      shell_tty = fileno (stderr);
    }
  else
    {
      shell_tty = -1;

      /* If forced_interactive is set, we skip the normal check that stderr
	 is attached to a tty, so we need to check here.  If it's not, we
	 need to see whether we have a controlling tty by opening /dev/tty,
	 since trying to use job control tty pgrp manipulations on a non-tty
	 is going to fail. */
      if (forced_interactive && isatty (fileno (stderr)) == 0)
	shell_tty = open ("/dev/tty", O_RDWR|O_NONBLOCK);

      /* Get our controlling terminal.  If job_control is set, or
	 interactive is set, then this is an interactive shell no
	 matter where fd 2 is directed. */
      if (shell_tty == -1)
	shell_tty = dup (fileno (stderr));	/* fd 2 */

      shell_tty = move_to_high_fd (shell_tty, 1, -1);

      /* Compensate for a bug in systems that compiled the BSD
	 rlogind with DEBUG defined, like NeXT and Alliant. */
      if (shell_pgrp == 0)
	{
	  shell_pgrp = getpid ();
	  setpgid (0, shell_pgrp);
	  tcsetpgrp (shell_tty, shell_pgrp);
	}

      while ((terminal_pgrp = tcgetpgrp (shell_tty)) != -1)
	{
	  if (shell_pgrp != terminal_pgrp)
	    {
	      SigHandler *ottin;

	      ottin = set_signal_handler(SIGTTIN, SIG_DFL);
	      kill (0, SIGTTIN);
	      set_signal_handler (SIGTTIN, ottin);
	      continue;
	    }
	  break;
	}

      if (terminal_pgrp == -1)
	t_errno = errno;

      /* Make sure that we are using the new line discipline. */
      if (set_new_line_discipline (shell_tty) < 0)
	{
	  sys_error (_("initialize_job_control: line discipline"));
	  job_control = 0;
	}
      else
	{
	  original_pgrp = shell_pgrp;
	  shell_pgrp = getpid ();

	  if ((original_pgrp != shell_pgrp) && (setpgid (0, shell_pgrp) < 0))
	    {
	      sys_error (_("initialize_job_control: setpgid"));
	      shell_pgrp = original_pgrp;
	    }

	  job_control = 1;

	  /* If (and only if) we just set our process group to our pid,
	     thereby becoming a process group leader, and the terminal
	     is not in the same process group as our (new) process group,
	     then set the terminal's process group to our (new) process
	     group.  If that fails, set our process group back to what it
	     was originally (so we can still read from the terminal) and
	     turn off job control.  */
	  if (shell_pgrp != original_pgrp && shell_pgrp != terminal_pgrp)
	    {
	      if (give_terminal_to (shell_pgrp, 0) < 0)
		{
		  t_errno = errno;
		  setpgid (0, original_pgrp);
		  shell_pgrp = original_pgrp;
		  job_control = 0;
		}
	    }

	  if (job_control && ((t = tcgetpgrp (shell_tty)) == -1 || t != shell_pgrp))
	    {
	      if (t_errno != -1)
		errno = t_errno;
	      sys_error (_("cannot set terminal process group (%d)"), t);
	      job_control = 0;
	    }
	}
      if (job_control == 0)
	internal_error (_("no job control in this shell"));
    }

  if (shell_tty != fileno (stderr))
    SET_CLOSE_ON_EXEC (shell_tty);

  set_signal_handler (SIGCHLD, sigchld_handler);

  change_flag ('m', job_control ? '-' : '+');

  if (interactive)
    get_tty_state ();

  if (js.c_childmax < 0)
    js.c_childmax = getmaxchild ();
  if (js.c_childmax < 0)
    js.c_childmax = DEFAULT_CHILD_MAX;

  return job_control;
}

#ifdef DEBUG
void
debug_print_pgrps ()
{
  itrace("original_pgrp = %ld shell_pgrp = %ld terminal_pgrp = %ld",
	 (long)original_pgrp, (long)shell_pgrp, (long)terminal_pgrp);
  itrace("tcgetpgrp(%d) -> %ld, getpgid(0) -> %ld",
	 shell_tty, (long)tcgetpgrp (shell_tty), (long)getpgid(0));
}
#endif

/* Set the line discipline to the best this system has to offer.
   Return -1 if this is not possible. */
static int
set_new_line_discipline (tty)
     int tty;
{
#if defined (NEW_TTY_DRIVER)
  int ldisc;

  if (ioctl (tty, TIOCGETD, &ldisc) < 0)
    return (-1);

  if (ldisc != NTTYDISC)
    {
      ldisc = NTTYDISC;

      if (ioctl (tty, TIOCSETD, &ldisc) < 0)
	return (-1);
    }
  return (0);
#endif /* NEW_TTY_DRIVER */

#if defined (TERMIO_TTY_DRIVER)
#  if defined (TERMIO_LDISC) && (NTTYDISC)
  if (ioctl (tty, TCGETA, &shell_tty_info) < 0)
    return (-1);

  if (shell_tty_info.c_line != NTTYDISC)
    {
      shell_tty_info.c_line = NTTYDISC;
      if (ioctl (tty, TCSETAW, &shell_tty_info) < 0)
	return (-1);
    }
#  endif /* TERMIO_LDISC && NTTYDISC */
  return (0);
#endif /* TERMIO_TTY_DRIVER */

#if defined (TERMIOS_TTY_DRIVER)
#  if defined (TERMIOS_LDISC) && defined (NTTYDISC)
  if (tcgetattr (tty, &shell_tty_info) < 0)
    return (-1);

  if (shell_tty_info.c_line != NTTYDISC)
    {
      shell_tty_info.c_line = NTTYDISC;
      if (tcsetattr (tty, TCSADRAIN, &shell_tty_info) < 0)
	return (-1);
    }
#  endif /* TERMIOS_LDISC && NTTYDISC */
  return (0);
#endif /* TERMIOS_TTY_DRIVER */

#if !defined (NEW_TTY_DRIVER) && !defined (TERMIO_TTY_DRIVER) && !defined (TERMIOS_TTY_DRIVER)
  return (-1);
#endif
}

/* Setup this shell to handle C-C, etc. */
void
initialize_job_signals ()
{
  if (interactive)
    {
      set_signal_handler (SIGINT, sigint_sighandler);
      set_signal_handler (SIGTSTP, SIG_IGN);
      set_signal_handler (SIGTTOU, SIG_IGN);
      set_signal_handler (SIGTTIN, SIG_IGN);
    }
  else if (job_control)
    {
      old_tstp = set_signal_handler (SIGTSTP, sigstop_sighandler);
      old_ttin = set_signal_handler (SIGTTIN, sigstop_sighandler);
      old_ttou = set_signal_handler (SIGTTOU, sigstop_sighandler);
    }
  /* Leave these things alone for non-interactive shells without job
     control. */
}

/* Here we handle CONT signals. */
static sighandler
sigcont_sighandler (sig)
     int sig;
{
  initialize_job_signals ();
  set_signal_handler (SIGCONT, old_cont);
  kill (getpid (), SIGCONT);

  SIGRETURN (0);
}

/* Here we handle stop signals while we are running not as a login shell. */
static sighandler
sigstop_sighandler (sig)
     int sig;
{
  set_signal_handler (SIGTSTP, old_tstp);
  set_signal_handler (SIGTTOU, old_ttou);
  set_signal_handler (SIGTTIN, old_ttin);

  old_cont = set_signal_handler (SIGCONT, sigcont_sighandler);

  give_terminal_to (shell_pgrp, 0);

  kill (getpid (), sig);

  SIGRETURN (0);
}

/* Give the terminal to PGRP.  */
int
give_terminal_to (pgrp, force)
     pid_t pgrp;
     int force;
{
  sigset_t set, oset;
  int r, e;

  r = 0;
  if (job_control || force)
    {
      sigemptyset (&set);
      sigaddset (&set, SIGTTOU);
      sigaddset (&set, SIGTTIN);
      sigaddset (&set, SIGTSTP);
      sigaddset (&set, SIGCHLD);
      sigemptyset (&oset);
      sigprocmask (SIG_BLOCK, &set, &oset);

      if (tcsetpgrp (shell_tty, pgrp) < 0)
	{
	  /* Maybe we should print an error message? */
#if 0
	  sys_error ("tcsetpgrp(%d) failed: pid %ld to pgrp %ld",
	    shell_tty, (long)getpid(), (long)pgrp);
#endif
	  r = -1;
	  e = errno;
	}
      else
	terminal_pgrp = pgrp;
      sigprocmask (SIG_SETMASK, &oset, (sigset_t *)NULL);
    }

  if (r == -1)
    errno = e;

  return r;
}

/* Give terminal to NPGRP iff it's currently owned by OPGRP.  FLAGS are the
   flags to pass to give_terminal_to(). */
static int
maybe_give_terminal_to (opgrp, npgrp, flags)
     pid_t opgrp, npgrp;
     int flags;
{
  int tpgrp;

  tpgrp = tcgetpgrp (shell_tty);
  if (tpgrp < 0 && errno == ENOTTY)
    return -1;
  if (tpgrp == npgrp)
    {
      terminal_pgrp = npgrp;
      return 0;
    }
  else if (tpgrp != opgrp)
    {
#if defined (DEBUG)
      internal_warning ("maybe_give_terminal_to: terminal pgrp == %d shell pgrp = %d new pgrp = %d", tpgrp, opgrp, npgrp);
#endif
      return -1;
    }
  else
    return (give_terminal_to (npgrp, flags));     
}

/* Clear out any jobs in the job array.  This is intended to be used by
   children of the shell, who should not have any job structures as baggage
   when they start executing (forking subshells for parenthesized execution
   and functions with pipes are the two that spring to mind).  If RUNNING_ONLY
   is nonzero, only running jobs are removed from the table. */
void
delete_all_jobs (running_only)
     int running_only;
{
  register int i;
  sigset_t set, oset;

  BLOCK_CHILD (set, oset);

  /* XXX - need to set j_lastj, j_firstj appropriately if running_only != 0. */
  if (js.j_jobslots)
    {
      js.j_current = js.j_previous = NO_JOB;

      /* XXX could use js.j_firstj here */
      for (i = 0; i < js.j_jobslots; i++)
	{
#if defined (DEBUG)
	  if (i < js.j_firstj && jobs[i])
	    itrace("delete_all_jobs: job %d non-null before js.j_firstj (%d)", i, js.j_firstj);
	  if (i > js.j_lastj && jobs[i])
	    itrace("delete_all_jobs: job %d non-null after js.j_lastj (%d)", i, js.j_lastj);
#endif
	  if (jobs[i] && (running_only == 0 || (running_only && RUNNING(i))))
	    delete_job (i, DEL_WARNSTOPPED);
	}
      if (running_only == 0)
	{
	  free ((char *)jobs);
	  js.j_jobslots = 0;
	  js.j_firstj = js.j_lastj = js.j_njobs = 0;
	}
    }

  if (running_only == 0)
    bgp_clear ();

  UNBLOCK_CHILD (oset);
}

/* Mark all jobs in the job array so that they don't get a SIGHUP when the
   shell gets one.  If RUNNING_ONLY is nonzero, mark only running jobs. */
void
nohup_all_jobs (running_only)
     int running_only;
{
  register int i;
  sigset_t set, oset;

  BLOCK_CHILD (set, oset);

  if (js.j_jobslots)
    {
      /* XXX could use js.j_firstj here */
      for (i = 0; i < js.j_jobslots; i++)
	if (jobs[i] && (running_only == 0 || (running_only && RUNNING(i))))
	  nohup_job (i);
    }

  UNBLOCK_CHILD (oset);
}

int
count_all_jobs ()
{
  int i, n;
  sigset_t set, oset;

  /* This really counts all non-dead jobs. */
  BLOCK_CHILD (set, oset);
  /* XXX could use js.j_firstj here */
  for (i = n = 0; i < js.j_jobslots; i++)
    {
#if defined (DEBUG)
      if (i < js.j_firstj && jobs[i])
	itrace("count_all_jobs: job %d non-null before js.j_firstj (%d)", i, js.j_firstj);
      if (i > js.j_lastj && jobs[i])
	itrace("count_all_jobs: job %d non-null after js.j_lastj (%d)", i, js.j_lastj);
#endif
      if (jobs[i] && DEADJOB(i) == 0)
	n++;
    }
  UNBLOCK_CHILD (oset);
  return n;
}

static void
mark_all_jobs_as_dead ()
{
  register int i;
  sigset_t set, oset;

  if (js.j_jobslots == 0)
    return;

  BLOCK_CHILD (set, oset);

  /* XXX could use js.j_firstj here */
  for (i = 0; i < js.j_jobslots; i++)
    if (jobs[i])
      {
	jobs[i]->state = JDEAD;
	js.j_ndead++;
      }

  UNBLOCK_CHILD (oset);
}

/* Mark all dead jobs as notified, so delete_job () cleans them out
   of the job table properly.  POSIX.2 says we need to save the
   status of the last CHILD_MAX jobs, so we count the number of dead
   jobs and mark only enough as notified to save CHILD_MAX statuses. */
static void
mark_dead_jobs_as_notified (force)
     int force;
{
  register int i, ndead, ndeadproc;
  sigset_t set, oset;

  if (js.j_jobslots == 0)
    return;

  BLOCK_CHILD (set, oset);

  /* If FORCE is non-zero, we don't have to keep CHILD_MAX statuses
     around; just run through the array. */
  if (force)
    {
    /* XXX could use js.j_firstj here */
      for (i = 0; i < js.j_jobslots; i++)
	{
	  if (jobs[i] && DEADJOB (i) && (interactive_shell || (find_last_pid (i, 0) != last_asynchronous_pid)))
	    jobs[i]->flags |= J_NOTIFIED;
	}
      UNBLOCK_CHILD (oset);
      return;
    }

  /* Mark enough dead jobs as notified to keep CHILD_MAX processes left in the
     array with the corresponding not marked as notified.  This is a better
     way to avoid pid aliasing and reuse problems than keeping the POSIX-
     mandated CHILD_MAX jobs around.  delete_job() takes care of keeping the
     bgpids list regulated. */
          
  /* Count the number of dead jobs */
  /* XXX could use js.j_firstj here */
  for (i = ndead = ndeadproc = 0; i < js.j_jobslots; i++)
    {
#if defined (DEBUG)
      if (i < js.j_firstj && jobs[i])
	itrace("mark_dead_jobs_as_notified: job %d non-null before js.j_firstj (%d)", i, js.j_firstj);
      if (i > js.j_lastj && jobs[i])
	itrace("mark_dead_jobs_as_notified: job %d non-null after js.j_lastj (%d)", i, js.j_lastj);
#endif
      if (jobs[i] && DEADJOB (i))
	{
	  ndead++;
	  ndeadproc += processes_in_job (i);
	}
    }

#ifdef DEBUG
  if (ndeadproc != js.c_reaped)
    itrace("mark_dead_jobs_as_notified: ndeadproc (%d) != js.c_reaped (%d)", ndeadproc, js.c_reaped);
  if (ndead != js.j_ndead)
    itrace("mark_dead_jobs_as_notified: ndead (%d) != js.j_ndead (%d)", ndead, js.j_ndead);
#endif

  if (js.c_childmax < 0)
    js.c_childmax = getmaxchild ();
  if (js.c_childmax < 0)
    js.c_childmax = DEFAULT_CHILD_MAX;

  /* Don't do anything if the number of dead processes is less than CHILD_MAX
     and we're not forcing a cleanup. */
  if (ndeadproc <= js.c_childmax)
    {
      UNBLOCK_CHILD (oset);
      return;
    }

#if 0
itrace("mark_dead_jobs_as_notified: child_max = %d ndead = %d ndeadproc = %d", js.c_childmax, ndead, ndeadproc);
#endif

  /* Mark enough dead jobs as notified that we keep CHILD_MAX jobs in
     the list.  This isn't exactly right yet; changes need to be made
     to stop_pipeline so we don't mark the newer jobs after we've
     created CHILD_MAX slots in the jobs array.  This needs to be
     integrated with a way to keep the jobs array from growing without
     bound.  Maybe we wrap back around to 0 after we reach some max
     limit, and there are sufficient job slots free (keep track of total
     size of jobs array (js.j_jobslots) and running count of number of jobs
     in jobs array.  Then keep a job index corresponding to the `oldest job'
     and start this loop there, wrapping around as necessary.  In effect,
     we turn the list into a circular buffer. */
  /* XXX could use js.j_firstj here */
  for (i = 0; i < js.j_jobslots; i++)
    {
      if (jobs[i] && DEADJOB (i) && (interactive_shell || (find_last_pid (i, 0) != last_asynchronous_pid)))
	{
#if defined (DEBUG)
	  if (i < js.j_firstj && jobs[i])
	    itrace("mark_dead_jobs_as_notified: job %d non-null before js.j_firstj (%d)", i, js.j_firstj);
	  if (i > js.j_lastj && jobs[i])
	    itrace("mark_dead_jobs_as_notified: job %d non-null after js.j_lastj (%d)", i, js.j_lastj);
#endif
	  /* If marking this job as notified would drop us down below
	     child_max, don't mark it so we can keep at least child_max
	     statuses.  XXX -- need to check what Posix actually says
	     about keeping statuses. */
	  if ((ndeadproc -= processes_in_job (i)) <= js.c_childmax)
	    break;
	  jobs[i]->flags |= J_NOTIFIED;
	}
    }

  UNBLOCK_CHILD (oset);
}

/* Here to allow other parts of the shell (like the trap stuff) to
   unfreeze the jobs list. */
void
unfreeze_jobs_list ()
{
  jobs_list_frozen = 0;
}

/* Allow or disallow job control to take place.  Returns the old value
   of job_control. */
int
set_job_control (arg)
     int arg;
{
  int old;

  old = job_control;
  job_control = arg;

  /* If we're turning on job control, reset pipeline_pgrp so make_child will
     put new child processes into the right pgrp */
  if (job_control != old && job_control)
    pipeline_pgrp = 0;

  return (old);
}

/* Turn off all traces of job control.  This is run by children of the shell
   which are going to do shellsy things, like wait (), etc. */
void
without_job_control ()
{
  stop_making_children ();
  start_pipeline ();
#if defined (PGRP_PIPE)
  sh_closepipe (pgrp_pipe);
#endif
  delete_all_jobs (0);
  set_job_control (0);
}

/* If this shell is interactive, terminate all stopped jobs and
   restore the original terminal process group.  This is done
   before the `exec' builtin calls shell_execve. */
void
end_job_control ()
{
  if (interactive_shell)		/* XXX - should it be interactive? */
    {
      terminate_stopped_jobs ();

      if (original_pgrp >= 0)
	give_terminal_to (original_pgrp, 1);
    }

  if (original_pgrp >= 0)
    setpgid (0, original_pgrp);
}

/* Restart job control by closing shell tty and reinitializing.  This is
   called after an exec fails in an interactive shell and we do not exit. */
void
restart_job_control ()
{
  if (shell_tty != -1)
    close (shell_tty);
  initialize_job_control (0);
}

/* Set the handler to run when the shell receives a SIGCHLD signal. */
void
set_sigchld_handler ()
{
  set_signal_handler (SIGCHLD, sigchld_handler);
}

#if defined (PGRP_PIPE)
/* Read from the read end of a pipe.  This is how the process group leader
   blocks until all of the processes in a pipeline have been made. */
static void
pipe_read (pp)
     int *pp;
{
  char ch;

  if (pp[1] >= 0)
    {
      close (pp[1]);
      pp[1] = -1;
    }

  if (pp[0] >= 0)
    {
      while (read (pp[0], &ch, 1) == -1 && errno == EINTR)
	;
    }
}

/* Functional interface closes our local-to-job-control pipes. */
void
close_pgrp_pipe ()
{
  sh_closepipe (pgrp_pipe);
}

void
save_pgrp_pipe (p, clear)
     int *p;
     int clear;
{
  p[0] = pgrp_pipe[0];
  p[1] = pgrp_pipe[1];
  if (clear)
    pgrp_pipe[0] = pgrp_pipe[1] = -1;
}

void
restore_pgrp_pipe (p)
     int *p;
{
  pgrp_pipe[0] = p[0];
  pgrp_pipe[1] = p[1];
}

#endif /* PGRP_PIPE */
