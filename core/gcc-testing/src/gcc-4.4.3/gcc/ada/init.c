/****************************************************************************
 *                                                                          *
 *                         GNAT COMPILER COMPONENTS                         *
 *                                                                          *
 *                                 I N I T                                  *
 *                                                                          *
 *                          C Implementation File                           *
 *                                                                          *
 *          Copyright (C) 1992-2009, Free Software Foundation, Inc.         *
 *                                                                          *
 * GNAT is free software;  you can  redistribute it  and/or modify it under *
 * terms of the  GNU General Public License as published  by the Free Soft- *
 * ware  Foundation;  either version 3,  or (at your option) any later ver- *
 * sion.  GNAT is distributed in the hope that it will be useful, but WITH- *
 * OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.                                     *
 *                                                                          *
 * As a special exception under Section 7 of GPL version 3, you are granted *
 * additional permissions described in the GCC Runtime Library Exception,   *
 * version 3.1, as published by the Free Software Foundation.               *
 *                                                                          *
 * You should have received a copy of the GNU General Public License and    *
 * a copy of the GCC Runtime Library Exception along with this program;     *
 * see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see    *
 * <http://www.gnu.org/licenses/>.                                          *
 *                                                                          *
 * GNAT was originally developed  by the GNAT team at  New York University. *
 * Extensive contributions were provided by Ada Core Technologies Inc.      *
 *                                                                          *
 ****************************************************************************/

/*  This unit contains initialization circuits that are system dependent.
    A major part of the functionality involves stack overflow checking.
    The GCC backend generates probe instructions to test for stack overflow.
    For details on the exact approach used to generate these probes, see the
    "Using and Porting GCC" manual, in particular the "Stack Checking" section
    and the subsection "Specifying How Stack Checking is Done".  The handlers
    installed by this file are used to catch the resulting signals that come
    from these probes failing (i.e. touching protected pages).  */

/* This file should be kept synchronized with 2sinit.ads, 2sinit.adb,
   s-init-ae653-cert.adb and s-init-xi-sparc.adb.  All these files implement
   the required functionality for different targets.  */

/* The following include is here to meet the published VxWorks requirement
   that the __vxworks header appear before any other include.  */
#ifdef __vxworks
#include "vxWorks.h"
#endif

#ifdef IN_RTS
#include "tconfig.h"
#include "tsystem.h"
#include <sys/stat.h>

/* We don't have libiberty, so us malloc.  */
#define xmalloc(S) malloc (S)
#else
#include "config.h"
#include "system.h"
#endif

#include "adaint.h"
#include "raise.h"

extern void __gnat_raise_program_error (const char *, int);

/* Addresses of exception data blocks for predefined exceptions.  Tasking_Error
   is not used in this unit, and the abort signal is only used on IRIX.  */
extern struct Exception_Data constraint_error;
extern struct Exception_Data numeric_error;
extern struct Exception_Data program_error;
extern struct Exception_Data storage_error;

/* For the Cert run time we use the regular raise exception routine because
   Raise_From_Signal_Handler is not available.  */
#ifdef CERT
#define Raise_From_Signal_Handler \
                      __gnat_raise_exception
extern void Raise_From_Signal_Handler (struct Exception_Data *, const char *);
#else
#define Raise_From_Signal_Handler \
                      ada__exceptions__raise_from_signal_handler
extern void Raise_From_Signal_Handler (struct Exception_Data *, const char *);
#endif

/* Global values computed by the binder.  */
int   __gl_main_priority                 = -1;
int   __gl_time_slice_val                = -1;
char  __gl_wc_encoding                   = 'n';
char  __gl_locking_policy                = ' ';
char  __gl_queuing_policy                = ' ';
char  __gl_task_dispatching_policy       = ' ';
char *__gl_priority_specific_dispatching = 0;
int   __gl_num_specific_dispatching      = 0;
char *__gl_interrupt_states              = 0;
int   __gl_num_interrupt_states          = 0;
int   __gl_unreserve_all_interrupts      = 0;
int   __gl_exception_tracebacks          = 0;
int   __gl_zero_cost_exceptions          = 0;
int   __gl_detect_blocking               = 0;
int   __gl_default_stack_size            = -1;
int   __gl_leap_seconds_support          = 0;
int   __gl_canonical_streams             = 0;

/* Indication of whether synchronous signal handler has already been
   installed by a previous call to adainit.  */
int  __gnat_handler_installed      = 0;

#ifndef IN_RTS
int __gnat_inside_elab_final_code = 0;
/* ??? This variable is obsolete since 2001-08-29 but is kept to allow
   bootstrap from old GNAT versions (< 3.15).  */
#endif

/* HAVE_GNAT_INIT_FLOAT must be set on every targets where a __gnat_init_float
   is defined.  If this is not set then a void implementation will be defined
   at the end of this unit.  */
#undef HAVE_GNAT_INIT_FLOAT

/******************************/
/* __gnat_get_interrupt_state */
/******************************/

char __gnat_get_interrupt_state (int);

/* This routine is called from the runtime as needed to determine the state
   of an interrupt, as set by an Interrupt_State pragma appearing anywhere
   in the current partition.  The input argument is the interrupt number,
   and the result is one of the following:

       'n'   this interrupt not set by any Interrupt_State pragma
       'u'   Interrupt_State pragma set state to User
       'r'   Interrupt_State pragma set state to Runtime
       's'   Interrupt_State pragma set state to System  */

char
__gnat_get_interrupt_state (int intrup)
{
  if (intrup >= __gl_num_interrupt_states)
    return 'n';
  else
    return __gl_interrupt_states [intrup];
}

/***********************************/
/* __gnat_get_specific_dispatching */
/***********************************/

char __gnat_get_specific_dispatching (int);

/* This routine is called from the runtime as needed to determine the
   priority specific dispatching policy, as set by a
   Priority_Specific_Dispatching pragma appearing anywhere in the current
   partition.  The input argument is the priority number, and the result
   is the upper case first character of the policy name, e.g. 'F' for
   FIFO_Within_Priorities. A space ' ' is returned if no
   Priority_Specific_Dispatching pragma is used in the partition.  */

char
__gnat_get_specific_dispatching (int priority)
{
  if (__gl_num_specific_dispatching == 0)
    return ' ';
  else if (priority >= __gl_num_specific_dispatching)
    return 'F';
  else
    return __gl_priority_specific_dispatching [priority];
}

#ifndef IN_RTS

/**********************/
/* __gnat_set_globals */
/**********************/

/* This routine is kept for bootstrapping purposes, since the binder generated
   file now sets the __gl_* variables directly.  */

void
__gnat_set_globals ()
{
}

#endif

/***************/
/* AIX Section */
/***************/

#if defined (_AIX)

#include <signal.h>
#include <sys/time.h>

/* Some versions of AIX don't define SA_NODEFER.  */

#ifndef SA_NODEFER
#define SA_NODEFER 0
#endif /* SA_NODEFER */

/* Versions of AIX before 4.3 don't have nanosleep but provide
   nsleep instead.  */

#ifndef _AIXVERSION_430

extern int nanosleep (struct timestruc_t *, struct timestruc_t *);

int
nanosleep (struct timestruc_t *Rqtp, struct timestruc_t *Rmtp)
{
  return nsleep (Rqtp, Rmtp);
}

#endif /* _AIXVERSION_430 */

static void __gnat_error_handler (int sig, siginfo_t * si, void * uc);

static void
__gnat_error_handler (int sig, siginfo_t * si, void * uc)
{
  struct Exception_Data *exception;
  const char *msg;

  switch (sig)
    {
    case SIGSEGV:
      /* FIXME: we need to detect the case of a *real* SIGSEGV.  */
      exception = &storage_error;
      msg = "stack overflow or erroneous memory access";
      break;

    case SIGBUS:
      exception = &constraint_error;
      msg = "SIGBUS";
      break;

    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;

    default:
      exception = &program_error;
      msg = "unhandled signal";
    }

  Raise_From_Signal_Handler (exception, msg);
}

