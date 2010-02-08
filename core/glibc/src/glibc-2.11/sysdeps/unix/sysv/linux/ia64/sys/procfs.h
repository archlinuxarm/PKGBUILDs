/* Copyright (C) 1999, 2000, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _SYS_PROCFS_H
#define _SYS_PROCFS_H	1

/* This is somehow modelled after the file of the same name on SysVr4
   systems.  It provides a definition of the core file format for ELF
   used on Linux.  */

#include <features.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/user.h>

__BEGIN_DECLS

struct elf_siginfo
  {
    int si_signo;			/* Signal number.  */
    int si_code;			/* Extra code.  */
    int si_errno;			/* Errno.  */
  };

/* We really need just 72 but let's leave some headroom...  */
#define ELF_NGREG	128
/* f0 and f1 could be omitted, but so what...  */
#define ELF_NFPREG	128

typedef unsigned long elf_greg_t;
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

typedef struct ia64_fpreg elf_fpreg_t;
typedef elf_fpreg_t elf_fpregset_t[ELF_NFPREG];

typedef elf_greg_t greg_t;
typedef elf_gregset_t gregset_t;
typedef elf_fpregset_t fpregset_t;
#define NGREG ELF_NGREG

/* Definitions to generate Intel SVR4-like core files.  These mostly
   have the same names as the SVR4 types with "elf_" tacked on the
   front to prevent clashes with linux definitions, and the typedef
   forms have been avoided.  This is mostly like the SVR4 structure,
   but more Linuxy, with things that Linux does not support and which
   gdb doesn't really use excluded.  Fields present but not used are
   marked with "XXX".  */
struct elf_prstatus
  {
#if 0
    long int pr_flags;			/* XXX Process flags.  */
    short int pr_why;			/* XXX Reason for process halt.  */
    short int pr_what;			/* XXX More detailed reason.  */
#endif
    struct elf_siginfo pr_info;		/* Info associated with signal.  */
    short int pr_cursig;		/* Current signal.  */
    unsigned long int pr_sigpend;	/* Set of pending signals.  */
    unsigned long int pr_sighold;	/* Set of held signals.  */
#if 0
    struct sigaltstack pr_altstack;	/* Alternate stack info.  */
    struct sigaction pr_action;		/* Signal action for current sig.  */
#endif
    __pid_t pr_pid;
    __pid_t pr_ppid;
    __pid_t pr_pgrp;
    __pid_t pr_sid;
    struct timeval pr_utime;		/* User time.  */
    struct timeval pr_stime;		/* System time.  */
    struct timeval pr_cutime;		/* Cumulative user time.  */
    struct timeval pr_cstime;		/* Cumulative system time.  */
#if 0
    long int pr_instr;			/* Current instruction.  */
#endif
    elf_gregset_t pr_reg;		/* GP registers.  */
    int pr_fpvalid;			/* True if math copro being used.  */
  };


#define ELF_PRARGSZ     (80)    /* Number of chars for args */

struct elf_prpsinfo
  {
    char pr_state;			/* Numeric process state.  */
    char pr_sname;			/* Char for pr_state.  */
    char pr_zomb;			/* Zombie.  */
    char pr_nice;			/* Nice val.  */
    unsigned long int pr_flag;		/* Flags.  */
    unsigned int pr_uid;
    unsigned int pr_gid;
    int pr_pid, pr_ppid, pr_pgrp, pr_sid;
    /* Lots missing */
    char pr_fname[16];			/* Filename of executable.  */
    char pr_psargs[ELF_PRARGSZ];	/* Initial part of arg list.  */
  };

/* Addresses.  */
typedef void *psaddr_t;

/* Register sets.  Linux has different names.  */
typedef gregset_t prgregset_t;
typedef fpregset_t prfpregset_t;

/* We don't have any differences between processes and threads,
   therefore habe only ine PID type.  */
typedef __pid_t lwpid_t;


typedef struct elf_prstatus prstatus_t;
typedef struct elf_prpsinfo prpsinfo_t;

__END_DECLS

#endif	/* sys/procfs.h */