void
__gnat_install_handler (void)
{
  struct sigaction act;

  /* Set up signal handler to map synchronous signals to appropriate
     exceptions.  Make sure that the handler isn't interrupted by another
     signal that might cause a scheduling event!  */

  act.sa_flags = SA_NODEFER | SA_RESTART | SA_SIGINFO;
  act.sa_sigaction = __gnat_error_handler;
  sigemptyset (&act.sa_mask);

  /* Do not install handlers if interrupt state is "System".  */
  if (__gnat_get_interrupt_state (SIGABRT) != 's')
    sigaction (SIGABRT, &act, NULL);
  if (__gnat_get_interrupt_state (SIGFPE) != 's')
    sigaction (SIGFPE,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGILL) != 's')
    sigaction (SIGILL,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGSEGV) != 's')
    sigaction (SIGSEGV, &act, NULL);
  if (__gnat_get_interrupt_state (SIGBUS) != 's')
    sigaction (SIGBUS,  &act, NULL);

  __gnat_handler_installed = 1;
}

/*****************/
/* Tru64 section */
/*****************/

#elif defined(__alpha__) && defined(__osf__)

#include <signal.h>
#include <sys/siginfo.h>

static void __gnat_error_handler (int, siginfo_t *, struct sigcontext *);
extern char *__gnat_get_code_loc (struct sigcontext *);
extern void __gnat_set_code_loc (struct sigcontext *, char *);
extern size_t __gnat_machine_state_length (void);

#define HAVE_GNAT_ADJUST_CONTEXT_FOR_RAISE

void
__gnat_adjust_context_for_raise (int signo, void *ucontext)
{
  struct sigcontext *sigcontext = (struct sigcontext *) ucontext;

  /* The unwinder expects the signal context to contain the address of the
     faulting instruction.  For SIGFPE, this depends on the trap shadow
     situation (see man ieee).  We nonetheless always compensate for it,
     considering that PC designates the instruction following the one that
     trapped.  This is not necessarily true but corresponds to what we have
     always observed.  */
  if (signo == SIGFPE)
    sigcontext->sc_pc--;
}

static void
__gnat_error_handler
  (int sig, siginfo_t *sip, struct sigcontext *context)
{
  struct Exception_Data *exception;
  static int recurse = 0;
  const char *msg;

  /* Adjusting is required for every fault context, so adjust for this one
     now, before we possibly trigger a recursive fault below.  */
  __gnat_adjust_context_for_raise (sig, context);

  /* If this was an explicit signal from a "kill", just resignal it.  */
  if (SI_FROMUSER (sip))
    {
      signal (sig, SIG_DFL);
      kill (getpid(), sig);
    }

  /* Otherwise, treat it as something we handle.  */
  switch (sig)
    {
    case SIGSEGV:
      /* If the problem was permissions, this is a constraint error.
	 Likewise if the failing address isn't maximally aligned or if
	 we've recursed.

	 ??? Using a static variable here isn't task-safe, but it's
	 much too hard to do anything else and we're just determining
	 which exception to raise.  */
      if (sip->si_code == SEGV_ACCERR
	  || (((long) sip->si_addr) & 3) != 0
	  || recurse)
	{
	  exception = &constraint_error;
	  msg = "SIGSEGV";
	}
      else
	{
	  /* See if the page before the faulting page is accessible.  Do that
	     by trying to access it.  We'd like to simply try to access
	     4096 + the faulting address, but it's not guaranteed to be
	     the actual address, just to be on the same page.  */
	  recurse++;
	  ((volatile char *)
	   ((long) sip->si_addr & - getpagesize ()))[getpagesize ()];
	  msg = "stack overflow (or erroneous memory access)";
	  exception = &storage_error;
	}
      break;

    case SIGBUS:
      exception = &program_error;
      msg = "SIGBUS";
      break;

    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;

    default:
      exception = &program_error;
      msg = "unhandled signal";
    }

  recurse = 0;
  Raise_From_Signal_Handler (exception, (char *) msg);
}

void
__gnat_install_handler (void)
{
  struct sigaction act;

  /* Setup signal handler to map synchronous signals to appropriate
     exceptions. Make sure that the handler isn't interrupted by another
     signal that might cause a scheduling event!  */

  act.sa_handler = (void (*) (int)) __gnat_error_handler;
  act.sa_flags = SA_RESTART | SA_NODEFER | SA_SIGINFO;
  sigemptyset (&act.sa_mask);

  /* Do not install handlers if interrupt state is "System".  */
  if (__gnat_get_interrupt_state (SIGABRT) != 's')
    sigaction (SIGABRT, &act, NULL);
  if (__gnat_get_interrupt_state (SIGFPE) != 's')
    sigaction (SIGFPE,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGILL) != 's')
    sigaction (SIGILL,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGSEGV) != 's')
    sigaction (SIGSEGV, &act, NULL);
  if (__gnat_get_interrupt_state (SIGBUS) != 's')
    sigaction (SIGBUS,  &act, NULL);

  __gnat_handler_installed = 1;
}

/* Routines called by s-mastop-tru64.adb.  */

#define SC_GP 29

char *
__gnat_get_code_loc (struct sigcontext *context)
{
  return (char *) context->sc_pc;
}

void
__gnat_set_code_loc (struct sigcontext *context, char *pc)
{
  context->sc_pc = (long) pc;
}

size_t
__gnat_machine_state_length (void)
{
  return sizeof (struct sigcontext);
}

/*****************/
/* HP-UX section */
/*****************/

#elif defined (__hpux__)

#include <signal.h>
#include <sys/ucontext.h>

static void
__gnat_error_handler (int sig, siginfo_t *siginfo, void *ucontext);

static void
__gnat_error_handler
  (int sig, siginfo_t *siginfo ATTRIBUTE_UNUSED, void *ucontext)
{
  struct Exception_Data *exception;
  const char *msg;

  switch (sig)
    {
    case SIGSEGV:
      /* FIXME: we need to detect the case of a *real* SIGSEGV.  */
      exception = &storage_error;
      msg = "stack overflow or erroneous memory access";
      break;

    case SIGBUS:
      exception = &constraint_error;
      msg = "SIGBUS";
      break;

    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;

    default:
      exception = &program_error;
      msg = "unhandled signal";
    }

  Raise_From_Signal_Handler (exception, msg);
}

/* This must be in keeping with System.OS_Interface.Alternate_Stack_Size.  */
#if defined (__hppa__)
char __gnat_alternate_stack[16 * 1024]; /* 2 * SIGSTKSZ */
#else
char __gnat_alternate_stack[128 * 1024]; /* MINSIGSTKSZ */
#endif

void
__gnat_install_handler (void)
{
  struct sigaction act;

  /* Set up signal handler to map synchronous signals to appropriate
     exceptions.  Make sure that the handler isn't interrupted by another
     signal that might cause a scheduling event!  Also setup an alternate
     stack region for the handler execution so that stack overflows can be
     handled properly, avoiding a SEGV generation from stack usage by the
     handler itself.  */

  stack_t stack;
  stack.ss_sp = __gnat_alternate_stack;
  stack.ss_size = sizeof (__gnat_alternate_stack);
  stack.ss_flags = 0;
  sigaltstack (&stack, NULL);

  act.sa_sigaction = __gnat_error_handler;
  act.sa_flags = SA_NODEFER | SA_RESTART | SA_SIGINFO;
  sigemptyset (&act.sa_mask);

  /* Do not install handlers if interrupt state is "System".  */
  if (__gnat_get_interrupt_state (SIGABRT) != 's')
    sigaction (SIGABRT, &act, NULL);
  if (__gnat_get_interrupt_state (SIGFPE) != 's')
    sigaction (SIGFPE,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGILL) != 's')
    sigaction (SIGILL,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGBUS) != 's')
    sigaction (SIGBUS,  &act, NULL);
  act.sa_flags |= SA_ONSTACK;
  if (__gnat_get_interrupt_state (SIGSEGV) != 's')
    sigaction (SIGSEGV, &act, NULL);

  __gnat_handler_installed = 1;
}

/*********************/
/* GNU/Linux Section */
/*********************/

#elif defined (linux) && (defined (i386) || defined (__x86_64__) \
                          || defined (__ia64__) || defined (__powerpc__))

#include <signal.h>

#define __USE_GNU 1 /* required to get REG_EIP/RIP from glibc's ucontext.h */
#include <sys/ucontext.h>

/* GNU/Linux, which uses glibc, does not define NULL in included
   header files.  */

#if !defined (NULL)
#define NULL ((void *) 0)
#endif

#if defined (MaRTE)

/* MaRTE OS provides its own version of sigaction, sigfillset, and
   sigemptyset (overriding these symbol names).  We want to make sure that
   the versions provided by the underlying C library are used here (these
   versions are renamed by MaRTE to linux_sigaction, fake_linux_sigfillset,
   and fake_linux_sigemptyset, respectively).  The MaRTE library will not
   always be present (it will not be linked if no tasking constructs are
   used), so we use the weak symbol mechanism to point always to the symbols
   defined within the C library.  */

#pragma weak linux_sigaction
int linux_sigaction (int signum, const struct sigaction *act,
		     struct sigaction *oldact) {
  return sigaction (signum, act, oldact);
}
#define sigaction(signum, act, oldact) linux_sigaction (signum, act, oldact)

#pragma weak fake_linux_sigfillset
void fake_linux_sigfillset (sigset_t *set) {
  sigfillset (set);
}
#define sigfillset(set) fake_linux_sigfillset (set)

#pragma weak fake_linux_sigemptyset
void fake_linux_sigemptyset (sigset_t *set) {
  sigemptyset (set);
}
#define sigemptyset(set) fake_linux_sigemptyset (set)

#endif

static void __gnat_error_handler (int, siginfo_t *siginfo, void *ucontext);

#if defined (i386) || defined (__x86_64__) || defined (__ia64__)

#define HAVE_GNAT_ADJUST_CONTEXT_FOR_RAISE

void
__gnat_adjust_context_for_raise (int signo ATTRIBUTE_UNUSED, void *ucontext)
{
  mcontext_t *mcontext = &((ucontext_t *) ucontext)->uc_mcontext;

  /* On the i386 and x86-64 architectures, stack checking is performed by
     means of probes with moving stack pointer, that is to say the probed
     address is always the value of the stack pointer.  Upon hitting the
     guard page, the stack pointer therefore points to an inaccessible
     address and an alternate signal stack is needed to run the handler.
     But there is an additional twist: on these architectures, the EH
     return code writes the address of the handler at the target CFA's
     value on the stack before doing the jump.  As a consequence, if
     there is an active handler in the frame whose stack has overflowed,
     the stack pointer must nevertheless point to an accessible address
     by the time the EH return is executed.

     We therefore adjust the saved value of the stack pointer by the size
     of one page, in order to make sure that it points to an accessible
     address in case it's used as the target CFA.  The stack checking code
     guarantees that this page is unused by the time this happens.  */

#if defined (i386)
  unsigned long pattern = *(unsigned long *)mcontext->gregs[REG_EIP];
  /* The pattern is "orl $0x0,(%esp)" for a probe in 32-bit mode.  */
  if (signo == SIGSEGV && pattern == 0x00240c83)
    mcontext->gregs[REG_ESP] += 4096;
#elif defined (__x86_64__)
  unsigned long pattern = *(unsigned long *)mcontext->gregs[REG_RIP];
  /* The pattern is "orq $0x0,(%rsp)" for a probe in 64-bit mode.  */
  if (signo == SIGSEGV && (pattern & 0xffffffffff) == 0x00240c8348)
    mcontext->gregs[REG_RSP] += 4096;
#elif defined (__ia64__)
  /* ??? The IA-64 unwinder doesn't compensate for signals.  */
  mcontext->sc_ip++;
#endif
}

#endif

static void
__gnat_error_handler (int sig,
                      siginfo_t *siginfo ATTRIBUTE_UNUSED,
                      void *ucontext)
{
  struct Exception_Data *exception;
  const char *msg;
  static int recurse = 0;

  switch (sig)
    {
    case SIGSEGV:
      /* If the problem was permissions, this is a constraint error.
       Likewise if the failing address isn't maximally aligned or if
       we've recursed.

       ??? Using a static variable here isn't task-safe, but it's
       much too hard to do anything else and we're just determining
       which exception to raise.  */
      if (recurse)
      {
        exception = &constraint_error;
        msg = "SIGSEGV";
      }
      else
      {
        /* Here we would like a discrimination test to see whether the
           page before the faulting address is accessible. Unfortunately
           Linux seems to have no way of giving us the faulting address.

           In versions of a-init.c before 1.95, we had a test of the page
           before the stack pointer using:

            recurse++;
             ((volatile char *)
              ((long) info->esp_at_signal & - getpagesize ()))[getpagesize ()];

           but that's wrong, since it tests the stack pointer location, and
           the current stack probe code does not move the stack pointer
           until all probes succeed.

           For now we simply do not attempt any discrimination at all. Note
           that this is quite acceptable, since a "real" SIGSEGV can only
           occur as the result of an erroneous program.  */

        msg = "stack overflow (or erroneous memory access)";
        exception = &storage_error;
      }
      break;

    case SIGBUS:
      exception = &constraint_error;
      msg = "SIGBUS";
      break;

    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;

    default:
      exception = &program_error;
      msg = "unhandled signal";
    }
  recurse = 0;

  /* We adjust the interrupted context here (and not in the fallback
     unwinding routine) because recent versions of the Native POSIX
     Thread Library (NPTL) are compiled with unwind information, so
     the fallback routine is never executed for signal frames.  */
  __gnat_adjust_context_for_raise (sig, ucontext);

  Raise_From_Signal_Handler (exception, msg);
}

#if defined (i386) || defined (__x86_64__)
/* This must be in keeping with System.OS_Interface.Alternate_Stack_Size.  */
char __gnat_alternate_stack[16 * 1024]; /* 2 * SIGSTKSZ */
#endif

#ifdef __XENO__
#include <sys/mman.h>
#include <native/task.h>

RT_TASK main_task;
#endif

void
__gnat_install_handler (void)
{
  struct sigaction act;

#ifdef __XENO__
  int prio;

  if (__gl_main_priority == -1)
    prio = 49;
  else
    prio = __gl_main_priority;

  /* Avoid memory swapping for this program */

  mlockall (MCL_CURRENT|MCL_FUTURE);

  /* Turn the current Linux task into a native Xenomai task */

  rt_task_shadow(&main_task, "environment_task", prio, T_FPU);
#endif

  /* Set up signal handler to map synchronous signals to appropriate
     exceptions.  Make sure that the handler isn't interrupted by another
     signal that might cause a scheduling event!  Also setup an alternate
     stack region for the handler execution so that stack overflows can be
     handled properly, avoiding a SEGV generation from stack usage by the
     handler itself.  */

#if defined (i386) || defined (__x86_64__)
  stack_t stack;
  stack.ss_sp = __gnat_alternate_stack;
  stack.ss_size = sizeof (__gnat_alternate_stack);
  stack.ss_flags = 0;
  sigaltstack (&stack, NULL);
#endif

  act.sa_sigaction = __gnat_error_handler;
  act.sa_flags = SA_NODEFER | SA_RESTART | SA_SIGINFO;
  sigemptyset (&act.sa_mask);

  /* Do not install handlers if interrupt state is "System".  */
  if (__gnat_get_interrupt_state (SIGABRT) != 's')
    sigaction (SIGABRT, &act, NULL);
  if (__gnat_get_interrupt_state (SIGFPE) != 's')
    sigaction (SIGFPE,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGILL) != 's')
    sigaction (SIGILL,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGBUS) != 's')
    sigaction (SIGBUS,  &act, NULL);
#if defined (i386) || defined (__x86_64__)
  act.sa_flags |= SA_ONSTACK;
#endif
  if (__gnat_get_interrupt_state (SIGSEGV) != 's')
    sigaction (SIGSEGV, &act, NULL);

  __gnat_handler_installed = 1;
}

/****************/
/* IRIX Section */
/****************/

#elif defined (sgi)

#include <signal.h>
#include <siginfo.h>

#ifndef NULL
#define NULL 0
#endif

#define SIGADAABORT 48
#define SIGNAL_STACK_SIZE 4096
#define SIGNAL_STACK_ALIGNMENT 64

#define Check_Abort_Status     \
                      system__soft_links__check_abort_status
extern int (*Check_Abort_Status) (void);

extern struct Exception_Data _abort_signal;

static void __gnat_error_handler (int, int, sigcontext_t *);

/* We are not setting the SA_SIGINFO bit in the sigaction flags when
   connecting that handler, with the effects described in the sigaction
   man page:

          SA_SIGINFO [...]
          If cleared and the signal is caught, the first argument is
          also the signal number but the second argument is the signal
          code identifying the cause of the signal. The third argument
          points to a sigcontext_t structure containing the receiving
          process's context when the signal was delivered.  */

static void
__gnat_error_handler (int sig, int code, sigcontext_t *sc ATTRIBUTE_UNUSED)
{
  struct Exception_Data *exception;
  const char *msg;

  switch (sig)
    {
    case SIGSEGV:
      if (code == EFAULT)
	{
	  exception = &program_error;
	  msg = "SIGSEGV: (Invalid virtual address)";
	}
      else if (code == ENXIO)
	{
	  exception = &program_error;
	  msg = "SIGSEGV: (Read beyond mapped object)";
	}
      else if (code == ENOSPC)
	{
	  exception = &program_error; /* ??? storage_error ??? */
	  msg = "SIGSEGV: (Autogrow for file failed)";
	}
      else if (code == EACCES || code == EEXIST)
	{
	  /* ??? We handle stack overflows here, some of which do trigger
	         SIGSEGV + EEXIST on Irix 6.5 although EEXIST is not part of
	         the documented valid codes for SEGV in the signal(5) man
	         page.  */

	  /* ??? Re-add smarts to further verify that we launched
		 the stack into a guard page, not an attempt to
		 write to .text or something.  */
	  exception = &storage_error;
	  msg = "SIGSEGV: (stack overflow or erroneous memory access)";
	}
      else
	{
	  /* Just in case the OS guys did it to us again.  Sometimes
	     they fail to document all of the valid codes that are
	     passed to signal handlers, just in case someone depends
	     on knowing all the codes.  */
	  exception = &program_error;
	  msg = "SIGSEGV: (Undocumented reason)";
	}
      break;

    case SIGBUS:
      /* Map all bus errors to Program_Error.  */
      exception = &program_error;
      msg = "SIGBUS";
      break;

    case SIGFPE:
      /* Map all fpe errors to Constraint_Error.  */
      exception = &constraint_error;
      msg = "SIGFPE";
      break;

    case SIGADAABORT:
      if ((*Check_Abort_Status) ())
	{
	  exception = &_abort_signal;
	  msg = "";
	}
      else
	return;

      break;

    default:
      /* Everything else is a Program_Error.  */
      exception = &program_error;
      msg = "unhandled signal";
    }

  Raise_From_Signal_Handler (exception, msg);
}

void
__gnat_install_handler (void)
{
  struct sigaction act;

  /* Setup signal handler to map synchronous signals to appropriate
     exceptions.  Make sure that the handler isn't interrupted by another
     signal that might cause a scheduling event!  */

  act.sa_handler = __gnat_error_handler;
  act.sa_flags = SA_NODEFER + SA_RESTART;
  sigfillset (&act.sa_mask);
  sigemptyset (&act.sa_mask);

  /* Do not install handlers if interrupt state is "System".  */
  if (__gnat_get_interrupt_state (SIGABRT) != 's')
    sigaction (SIGABRT, &act, NULL);
  if (__gnat_get_interrupt_state (SIGFPE) != 's')
    sigaction (SIGFPE,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGILL) != 's')
    sigaction (SIGILL,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGSEGV) != 's')
    sigaction (SIGSEGV, &act, NULL);
  if (__gnat_get_interrupt_state (SIGBUS) != 's')
    sigaction (SIGBUS,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGADAABORT) != 's')
    sigaction (SIGADAABORT,  &act, NULL);

  __gnat_handler_installed = 1;
}

/*******************/
/* LynxOS Section */
/*******************/

#elif defined (__Lynx__)

#include <signal.h>
#include <unistd.h>

static void
__gnat_error_handler (int sig)
{
  struct Exception_Data *exception;
  const char *msg;

  switch(sig)
  {
    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;
    case SIGILL:
      exception = &constraint_error;
      msg = "SIGILL";
      break;
    case SIGSEGV:
      exception = &storage_error;
      msg = "stack overflow or erroneous memory access";
      break;
    case SIGBUS:
      exception = &constraint_error;
      msg = "SIGBUS";
      break;
    default:
      exception = &program_error;
      msg = "unhandled signal";
    }

    Raise_From_Signal_Handler(exception, msg);
}

void
__gnat_install_handler(void)
{
  struct sigaction act;

  act.sa_handler = __gnat_error_handler;
  act.sa_flags = 0x0;
  sigemptyset (&act.sa_mask);

  /* Do not install handlers if interrupt state is "System".  */
  if (__gnat_get_interrupt_state (SIGFPE) != 's')
    sigaction (SIGFPE,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGILL) != 's')
    sigaction (SIGILL,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGSEGV) != 's')
    sigaction (SIGSEGV, &act, NULL);
  if (__gnat_get_interrupt_state (SIGBUS) != 's')
    sigaction (SIGBUS,  &act, NULL);

  __gnat_handler_installed = 1;
}

/*******************/
/* Solaris Section */
/*******************/

#elif defined (sun) && defined (__SVR4) && !defined (__vxworks)

#include <signal.h>
#include <siginfo.h>
#include <sys/ucontext.h>
#include <sys/regset.h>

/* The code below is common to SPARC and x86.  Beware of the delay slot
   differences for signal context adjustments.  */

#if defined (__sparc)
#define RETURN_ADDR_OFFSET 8
#else
#define RETURN_ADDR_OFFSET 0
#endif

/* Likewise regarding how the "instruction pointer" register slot can
   be identified in signal machine contexts.  We have either "REG_PC"
   or "PC" at hand, depending on the target CPU and Solaris version.  */

#if !defined (REG_PC)
#define REG_PC PC
#endif

static void __gnat_error_handler (int, siginfo_t *, ucontext_t *);

static void
__gnat_error_handler (int sig, siginfo_t *sip, ucontext_t *uctx)
{
  struct Exception_Data *exception;
  static int recurse = 0;
  const char *msg;

  /* If this was an explicit signal from a "kill", just resignal it.  */
  if (SI_FROMUSER (sip))
    {
      signal (sig, SIG_DFL);
      kill (getpid(), sig);
    }

  /* Otherwise, treat it as something we handle.  */
  switch (sig)
    {
    case SIGSEGV:
      /* If the problem was permissions, this is a constraint error.
	 Likewise if the failing address isn't maximally aligned or if
	 we've recursed.

	 ??? Using a static variable here isn't task-safe, but it's
	 much too hard to do anything else and we're just determining
	 which exception to raise.  */
      if (sip->si_code == SEGV_ACCERR
	  || (((long) sip->si_addr) & 3) != 0
	  || recurse)
	{
	  exception = &constraint_error;
	  msg = "SIGSEGV";
	}
      else
	{
	  /* See if the page before the faulting page is accessible.  Do that
	     by trying to access it.  We'd like to simply try to access
	     4096 + the faulting address, but it's not guaranteed to be
	     the actual address, just to be on the same page.  */
	  recurse++;
	  ((volatile char *)
	   ((long) sip->si_addr & - getpagesize ()))[getpagesize ()];
	  exception = &storage_error;
	  msg = "stack overflow (or erroneous memory access)";
	}
      break;

    case SIGBUS:
      exception = &program_error;
      msg = "SIGBUS";
      break;

    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;

    default:
      exception = &program_error;
      msg = "unhandled signal";
    }

  recurse = 0;

  Raise_From_Signal_Handler (exception, msg);
}

void
__gnat_install_handler (void)
{
  struct sigaction act;

  /* Set up signal handler to map synchronous signals to appropriate
     exceptions.  Make sure that the handler isn't interrupted by another
     signal that might cause a scheduling event!  */

  act.sa_handler = __gnat_error_handler;
  act.sa_flags = SA_NODEFER | SA_RESTART | SA_SIGINFO;
  sigemptyset (&act.sa_mask);

  /* Do not install handlers if interrupt state is "System".  */
  if (__gnat_get_interrupt_state (SIGABRT) != 's')
    sigaction (SIGABRT, &act, NULL);
  if (__gnat_get_interrupt_state (SIGFPE) != 's')
    sigaction (SIGFPE,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGSEGV) != 's')
    sigaction (SIGSEGV, &act, NULL);
  if (__gnat_get_interrupt_state (SIGBUS) != 's')
    sigaction (SIGBUS,  &act, NULL);

  __gnat_handler_installed = 1;
}

/***************/
/* VMS Section */
/***************/

#elif defined (VMS)

/* Routine called from binder to override default feature values. */
void __gnat_set_features ();
int __gnat_features_set = 0;

long __gnat_error_handler (int *, void *);

#ifdef __IA64
#define lib_get_curr_invo_context LIB$I64_GET_CURR_INVO_CONTEXT
#define lib_get_prev_invo_context LIB$I64_GET_PREV_INVO_CONTEXT
#define lib_get_invo_handle LIB$I64_GET_INVO_HANDLE
#else
#define lib_get_curr_invo_context LIB$GET_CURR_INVO_CONTEXT
#define lib_get_prev_invo_context LIB$GET_PREV_INVO_CONTEXT
#define lib_get_invo_handle LIB$GET_INVO_HANDLE
#endif

#if defined (IN_RTS) && !defined (__IA64)

/* The prehandler actually gets control first on a condition.  It swaps the
   stack pointer and calls the handler (__gnat_error_handler).  */
extern long __gnat_error_prehandler (void);

extern char *__gnat_error_prehandler_stack;   /* Alternate signal stack */
#endif

/* Define macro symbols for the VMS conditions that become Ada exceptions.
   Most of these are also defined in the header file ssdef.h which has not
   yet been converted to be recognized by GNU C.  */

/* Defining these as macros, as opposed to external addresses, allows
   them to be used in a case statement below.  */
#define SS$_ACCVIO            12
#define SS$_HPARITH         1284
#define SS$_STKOVF          1364
#define SS$_RESIGNAL        2328

/* These codes are in standard message libraries.  */
extern int CMA$_EXIT_THREAD;
extern int SS$_DEBUG;
extern int SS$_INTDIV;
extern int LIB$_KEYNOTFOU;
extern int LIB$_ACTIMAGE;
extern int MTH$_FLOOVEMAT;       /* Some ACVC_21 CXA tests */

/* These codes are non standard, which is to say the author is
   not sure if they are defined in the standard message libraries
   so keep them as macros for now.  */
#define RDB$_STREAM_EOF 20480426
#define FDL$_UNPRIKW 11829410

struct cond_except {
  const int *cond;
  const struct Exception_Data *except;
};

struct descriptor_s {unsigned short len, mbz; __char_ptr32 adr; };

/* Conditions that don't have an Ada exception counterpart must raise
   Non_Ada_Error.  Since this is defined in s-auxdec, it should only be
   referenced by user programs, not the compiler or tools.  Hence the
   #ifdef IN_RTS.  */

#ifdef IN_RTS

#define Status_Error ada__io_exceptions__status_error
extern struct Exception_Data Status_Error;

#define Mode_Error ada__io_exceptions__mode_error
extern struct Exception_Data Mode_Error;

#define Name_Error ada__io_exceptions__name_error
extern struct Exception_Data Name_Error;

#define Use_Error ada__io_exceptions__use_error
extern struct Exception_Data Use_Error;

#define Device_Error ada__io_exceptions__device_error
extern struct Exception_Data Device_Error;

#define End_Error ada__io_exceptions__end_error
extern struct Exception_Data End_Error;

#define Data_Error ada__io_exceptions__data_error
extern struct Exception_Data Data_Error;

#define Layout_Error ada__io_exceptions__layout_error
extern struct Exception_Data Layout_Error;

#define Non_Ada_Error system__aux_dec__non_ada_error
extern struct Exception_Data Non_Ada_Error;

#define Coded_Exception system__vms_exception_table__coded_exception
extern struct Exception_Data *Coded_Exception (Exception_Code);

#define Base_Code_In system__vms_exception_table__base_code_in
extern Exception_Code Base_Code_In (Exception_Code);

/* DEC Ada exceptions are not defined in a header file, so they
   must be declared as external addresses.  */

extern int ADA$_PROGRAM_ERROR;
extern int ADA$_LOCK_ERROR;
extern int ADA$_EXISTENCE_ERROR;
extern int ADA$_KEY_ERROR;
extern int ADA$_KEYSIZERR;
extern int ADA$_STAOVF;
extern int ADA$_CONSTRAINT_ERRO;
extern int ADA$_IOSYSFAILED;
extern int ADA$_LAYOUT_ERROR;
extern int ADA$_STORAGE_ERROR;
extern int ADA$_DATA_ERROR;
extern int ADA$_DEVICE_ERROR;
extern int ADA$_END_ERROR;
extern int ADA$_MODE_ERROR;
extern int ADA$_NAME_ERROR;
extern int ADA$_STATUS_ERROR;
extern int ADA$_NOT_OPEN;
extern int ADA$_ALREADY_OPEN;
extern int ADA$_USE_ERROR;
extern int ADA$_UNSUPPORTED;
extern int ADA$_FAC_MODE_MISMAT;
extern int ADA$_ORG_MISMATCH;
extern int ADA$_RFM_MISMATCH;
extern int ADA$_RAT_MISMATCH;
extern int ADA$_MRS_MISMATCH;
extern int ADA$_MRN_MISMATCH;
extern int ADA$_KEY_MISMATCH;
extern int ADA$_MAXLINEXC;
extern int ADA$_LINEXCMRS;

/* DEC Ada specific conditions.  */
static const struct cond_except dec_ada_cond_except_table [] = {
  {&ADA$_PROGRAM_ERROR,   &program_error},
  {&ADA$_USE_ERROR,       &Use_Error},
  {&ADA$_KEYSIZERR,       &program_error},
  {&ADA$_STAOVF,          &storage_error},
  {&ADA$_CONSTRAINT_ERRO, &constraint_error},
  {&ADA$_IOSYSFAILED,     &Device_Error},
  {&ADA$_LAYOUT_ERROR,    &Layout_Error},
  {&ADA$_STORAGE_ERROR,   &storage_error},
  {&ADA$_DATA_ERROR,      &Data_Error},
  {&ADA$_DEVICE_ERROR,    &Device_Error},
  {&ADA$_END_ERROR,       &End_Error},
  {&ADA$_MODE_ERROR,      &Mode_Error},
  {&ADA$_NAME_ERROR,      &Name_Error},
  {&ADA$_STATUS_ERROR,    &Status_Error},
  {&ADA$_NOT_OPEN,        &Use_Error},
  {&ADA$_ALREADY_OPEN,    &Use_Error},
  {&ADA$_USE_ERROR,       &Use_Error},
  {&ADA$_UNSUPPORTED,     &Use_Error},
  {&ADA$_FAC_MODE_MISMAT, &Use_Error},
  {&ADA$_ORG_MISMATCH,    &Use_Error},
  {&ADA$_RFM_MISMATCH,    &Use_Error},
  {&ADA$_RAT_MISMATCH,    &Use_Error},
  {&ADA$_MRS_MISMATCH,    &Use_Error},
  {&ADA$_MRN_MISMATCH,    &Use_Error},
  {&ADA$_KEY_MISMATCH,    &Use_Error},
  {&ADA$_MAXLINEXC,       &constraint_error},
  {&ADA$_LINEXCMRS,       &constraint_error},
  {0,                     0}
};

#if 0
   /* Already handled by a pragma Import_Exception
      in Aux_IO_Exceptions */
  {&ADA$_LOCK_ERROR,      &Lock_Error},
  {&ADA$_EXISTENCE_ERROR, &Existence_Error},
  {&ADA$_KEY_ERROR,       &Key_Error},
#endif

#endif /* IN_RTS */

/* Non-DEC Ada specific conditions.  We could probably also put
   SS$_HPARITH here and possibly SS$_ACCVIO, SS$_STKOVF.  */
static const struct cond_except cond_except_table [] = {
  {&MTH$_FLOOVEMAT, &constraint_error},
  {&SS$_INTDIV,     &constraint_error},
  {0,               0}
};

/* To deal with VMS conditions and their mapping to Ada exceptions,
   the __gnat_error_handler routine below is installed as an exception
   vector having precedence over DEC frame handlers.  Some conditions
   still need to be handled by such handlers, however, in which case
   __gnat_error_handler needs to return SS$_RESIGNAL.  Consider for
   instance the use of a third party library compiled with DECAda and
   performing its own exception handling internally.

   To allow some user-level flexibility, which conditions should be
   resignaled is controlled by a predicate function, provided with the
   condition value and returning a boolean indication stating whether
   this condition should be resignaled or not.

   That predicate function is called indirectly, via a function pointer,
   by __gnat_error_handler, and changing that pointer is allowed to the
   the user code by way of the __gnat_set_resignal_predicate interface.

   The user level function may then implement what it likes, including
   for instance the maintenance of a dynamic data structure if the set
   of to be resignalled conditions has to change over the program's
   lifetime.

   ??? This is not a perfect solution to deal with the possible
   interactions between the GNAT and the DECAda exception handling
   models and better (more general) schemes are studied.  This is so
   just provided as a convenient workaround in the meantime, and
   should be use with caution since the implementation has been kept
   very simple.  */

typedef int
resignal_predicate (int code);

const int *cond_resignal_table [] = {
  &CMA$_EXIT_THREAD,
  &SS$_DEBUG,
  &LIB$_KEYNOTFOU,
  &LIB$_ACTIMAGE,
  (int *) RDB$_STREAM_EOF,
  (int *) FDL$_UNPRIKW,
  0
};

const int facility_resignal_table [] = {
  0x1380000, /* RDB */
  0x2220000, /* SQL */
  0
};

/* Default GNAT predicate for resignaling conditions.  */

static int
__gnat_default_resignal_p (int code)
{
  int i, iexcept;

  for (i = 0; facility_resignal_table [i]; i++)
    if ((code & 0xfff0000) == facility_resignal_table [i])
      return 1;

  for (i = 0, iexcept = 0;
       cond_resignal_table [i] &&
       !(iexcept = LIB$MATCH_COND (&code, &cond_resignal_table [i]));
       i++);

  return iexcept;
}

/* Static pointer to predicate that the __gnat_error_handler exception
   vector invokes to determine if it should resignal a condition.  */

static resignal_predicate * __gnat_resignal_p = __gnat_default_resignal_p;

/* User interface to change the predicate pointer to PREDICATE. Reset to
   the default if PREDICATE is null.  */

void
__gnat_set_resignal_predicate (resignal_predicate * predicate)
{
  if (predicate == 0)
    __gnat_resignal_p = __gnat_default_resignal_p;
  else
    __gnat_resignal_p = predicate;
}

/* Should match System.Parameters.Default_Exception_Msg_Max_Length.  */
#define Default_Exception_Msg_Max_Length 512

/* Action routine for SYS$PUTMSG. There may be multiple
   conditions, each with text to be appended to MESSAGE
   and separated by line termination.  */

static int
copy_msg (msgdesc, message)
     struct descriptor_s *msgdesc;
     char *message;
{
  int len = strlen (message);
  int copy_len;

  /* Check for buffer overflow and skip.  */
  if (len > 0 && len <= Default_Exception_Msg_Max_Length - 3)
    {
      strcat (message, "\r\n");
      len += 2;
    }

  /* Check for buffer overflow and truncate if necessary.  */
  copy_len = (len + msgdesc->len <= Default_Exception_Msg_Max_Length - 1 ?
	      msgdesc->len :
	      Default_Exception_Msg_Max_Length - 1 - len);
  strncpy (&message [len], msgdesc->adr, copy_len);
  message [len + copy_len] = 0;

  return 0;
}

long
__gnat_handle_vms_condition (int *sigargs, void *mechargs)
{
  struct Exception_Data *exception = 0;
  Exception_Code base_code;
  struct descriptor_s gnat_facility = {4,0,"GNAT"};
  char message [Default_Exception_Msg_Max_Length];

  const char *msg = "";

  /* Check for conditions to resignal which aren't effected by pragma
     Import_Exception.  */
  if (__gnat_resignal_p (sigargs [1]))
    return SS$_RESIGNAL;

#ifdef IN_RTS
  /* See if it's an imported exception.  Beware that registered exceptions
     are bound to their base code, with the severity bits masked off.  */
  base_code = Base_Code_In ((Exception_Code) sigargs [1]);
  exception = Coded_Exception (base_code);

  if (exception)
    {
      message [0] = 0;

      /* Subtract PC & PSL fields which messes with PUTMSG.  */
      sigargs [0] -= 2;
      SYS$PUTMSG (sigargs, copy_msg, &gnat_facility, message);
      sigargs [0] += 2;
      msg = message;

      exception->Name_Length = 19;
      /* ??? The full name really should be get sys$getmsg returns.  */
      exception->Full_Name = "IMPORTED_EXCEPTION";
      exception->Import_Code = base_code;

#ifdef __IA64
      /* Do not adjust the program counter as already points to the next
	 instruction (just after the call to LIB$STOP).  */
      Raise_From_Signal_Handler (exception, msg);
#endif
    }
#endif

  if (exception == 0)
    switch (sigargs[1])
      {
      case SS$_ACCVIO:
        if (sigargs[3] == 0)
	  {
	    exception = &constraint_error;
	    msg = "access zero";
	  }
	else
	  {
	    exception = &storage_error;
	    msg = "stack overflow (or erroneous memory access)";
	  }
	__gnat_adjust_context_for_raise (0, (void *)mechargs);
	break;

      case SS$_STKOVF:
	exception = &storage_error;
	msg = "stack overflow";
	__gnat_adjust_context_for_raise (0, (void *)mechargs);
	break;

      case SS$_HPARITH:
#ifndef IN_RTS
	return SS$_RESIGNAL; /* toplev.c handles for compiler */
#else
	exception = &constraint_error;
	msg = "arithmetic error";
#ifndef __alpha__
	/* No need to adjust pc on Alpha: the pc is already on the instruction
	   after the trapping one.  */
	__gnat_adjust_context_for_raise (0, (void *)mechargs);
#endif
#endif
	break;

      default:
#ifdef IN_RTS
	{
	  int i;

	  /* Scan the DEC Ada exception condition table for a match and fetch
	     the associated GNAT exception pointer.  */
	  for (i = 0;
	       dec_ada_cond_except_table [i].cond &&
	       !LIB$MATCH_COND (&sigargs [1],
			        &dec_ada_cond_except_table [i].cond);
	       i++);
	  exception = (struct Exception_Data *)
	    dec_ada_cond_except_table [i].except;

	  if (!exception)
	    {
	      /* Scan the VMS standard condition table for a match and fetch
		 the associated GNAT exception pointer.  */
	      for (i = 0;
		   cond_except_table [i].cond &&
		   !LIB$MATCH_COND (&sigargs [1], &cond_except_table [i].cond);
		   i++);
	      exception = (struct Exception_Data *)
		cond_except_table [i].except;

	      if (!exception)
		/* User programs expect Non_Ada_Error to be raised, reference
		   DEC Ada test CXCONDHAN.  */
		exception = &Non_Ada_Error;
	    }
	}
#else
	exception = &program_error;
#endif
	message [0] = 0;
	/* Subtract PC & PSL fields which messes with PUTMSG.  */
	sigargs [0] -= 2;
	SYS$PUTMSG (sigargs, copy_msg, &gnat_facility, message);
	sigargs [0] += 2;
	msg = message;
	break;
      }

  Raise_From_Signal_Handler (exception, msg);
}

long
__gnat_error_handler (int *sigargs, void *mechargs)
{
  return __gnat_handle_vms_condition (sigargs, mechargs);
}

void
__gnat_install_handler (void)
{
  long prvhnd ATTRIBUTE_UNUSED;

#if !defined (IN_RTS)
  SYS$SETEXV (1, __gnat_error_handler, 3, &prvhnd);
#endif

  /* On alpha-vms, we avoid the global vector annoyance thanks to frame based
     handlers to turn conditions into exceptions since GCC 3.4.  The global
     vector is still required for earlier GCC versions.  We're resorting to
     the __gnat_error_prehandler assembly function in this case.  */

#if defined (IN_RTS) && defined (__alpha__)
  if ((__GNUC__ * 10 + __GNUC_MINOR__) < 34)
    {
      char * c = (char *) xmalloc (2049);

      __gnat_error_prehandler_stack = &c[2048];
      SYS$SETEXV (1, __gnat_error_prehandler, 3, &prvhnd);
    }
#endif

  __gnat_handler_installed = 1;
}

/* __gnat_adjust_context_for_raise for Alpha - see comments along with the
   default version later in this file.  */

#if defined (IN_RTS) && defined (__alpha__)

#include <vms/chfctxdef.h>
#include <vms/chfdef.h>

#define HAVE_GNAT_ADJUST_CONTEXT_FOR_RAISE

void
__gnat_adjust_context_for_raise (int signo ATTRIBUTE_UNUSED, void *ucontext)
{
  /* Add one to the address of the instruction signaling the condition,
     located in the sigargs array.  */

  CHF$MECH_ARRAY * mechargs = (CHF$MECH_ARRAY *) ucontext;
  CHF$SIGNAL_ARRAY * sigargs
    = (CHF$SIGNAL_ARRAY *) mechargs->chf$q_mch_sig_addr;

  int vcount = sigargs->chf$is_sig_args;
  int * pc_slot = & (&sigargs->chf$l_sig_name)[vcount-2];

  (*pc_slot) ++;
}

#endif

/* __gnat_adjust_context_for_raise for ia64.  */

#if defined (IN_RTS) && defined (__IA64)

#include <vms/chfctxdef.h>
#include <vms/chfdef.h>

#define HAVE_GNAT_ADJUST_CONTEXT_FOR_RAISE

typedef unsigned long long u64;

void
__gnat_adjust_context_for_raise (int signo ATTRIBUTE_UNUSED, void *ucontext)
{
  /* Add one to the address of the instruction signaling the condition,
     located in the 64bits sigargs array.  */

  CHF$MECH_ARRAY * mechargs = (CHF$MECH_ARRAY *) ucontext;

  CHF64$SIGNAL_ARRAY *chfsig64
    = (CHF64$SIGNAL_ARRAY *) mechargs->chf$ph_mch_sig64_addr;

  u64 * post_sigarray
    = (u64 *)chfsig64 + 1 + chfsig64->chf64$l_sig_args;

  u64 * ih_pc_loc = post_sigarray - 2;

  (*ih_pc_loc) ++;
}

#endif

/* Feature logical name and global variable address pair */
struct feature {char *name; int* gl_addr;};

/* Default values for GNAT features set by environment. */
int __gl_no_malloc_64 = 0;

/* Array feature logical names and global variable addresses */
static struct feature features[] = {
  {"GNAT$NO_MALLOC_64", &__gl_no_malloc_64},
  {0, 0}
};

void __gnat_set_features ()
{
  struct descriptor_s name_desc, result_desc;
  int i, status;
  unsigned short rlen;

#define MAXEQUIV 10
  char buff [MAXEQUIV];

  /* Loop through features array and test name for enable/disable */
  for (i=0; features [i].name; i++)
    {
       name_desc.len = strlen (features [i].name);
       name_desc.mbz = 0;
       name_desc.adr = features [i].name;

       result_desc.len = MAXEQUIV - 1;
       result_desc.mbz = 0;
       result_desc.adr = buff;

       status = LIB$GET_LOGICAL (&name_desc, &result_desc, &rlen);

       if (((status & 1) == 1) && (rlen < MAXEQUIV))
         buff [rlen] = 0;
       else
         strcpy (buff, "");

       if (strcmp (buff, "ENABLE") == 0)
          *features [i].gl_addr = 1;
       else if (strcmp (buff, "DISABLE") == 0)
          *features [i].gl_addr = 0;
    }

    __gnat_features_set = 1;
}

/*******************/
/* FreeBSD Section */
/*******************/

#elif defined (__FreeBSD__)

#include <signal.h>
#include <sys/ucontext.h>
#include <unistd.h>

static void __gnat_error_handler (int, siginfo_t *, ucontext_t *);

static void
__gnat_error_handler (int sig, siginfo_t *info __attribute__ ((unused)),
		      ucontext_t *ucontext)
{
  struct Exception_Data *exception;
  const char *msg;

  switch (sig)
    {
    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;

    case SIGILL:
      exception = &constraint_error;
      msg = "SIGILL";
      break;

    case SIGSEGV:
      exception = &storage_error;
      msg = "stack overflow or erroneous memory access";
      break;

    case SIGBUS:
      exception = &constraint_error;
      msg = "SIGBUS";
      break;

    default:
      exception = &program_error;
      msg = "unhandled signal";
    }

  Raise_From_Signal_Handler (exception, msg);
}

void
__gnat_install_handler ()
{
  struct sigaction act;

  /* Set up signal handler to map synchronous signals to appropriate
     exceptions.  Make sure that the handler isn't interrupted by another
     signal that might cause a scheduling event!  */

  act.sa_sigaction
    = (void (*)(int, struct __siginfo *, void*)) __gnat_error_handler;
  act.sa_flags = SA_NODEFER | SA_RESTART | SA_SIGINFO;
  (void) sigemptyset (&act.sa_mask);

  (void) sigaction (SIGILL,  &act, NULL);
  (void) sigaction (SIGFPE,  &act, NULL);
  (void) sigaction (SIGSEGV, &act, NULL);
  (void) sigaction (SIGBUS,  &act, NULL);

  __gnat_handler_installed = 1;
}

/*******************/
/* VxWorks Section */
/*******************/

#elif defined(__vxworks)

#include <signal.h>
#include <taskLib.h>

#ifndef __RTP__
#include <intLib.h>
#include <iv.h>
#endif

#ifdef VTHREADS
#include "private/vThreadsP.h"
#endif

void __gnat_error_handler (int, void *, struct sigcontext *);

#ifndef __RTP__

/* Directly vectored Interrupt routines are not supported when using RTPs.  */

extern int __gnat_inum_to_ivec (int);

/* This is needed by the GNAT run time to handle Vxworks interrupts.  */
int
__gnat_inum_to_ivec (int num)
{
  return INUM_TO_IVEC (num);
}
#endif

#if !defined(__alpha_vxworks) && (_WRS_VXWORKS_MAJOR != 6) && !defined(__RTP__)

/* getpid is used by s-parint.adb, but is not defined by VxWorks, except
   on Alpha VxWorks and VxWorks 6.x (including RTPs).  */

extern long getpid (void);

long
getpid (void)
{
  return taskIdSelf ();
}
#endif

/* VxWorks expects the field excCnt to be zeroed when a signal is handled.
   The VxWorks version of longjmp does this; GCC's builtin_longjmp doesn't.  */
void
__gnat_clear_exception_count (void)
{
#ifdef VTHREADS
  WIND_TCB *currentTask = (WIND_TCB *) taskIdSelf();

  currentTask->vThreads.excCnt = 0;
#endif
}

/* Handle different SIGnal to exception mappings in different VxWorks
   versions.   */
static void
__gnat_map_signal (int sig)
{
  struct Exception_Data *exception;
  const char *msg;

  switch (sig)
    {
    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;
#ifdef VTHREADS
    case SIGILL:
      exception = &constraint_error;
      msg = "Floating point exception or SIGILL";
      break;
    case SIGSEGV:
      exception = &storage_error;
      msg = "SIGSEGV: possible stack overflow";
      break;
    case SIGBUS:
      exception = &storage_error;
      msg = "SIGBUS: possible stack overflow";
      break;
#else
#ifdef __RTP__
    /* In RTP mode a SIGSEGV is most likely due to a stack overflow,
       since stack checking uses the probing mechanism.  */
    case SIGILL:
      exception = &constraint_error;
      msg = "SIGILL";
      break;
    case SIGSEGV:
      exception = &storage_error;
      msg = "SIGSEGV: possible stack overflow";
      break;
#else
    /* In kernel mode a SIGILL is most likely due to a stack overflow,
       since stack checking uses the stack limit mechanism.  */
    case SIGILL:
      exception = &storage_error;
      msg = "SIGILL: possible stack overflow";
      break;
    case SIGSEGV:
      exception = &program_error;
      msg = "SIGSEGV";
      break;
#endif
    case SIGBUS:
      exception = &program_error;
      msg = "SIGBUS";
      break;
#endif
    default:
      exception = &program_error;
      msg = "unhandled signal";
    }

  __gnat_clear_exception_count ();
  Raise_From_Signal_Handler (exception, msg);
}

/* Tasking and Non-tasking signal handler.  Map SIGnal to Ada exception
   propagation after the required low level adjustments.  */

void
__gnat_error_handler (int sig, void * si ATTRIBUTE_UNUSED,
		      struct sigcontext * sc)
{
  sigset_t mask;

  /* VxWorks will always mask out the signal during the signal handler and
     will reenable it on a longjmp.  GNAT does not generate a longjmp to
     return from a signal handler so the signal will still be masked unless
     we unmask it.  */
  sigprocmask (SIG_SETMASK, NULL, &mask);
  sigdelset (&mask, sig);
  sigprocmask (SIG_SETMASK, &mask, NULL);

  __gnat_map_signal (sig);
}

void
__gnat_install_handler (void)
{
  struct sigaction act;

  /* Setup signal handler to map synchronous signals to appropriate
     exceptions.  Make sure that the handler isn't interrupted by another
     signal that might cause a scheduling event!  */

  act.sa_handler = __gnat_error_handler;
  act.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigemptyset (&act.sa_mask);

  /* For VxWorks, install all signal handlers, since pragma Interrupt_State
     applies to vectored hardware interrupts, not signals.  */
  sigaction (SIGFPE,  &act, NULL);
  sigaction (SIGILL,  &act, NULL);
  sigaction (SIGSEGV, &act, NULL);
  sigaction (SIGBUS,  &act, NULL);

  __gnat_handler_installed = 1;
}

#define HAVE_GNAT_INIT_FLOAT

void
__gnat_init_float (void)
{
  /* Disable overflow/underflow exceptions on the PPC processor, needed
     to get correct Ada semantics.  Note that for AE653 vThreads, the HW
     overflow settings are an OS configuration issue.  The instructions
     below have no effect.  */
#if defined (_ARCH_PPC) && !defined (_SOFT_FLOAT) && !defined (VTHREADS)
  asm ("mtfsb0 25");
  asm ("mtfsb0 26");
#endif

#if (defined (__i386__) || defined (i386)) && !defined (VTHREADS)
  /* This is used to properly initialize the FPU on an x86 for each
     process thread.  */
  asm ("finit");
#endif

  /* Similarly for SPARC64.  Achieved by masking bits in the Trap Enable Mask
     field of the Floating-point Status Register (see the SPARC Architecture
     Manual Version 9, p 48).  */
#if defined (sparc64)

#define FSR_TEM_NVM (1 << 27)  /* Invalid operand  */
#define FSR_TEM_OFM (1 << 26)  /* Overflow  */
#define FSR_TEM_UFM (1 << 25)  /* Underflow  */
#define FSR_TEM_DZM (1 << 24)  /* Division by Zero  */
#define FSR_TEM_NXM (1 << 23)  /* Inexact result  */
  {
    unsigned int fsr;

    __asm__("st %%fsr, %0" : "=m" (fsr));
    fsr &= ~(FSR_TEM_OFM | FSR_TEM_UFM);
    __asm__("ld %0, %%fsr" : : "m" (fsr));
  }
#endif
}

/* This subprogram is called by System.Task_Primitives.Operations.Enter_Task
   (if not null) when a new task is created.  It is initialized by
   System.Stack_Checking.Operations.Initialize_Stack_Limit.
   The use of a hook avoids to drag stack checking subprograms if stack
   checking is not used.  */
void (*__gnat_set_stack_limit_hook)(void) = (void (*)(void))0;


/******************/
/* NetBSD Section */
/******************/

#elif defined(__NetBSD__)

#include <signal.h>
#include <unistd.h>

static void
__gnat_error_handler (int sig)
{
  struct Exception_Data *exception;
  const char *msg;

  switch(sig)
  {
    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;
    case SIGILL:
      exception = &constraint_error;
      msg = "SIGILL";
      break;
    case SIGSEGV:
      exception = &storage_error;
      msg = "stack overflow or erroneous memory access";
      break;
    case SIGBUS:
      exception = &constraint_error;
      msg = "SIGBUS";
      break;
    default:
      exception = &program_error;
      msg = "unhandled signal";
    }

    Raise_From_Signal_Handler(exception, msg);
}

void
__gnat_install_handler(void)
{
  struct sigaction act;

  act.sa_handler = __gnat_error_handler;
  act.sa_flags = SA_NODEFER | SA_RESTART;
  sigemptyset (&act.sa_mask);

  /* Do not install handlers if interrupt state is "System".  */
  if (__gnat_get_interrupt_state (SIGFPE) != 's')
    sigaction (SIGFPE,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGILL) != 's')
    sigaction (SIGILL,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGSEGV) != 's')
    sigaction (SIGSEGV, &act, NULL);
  if (__gnat_get_interrupt_state (SIGBUS) != 's')
    sigaction (SIGBUS,  &act, NULL);

  __gnat_handler_installed = 1;
}

/*******************/
/* OpenBSD Section */
/*******************/

#elif defined(__OpenBSD__)

#include <signal.h>
#include <unistd.h>

static void
__gnat_error_handler (int sig)
{
  struct Exception_Data *exception;
  const char *msg;

  switch(sig)
  {
    case SIGFPE:
      exception = &constraint_error;
      msg = "SIGFPE";
      break;
    case SIGILL:
      exception = &constraint_error;
      msg = "SIGILL";
      break;
    case SIGSEGV:
      exception = &storage_error;
      msg = "stack overflow or erroneous memory access";
      break;
    case SIGBUS:
      exception = &constraint_error;
      msg = "SIGBUS";
      break;
    default:
      exception = &program_error;
      msg = "unhandled signal";
    }

    Raise_From_Signal_Handler(exception, msg);
}

void
__gnat_install_handler(void)
{
  struct sigaction act;

  act.sa_handler = __gnat_error_handler;
  act.sa_flags = SA_NODEFER | SA_RESTART;
  sigemptyset (&act.sa_mask);

  /* Do not install handlers if interrupt state is "System" */
  if (__gnat_get_interrupt_state (SIGFPE) != 's')
    sigaction (SIGFPE,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGILL) != 's')
    sigaction (SIGILL,  &act, NULL);
  if (__gnat_get_interrupt_state (SIGSEGV) != 's')
    sigaction (SIGSEGV, &act, NULL);
  if (__gnat_get_interrupt_state (SIGBUS) != 's')
    sigaction (SIGBUS,  &act, NULL);

  __gnat_handler_installed = 1;
}

#else

/* For all other versions of GNAT, the handler does nothing.  */

/*******************/
/* Default Section */
/*******************/

void
__gnat_install_handler (void)
{
  __gnat_handler_installed = 1;
}

#endif

/*********************/
/* __gnat_init_float */
/*********************/

/* This routine is called as each process thread is created, for possible
   initialization of the FP processor.  This version is used under INTERIX,
   WIN32 and could be used under OS/2.  */

#if defined (_WIN32) || defined (__INTERIX) || defined (__EMX__) \
  || defined (__Lynx__) || defined(__NetBSD__) || defined(__FreeBSD__) \
  || defined (__OpenBSD__)

#define HAVE_GNAT_INIT_FLOAT

void
__gnat_init_float (void)
{
#if defined (__i386__) || defined (i386)

  /* This is used to properly initialize the FPU on an x86 for each
     process thread.  */

  asm ("finit");

#endif  /* Defined __i386__ */
}
#endif

#ifndef HAVE_GNAT_INIT_FLOAT

/* All targets without a specific __gnat_init_float will use an empty one.  */
void
__gnat_init_float (void)
{
}
#endif

/***********************************/
/* __gnat_adjust_context_for_raise */
/***********************************/

#ifndef HAVE_GNAT_ADJUST_CONTEXT_FOR_RAISE

/* All targets without a specific version will use an empty one.  */

/* Given UCONTEXT a pointer to a context structure received by a signal
   handler for SIGNO, perform the necessary adjustments to let the handler
   raise an exception.  Calls to this routine are not conditioned by the
   propagation scheme in use.  */

void
__gnat_adjust_context_for_raise (int signo ATTRIBUTE_UNUSED,
				 void *ucontext ATTRIBUTE_UNUSED)
{
  /* We used to compensate here for the raised from call vs raised from signal
     exception discrepancy with the GCC ZCX scheme, but this now can be dealt
     with generically in the unwinder (see GCC PR other/26208).  This however
     requires the use of the _Unwind_GetIPInfo routine in raise-gcc.c, which
     is predicated on the definition of HAVE_GETIPINFO at compile time.  Only
     the VMS ports still do the compensation described in the few lines below.

     *** Call vs signal exception discrepancy with GCC ZCX scheme ***

     The GCC unwinder expects to be dealing with call return addresses, since
     this is the "nominal" case of what we retrieve while unwinding a regular
     call chain.

     To evaluate if a handler applies at some point identified by a return
     address, the propagation engine needs to determine what region the
     corresponding call instruction pertains to.  Because the return address
     may not be attached to the same region as the call, the unwinder always
     subtracts "some" amount from a return address to search the region
     tables, amount chosen to ensure that the resulting address is inside the
     call instruction.

     When we raise an exception from a signal handler, e.g. to transform a
     SIGSEGV into Storage_Error, things need to appear as if the signal
     handler had been "called" by the instruction which triggered the signal,
     so that exception handlers that apply there are considered.  What the
     unwinder will retrieve as the return address from the signal handler is
     what it will find as the faulting instruction address in the signal
     context pushed by the kernel.  Leaving this address untouched looses, if
     the triggering instruction happens to be the very first of a region, as
     the later adjustments performed by the unwinder would yield an address
     outside that region.  We need to compensate for the unwinder adjustments
     at some point, and this is what this routine is expected to do.

     signo is passed because on some targets for some signals the PC in
     context points to the instruction after the faulting one, in which case
     the unwinder adjustment is still desired.  */
}

#endif
