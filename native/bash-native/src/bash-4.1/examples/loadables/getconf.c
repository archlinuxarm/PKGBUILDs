/*
 * ORIGINAL COPYRIGHT STATEMENT:
 *
 * Copyright (c) 1994 Winning Strategies, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Winning Strategies, Inc.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * POSIX.2 getconf utility
 *
 * Originally Written by:
 *	J.T. Conklin (jtc@wimsey.com), Winning Strategies, Inc.
 *
 * Heavily modified for inclusion in bash by
 *	Chet Ramey <chet@po.cwru.edu>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif

#include <stdio.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include "typemax.h"

#include "bashansi.h"
#include "shell.h"
#include "builtins.h"
#include "stdc.h"
#include "common.h"
#include "bashgetopt.h"

#include "getconf.h"

#ifndef errno
extern int errno;
#endif

struct conf_variable
{
  const char *name;
  enum { SYSCONF, CONFSTR, PATHCONF, CONSTANT, LLCONST, G_UNDEF } type;
  long value;
};

#ifndef HAVE_CONFSTR
static size_t confstr __P((int, char *, size_t));
#endif

#ifndef HAVE_SYSCONF
static long sysconf __P((int));
#endif

#ifndef HAVE_PATHCONF
static long pathconf __P((const char *, int));
#endif

/* Hack to `encode' values wider than long into a conf_variable */
#define VAL_LLONG_MIN		-1000
#define VAL_LLONG_MAX		-1001
#define VAL_ULLONG_MAX		-1002

static const struct conf_variable conf_table[] =
{
  /* POSIX.2 Configurable Variable Values */
  { "PATH",			CONFSTR,	_CS_PATH		},
  { "CS_PATH",			CONFSTR,	_CS_PATH		},

  /* POSIX.1 Configurable Variable Values (only Solaris?) */
#if defined (_CS_LFS_CFLAGS)
  { "LFS_CFLAGS",		CONFSTR,	_CS_LFS_CFLAGS		},
  { "LFS_LDFLAGS",		CONFSTR,	_CS_LFS_LDFLAGS		},
  { "LFS_LIBS",			CONFSTR,	_CS_LFS_LIBS		},
  { "LFS_LINTFLAGS",		CONFSTR,	_CS_LFS_LINTFLAGS	},
#endif
#if defined (_CS_LFS64_CFLAGS)
  { "LFS64_CFLAGS",		CONFSTR,	_CS_LFS64_CFLAGS	},
  { "LFS64_LDFLAGS",		CONFSTR,	_CS_LFS64_LDFLAGS	},
  { "LFS64_LIBS",		CONFSTR,	_CS_LFS64_LIBS		},
  { "LFS64_LINTFLAGS",		CONFSTR,	_CS_LFS64_LINTFLAGS	},
#endif

  /* Single UNIX Specification version 2 Configurable Variable Values.  The
     SYSCONF variables say whether or not the appropriate CONFSTR variables
     are available. */
#if defined (_SC_XBS5_ILP32_OFF32)
  { "XBS5_ILP32_OFF32",			SYSCONF,	_SC_XBS5_ILP32_OFF32	},
  { "_XBS5_ILP32_OFF32",		SYSCONF,	_SC_XBS5_ILP32_OFF32	},
#endif
#if defined (_CS_XBS5_ILP32_OFF32_CFLAGS)
  { "XBS5_ILP32_OFF32_CFLAGS",  	CONFSTR,	_CS_XBS5_ILP32_OFF32_CFLAGS },
  { "XBS5_ILP32_OFF32_LDFLAGS",		CONFSTR,	_CS_XBS5_ILP32_OFF32_LDFLAGS },
  { "XBS5_ILP32_OFF32_LIBS",    	CONFSTR,	_CS_XBS5_ILP32_OFF32_LIBS },
  { "XBS5_ILP32_OFF32_LINTFLAGS",       CONFSTR,	_CS_XBS5_ILP32_OFF32_LINTFLAGS },
#endif
#if defined (_SC_XBS5_ILP32_OFFBIG)
  { "XBS5_ILP32_OFFBIG",		SYSCONF,	_SC_XBS5_ILP32_OFFBIG	},
  { "_XBS5_ILP32_OFFBIG",		SYSCONF,	_SC_XBS5_ILP32_OFFBIG	},
#endif
#if defined (_CS_XBS5_ILP32_OFFBIG_CFLAGS)
  { "XBS5_ILP32_OFFBIG_CFLAGS", 	CONFSTR,	_CS_XBS5_ILP32_OFFBIG_CFLAGS },
  { "XBS5_ILP32_OFFBIG_LDFLAGS",	CONFSTR,	_CS_XBS5_ILP32_OFFBIG_LDFLAGS },
  { "XBS5_ILP32_OFFBIG_LIBS",   	CONFSTR,	_CS_XBS5_ILP32_OFFBIG_LIBS },
  { "XBS5_ILP32_OFFBIG_LINTFLAGS",      CONFSTR,	_CS_XBS5_ILP32_OFFBIG_LINTFLAGS },
#endif
#if defined (_SC_XBS5_LP64_OFF64)
  { "XBS5_LP64_OFF64",			SYSCONF,	_SC_XBS5_LP64_OFF64	},
  { "_XBS5_LP64_OFF64",			SYSCONF,	_SC_XBS5_LP64_OFF64	},
#endif
#if defined (_CS_XBS5_LP64_OFF64_CFLAGS)
  { "XBS5_LP64_OFF64_CFLAGS",   	CONFSTR,	_CS_XBS5_LP64_OFF64_CFLAGS },
  { "XBS5_LP64_OFF64_LDFLAGS",  	CONFSTR,	_CS_XBS5_LP64_OFF64_LDFLAGS },
  { "XBS5_LP64_OFF64_LIBS",     	CONFSTR,	_CS_XBS5_LP64_OFF64_LIBS },
  { "XBS5_LP64_OFF64_LINTFLAGS",	CONFSTR,	_CS_XBS5_LP64_OFF64_LINTFLAGS },
#endif
#if defined (_SC_XBS5_LPBIG_OFFBIG)
  { "XBS5_LPBIG_OFFBIG",		SYSCONF,	_SC_XBS5_LPBIG_OFFBIG	},
  { "_XBS5_LPBIG_OFFBIG",		SYSCONF,	_SC_XBS5_LPBIG_OFFBIG	},
#endif
#if defined (_CS_XBS5_LPBIG_OFFBIG_CFLAGS)
  { "XBS5_LPBIG_OFFBIG_CFLAGS", 	CONFSTR,	_CS_XBS5_LPBIG_OFFBIG_CFLAGS },
  { "XBS5_LPBIG_OFFBIG_LDFLAGS",	CONFSTR,	_CS_XBS5_LPBIG_OFFBIG_LDFLAGS },
  { "XBS5_LPBIG_OFFBIG_LIBS",   	CONFSTR,	_CS_XBS5_LPBIG_OFFBIG_LIBS },
  { "XBS5_LPBIG_OFFBIG_LINTFLAGS",      CONFSTR,	_CS_XBS5_LPBIG_OFFBIG_LINTFLAGS },
#endif

  /* Single UNIX Specification version 3 (POSIX.1-200x) Configurable Variable
     Values.  The SYSCONF variables say whether or not the appropriate CONFSTR
     variables are available. */

#if defined (_SC_POSIX_V6_ILP32_OFF32)
  { "_POSIX_V6_ILP32_OFF32",		SYSCONF,	_SC_POSIX_V6_ILP32_OFF32  },
#endif
#if defined (_CS_POSIX_V6_ILP32_OFF32_CFLAGS)
  { "POSIX_V6_ILP32_OFF32_CFLAGS",	CONFSTR,	_CS_POSIX_V6_ILP32_OFF32_CFLAGS },
  { "POSIX_V6_ILP32_OFF32_LDFLAGS",	CONFSTR,	_CS_POSIX_V6_ILP32_OFF32_LDFLAGS },
  { "POSIX_V6_ILP32_OFF32_LIBS",	CONFSTR,	_CS_POSIX_V6_ILP32_OFF32_LIBS },
#endif
#if defined (_CS_POSIX_V6_ILP32_OFF32_LINTFLAGS)
  { "POSIX_V6_ILP32_OFF32_LINTFLAGS",	CONFSTR,	_CS_POSIX_V6_ILP32_OFF32_LINTFLAGS },
#endif
#if defined (_SC_POSIX_V6_ILP32_OFFBIG)
  { "_POSIX_V6_ILP32_OFFBIG",		SYSCONF,	_SC_POSIX_V6_ILP32_OFFBIG },
#endif
#if defined (_CS_POSIX_V6_ILP32_OFFBIG_CFLAGS)
  { "POSIX_V6_ILP32_OFFBIG_CFLAGS",	CONFSTR,	_CS_POSIX_V6_ILP32_OFFBIG_CFLAGS },
  { "POSIX_V6_ILP32_OFFBIG_LDFLAGS",	CONFSTR,	_CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS },
  { "POSIX_V6_ILP32_OFFBIG_LIBS",	CONFSTR,	_CS_POSIX_V6_ILP32_OFFBIG_LIBS },
#endif
#if defined (_CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS)
  { "POSIX_V6_ILP32_OFFBIG_LINTFLAGS",	CONFSTR,	_CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS },
#endif
#if defined (_SC_POSIX_V6_LP64_OFF64)
  { "_POSIX_V6_LP64_OFF64",		SYSCONF,	_SC_POSIX_V6_LP64_OFF64	  },
#endif
#if defined (_CS_POSIX_V6_LP64_OFF64_CFLAGS)
  { "POSIX_V6_LP64_OFF64_CFLAGS",	CONFSTR,	_CS_POSIX_V6_LP64_OFF64_CFLAGS },
  { "POSIX_V6_LP64_OFF64_LDFLAGS",	CONFSTR,	_CS_POSIX_V6_LP64_OFF64_LDFLAGS },
  { "POSIX_V6_LP64_OFF64_LIBS",		CONFSTR,	_CS_POSIX_V6_LP64_OFF64_LIBS },
#endif
#if defined (CS_POSIX_V6_LP64_OFF64_LINTFLAGS)
  { "POSIX_V6_LP64_OFF64_LINTFLAGS",	CONFSTR,	_CS_POSIX_V6_LP64_OFF64_LINTFLAGS },
#endif
#if defined (_SC_POSIX_V6_LPBIG_OFFBIG)
  { "_POSIX_V6_LPBIG_OFFBIG",	SYSCONF,	_SC_POSIX_V6_LPBIG_OFFBIG },
#endif
#if defined (_CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS)
  { "POSIX_V6_LPBIG_OFFBIG_CFLAGS",	CONFSTR,	_CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS },
  { "POSIX_V6_LPBIG_OFFBIG_LDFLAGS",	CONFSTR,	_CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS },
  { "POSIX_V6_LPBIG_OFFBIG_LIBS",	CONFSTR,	_CS_POSIX_V6_LPBIG_OFFBIG_LIBS },
#endif
#if defined (_CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS)
  { "POSIX_V6_LPBIG_OFFBIG_LINTFLAGS",	CONFSTR,	_CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS },
#endif

#if defined (_CS_POSIX_V6_WIDTH_RESTRICTED_ENVS)
  { "POSIX_6_WIDTH_RESTRICTED_ENVS",	CONFSTR,	_CS_POSIX_V6_WIDTH_RESTRICTED_ENVS },
#endif

  /* POSIX.2 Utility Limit Minimum Values */
#ifdef _POSIX2_BC_BASE_MAX
  { "POSIX2_BC_BASE_MAX",	CONSTANT,	_POSIX2_BC_BASE_MAX	},
#else
  { "POSIX2_BC_BASE_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_BC_DIM_MAX
  { "POSIX2_BC_DIM_MAX",	CONSTANT,	_POSIX2_BC_DIM_MAX	},
#else
  { "POSIX2_BC_DIM_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_BC_SCALE_MAX
  { "POSIX2_BC_SCALE_MAX",	CONSTANT,	_POSIX2_BC_SCALE_MAX	},
#else
  { "POSIX2_BC_SCALE_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_BC_STRING_MAX
  { "POSIX2_BC_STRING_MAX",	CONSTANT,	_POSIX2_BC_STRING_MAX	},
#else
  { "POSIX2_BC_STRING_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_CHARCLASS_NAME_MAX
  { "POSIX2_CHARCLASS_NAME_MAX",	CONSTANT,	_POSIX2_CHARCLASS_NAME_MAX },
#else
  { "POSIX2_CHARCLASS_NAME_MAX",	G_UNDEF,	-1			 },
#endif
#ifdef _POSIX2_COLL_WEIGHTS_MAX
  { "POSIX2_COLL_WEIGHTS_MAX",	CONSTANT,	_POSIX2_COLL_WEIGHTS_MAX },
#else
  { "POSIX2_COLL_WEIGHTS_MAX",	G_UNDEF,	-1			 },
#endif
#if defined (_POSIX2_EQUIV_CLASS_MAX)
  { "POSIX2_EQUIV_CLASS_MAX",	CONSTANT,	_POSIX2_EQUIV_CLASS_MAX	},
#endif
#ifdef _POSIX2_EXPR_NEST_MAX
  { "POSIX2_EXPR_NEST_MAX",	CONSTANT,	_POSIX2_EXPR_NEST_MAX	},
#else
  { "POSIX2_EXPR_NEST_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_LINE_MAX
  { "POSIX2_LINE_MAX",		CONSTANT,	_POSIX2_LINE_MAX	},
#else
  { "POSIX2_LINE_MAX",		G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_RE_DUP_MAX
  { "POSIX2_RE_DUP_MAX",	CONSTANT,	_POSIX2_RE_DUP_MAX	},
#else
  { "POSIX2_RE_DUP_MAX",	G_UNDEF,	-1			},
#endif
#if defined (_POSIX2_VERSION)
  { "POSIX2_VERSION",		CONSTANT,	_POSIX2_VERSION		},
#else
#  if !defined (_SC_2_VERSION)
  { "POSIX2_VERSION",		G_UNDEF,	-1			},
#  endif
#endif

#ifdef _POSIX2_BC_BASE_MAX
  { "_POSIX2_BC_BASE_MAX",	CONSTANT,	_POSIX2_BC_BASE_MAX	},
#else
  { "_POSIX2_BC_BASE_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_BC_DIM_MAX
  { "_POSIX2_BC_DIM_MAX",	CONSTANT,	_POSIX2_BC_DIM_MAX	},
#else
  { "_POSIX2_BC_DIM_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_BC_SCALE_MAX
  { "_POSIX2_BC_SCALE_MAX",	CONSTANT,	_POSIX2_BC_SCALE_MAX	},
#else
  { "_POSIX2_BC_SCALE_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_BC_STRING_MAX
  { "_POSIX2_BC_STRING_MAX",	CONSTANT,	_POSIX2_BC_STRING_MAX	},
#else
  { "_POSIX2_BC_STRING_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_CHARCLASS_NAME_MAX
  { "_POSIX2_CHARCLASS_NAME_MAX",	CONSTANT,	_POSIX2_CHARCLASS_NAME_MAX },
#else
  { "_POSIX2_CHARCLASS_NAME_MAX",	G_UNDEF,	-1			 },
#endif
#ifdef _POSIX2_COLL_WEIGHTS_MAX
  { "_POSIX2_COLL_WEIGHTS_MAX",	CONSTANT,	_POSIX2_COLL_WEIGHTS_MAX },
#else
  { "_POSIX2_COLL_WEIGHTS_MAX",	G_UNDEF,	-1			 },
#endif
#if defined (_POSIX2_EQUIV_CLASS_MAX)
  { "POSIX2_EQUIV_CLASS_MAX",	CONSTANT,	_POSIX2_EQUIV_CLASS_MAX	},
#endif
#ifdef _POSIX2_EXPR_NEST_MAX
  { "_POSIX2_EXPR_NEST_MAX",	CONSTANT,	_POSIX2_EXPR_NEST_MAX	},
#else
  { "_POSIX2_EXPR_NEST_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_LINE_MAX
  { "_POSIX2_LINE_MAX",		CONSTANT,	_POSIX2_LINE_MAX	},
#else
  { "_POSIX2_LINE_MAX",		G_UNDEF,	-1			},
#endif
#ifdef _POSIX2_RE_DUP_MAX
  { "_POSIX2_RE_DUP_MAX",	CONSTANT,	_POSIX2_RE_DUP_MAX	},
#else
  { "_POSIX2_RE_DUP_MAX",	G_UNDEF,	-1			},
#endif

  /* X/OPEN Maxmimum Values */
#ifdef _XOPEN_IOV_MAX
  { "_XOPEN_IOV_MAX",		CONSTANT,	_XOPEN_IOV_MAX		},
#else
  { "_XOPEN_IOV_MAX",		G_UNDEF,	-1			},
#endif
#ifdef _XOPEN_NAME_MAX
  { "_XOPEN_NAME_MAX",		CONSTANT,	_XOPEN_NAME_MAX		},
#else
  { "_XOPEN_NAME_MAX",		G_UNDEF,	-1			},
#endif
#ifdef _XOPEN_PATH_MAX
  { "_XOPEN_PATH_MAX",		CONSTANT,	_XOPEN_PATH_MAX		},
#else
  { "_XOPEN_PATH_MAX",		G_UNDEF,	-1			},
#endif

  /* POSIX.1 Minimum Values */
#ifdef _POSIX_AIO_LISTIO_MAX
  { "_POSIX_AIO_LISTIO_MAX",	CONSTANT,	_POSIX_AIO_LISTIO_MAX	},
#else
  { "_POSIX_AIO_LISTIO_MAX",	G_UNDEF,	-1			},
#endif
#ifdef _POSIX_AIO_MAX
  { "_POSIX_AIO_MAX",		CONSTANT,	_POSIX_AIO_MAX		},
#else
  { "_POSIX_AIO_MAX",		G_UNDEF,	-1			},
#endif
#ifdef _POSIX_ARG_MAX
  { "_POSIX_ARG_MAX",		CONSTANT,	_POSIX_ARG_MAX		},
#else
  { "_POSIX_ARG_MAX",		G_UNDEF,	-1			},
#endif
#ifdef _POSIX_CHILD_MAX
  { "_POSIX_CHILD_MAX",		CONSTANT,	_POSIX_CHILD_MAX	},
#else
  { "_POSIX_CHILD_MAX",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_DELAYTIMER_MAX
  { "_POSIX_DELAYTIMER_MAX",	CONSTANT,	_POSIX_DELAYTIMER_MAX	},
#else
  { "_POSIX_DELAYTIMER_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_HOST_NAME_MAX
  { "_POSIX_HOST_NAME_MAX",	CONSTANT,	_POSIX_HOST_NAME_MAX	},
#else
  { "_POSIX_HOST_NAME_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_LINK_MAX
  { "_POSIX_LINK_MAX",		CONSTANT,	_POSIX_LINK_MAX		},
#else
  { "_POSIX_LINK_MAX",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_LOGIN_NAME_MAX
  { "_POSIX_LOGIN_NAME_MAX",	CONSTANT,	_POSIX_LOGIN_NAME_MAX	},
#else
  { "_POSIX_LOGIN_NAME_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_MAX_CANON
  { "_POSIX_MAX_CANON",		CONSTANT,	_POSIX_MAX_CANON	},
#else
  { "_POSIX_MAX_CANON",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_MAX_INPUT
  { "_POSIX_MAX_INPUT",		CONSTANT,	_POSIX_MAX_INPUT	},
#else
  { "_POSIX_MAX_INPUT",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_MQ_OPEN_MAX
  { "_POSIX_MQ_OPEN_MAX",	CONSTANT,	_POSIX_MQ_OPEN_MAX	},
#else
  { "_POSIX_MQ_OPEN_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_MQ_PRIO_MAX
  { "_POSIX_MQ_PRIO_MAX",	CONSTANT,	_POSIX_MQ_PRIO_MAX	},
#else
  { "_POSIX_MQ_PRIO_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_NAME_MAX
  { "_POSIX_NAME_MAX",		CONSTANT,	_POSIX_NAME_MAX		},
#else
  { "_POSIX_NAME_MAX",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_NGROUPS_MAX
  { "_POSIX_NGROUPS_MAX",	CONSTANT,	_POSIX_NGROUPS_MAX	},
#else
  { "_POSIX_NGROUPS_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_OPEN_MAX
  { "_POSIX_OPEN_MAX",		CONSTANT,	_POSIX_OPEN_MAX		},
#else
  { "_POSIX_OPEN_MAX",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_PATH_MAX
  { "_POSIX_PATH_MAX",		CONSTANT,	_POSIX_PATH_MAX		},
#else
  { "_POSIX_PATH_MAX",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_PIPE_BUF
  { "_POSIX_PIPE_BUF",		CONSTANT,	_POSIX_PIPE_BUF		},
#else
  { "_POSIX_PIPE_BUF",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_RE_DUP_MAX
  { "_POSIX_RE_DUP_MAX",	CONSTANT,	_POSIX_RE_DUP_MAX	},
#else
  { "_POSIX_RE_DUP_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_RTSIG_MAX
  { "_POSIX_RTSIG_MAX",		CONSTANT,	_POSIX_RTSIG_MAX	},
#else
  { "_POSIX_RTSIG_MAX",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_SEM_NSEMS_MAX
  { "_POSIX_SEM_NSEMS_MAX",	CONSTANT,	_POSIX_SEM_NSEMS_MAX	},
#else
  { "_POSIX_SEM_NSEMS_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_SEM_VALUE_MAX
  { "_POSIX_SEM_VALUE_MAX",	CONSTANT,	_POSIX_SEM_VALUE_MAX	},
#else
  { "_POSIX_SEM_VALUE_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_SIGQUEUE_MAX
  { "_POSIX_SIGQUEUE_MAX",	CONSTANT,	_POSIX_SIGQUEUE_MAX	},
#else
  { "_POSIX_SIGQUEUE_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_SSIZE_MAX
  { "_POSIX_SSIZE_MAX",		CONSTANT,	_POSIX_SSIZE_MAX	},
#else
  { "_POSIX_SSIZE_MAX",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_SS_REPL_MAX
  { "_POSIX_SS_REPL_MAX",	CONSTANT,	_POSIX_SS_REPL_MAX	},
#else
  { "_POSIX_SS_REPL_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_STREAM_MAX
  { "_POSIX_STREAM_MAX",	CONSTANT,	_POSIX_STREAM_MAX	},
#else
  { "_POSIX_STREAM_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_SYMLINK_MAX
  { "_POSIX_SYMLINK_MAX",	CONSTANT,	_POSIX_SYMLINK_MAX	},
#else
  { "_POSIX_SYMLINK_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_SYMLOOP_MAX
  { "_POSIX_SYMLOOP_MAX",	CONSTANT,	_POSIX_SYMLOOP_MAX	},
#else
  { "_POSIX_SYMLOOP_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_THREAD_DESTRUCTOR_ITERATIONS
  { "_POSIX_THREAD_DESTRUCTOR_ITERATIONS",	CONSTANT,	_POSIX_THREAD_DESTRUCTOR_ITERATIONS	},
#else
  { "_POSIX_THREAD_DESTRUCTOR_ITERATIONS",	G_UNDEF,	-1	},
#endif  
#ifdef _POSIX_THREAD_KEYS_MAX
  { "_POSIX_THREAD_KEYS_MAX",	CONSTANT,	_POSIX_THREAD_KEYS_MAX	},
#else
  { "_POSIX_THREAD_KEYS_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_THREAD_THREADS_MAX
  { "_POSIX_THREAD_THREADS_MAX",CONSTANT,	_POSIX_THREAD_THREADS_MAX },
#else
  { "_POSIX_THREAD_THREADS_MAX",G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_TIMER_MAX
  { "_POSIX_TIMER_MAX",		CONSTANT,	_POSIX_TIMER_MAX	},
#else
  { "_POSIX_TIMER_MAX",		G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_TRACE_EVENT_NAME_MAX
  { "_POSIX_TRACE_EVENT_NAME_MAX",	CONSTANT,	_POSIX_TRACE_EVENT_NAME_MAX	},
#else
  { "_POSIX_TRACE_EVENT_NAME_MAX",	G_UNDEF,	-1		},
#endif  
#ifdef _POSIX_TRACE_NAME_MAX
  { "_POSIX_TRACE_NAME_MAX",	CONSTANT,	_POSIX_TRACE_NAME_MAX	},
#else
  { "_POSIX_TRACE_NAME_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_TRACE_SYS_MAX
  { "_POSIX_TRACE_SYS_MAX",	CONSTANT,	_POSIX_TRACE_SYS_MAX	},
#else
  { "_POSIX_TRACE_SYS_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_TRACE_USER_EVENT_MAX
  { "_POSIX_TRACE_USER_EVENT_MAX",	CONSTANT,	_POSIX_TRACE_USER_EVENT_MAX	},
#else
  { "_POSIX_TRACE_USER_EVENT_MAX",	G_UNDEF,	-1		},
#endif  
#ifdef _POSIX_TTY_NAME_MAX
  { "_POSIX_TTY_NAME_MAX",	CONSTANT,	_POSIX_TTY_NAME_MAX	},
#else
  { "_POSIX_TTY_NAME_MAX",	G_UNDEF,	-1			},
#endif  
#ifdef _POSIX_TZNAME_MAX
  { "_POSIX_TZNAME_MAX",	CONSTANT,	_POSIX_TZNAME_MAX	},
#else
  { "_POSIX_TZNAME_MAX",	G_UNDEF,	-1			},
#endif  

  /* POSIX.1 Maximum Values */
#ifdef _POSIX_CLOCKRES_MIN
  { "_POSIX_CLOCKRES_MIN",	CONSTANT,	_POSIX_CLOCKRES_MIN	},
#else
  { "_POSIX_CLOCKRES_MIN",	G_UNDEF,	-1			},
#endif

  /* POSIX.1-2001/XPG6 (and later) Runtime Invariants from <limits.h> */
#ifdef _SC_SS_REPL_MAX
  { "SS_REPL_MAX",		SYSCONF,	_SC_SS_REPL_MAX	},
#endif  
#ifdef _SC_TRACE_EVENT_NAME_MAX
  { "TRACE_EVENT_NAME_MAX",	SYSCONF,	_SC_TRACE_EVENT_NAME_MAX	},
#endif  
#ifdef _SC_TRACE_NAME_MAX
  { "TRACE_NAME_MAX",		SYSCONF,	_SC_TRACE_NAME_MAX	},
#endif  
#ifdef _SC_TRACE_SYS_MAX
  { "TRACE_SYS_MAX",		SYSCONF,	_SC_TRACE_SYS_MAX	},
#endif  
#ifdef _SC_TRACE_USER_EVENT_MAX
  { "TRACE_USER_EVENT_MAX",	SYSCONF,	_SC_TRACE_USER_EVENT_MAX	},
#endif  

  /* POSIX.2/XPG 4.2 (and later) Symbolic Utility Limits */
#ifdef _SC_BC_BASE_MAX
  { "BC_BASE_MAX",		SYSCONF,	_SC_BC_BASE_MAX		},
#endif
#ifdef _SC_BC_DIM_MAX
  { "BC_DIM_MAX",		SYSCONF,	_SC_BC_DIM_MAX		},
#endif
#ifdef _SC_BC_SCALE_MAX
  { "BC_SCALE_MAX",		SYSCONF,	_SC_BC_SCALE_MAX	},
#endif
#ifdef _SC_BC_STRING_MAX
  { "BC_STRING_MAX",		SYSCONF,	_SC_BC_STRING_MAX	},
#endif
#ifdef CHARCLASS_NAME_MAX
  { "CHARCLASS_NAME_MAX",	CONSTANT,	CHARCLASS_NAME_MAX	},
#endif
#ifdef _SC_COLL_WEIGHTS_MAX
  { "COLL_WEIGHTS_MAX",		SYSCONF,	_SC_COLL_WEIGHTS_MAX	},
#endif
#ifdef _SC_EXPR_NEST_MAX
  { "EXPR_NEST_MAX",		SYSCONF,	_SC_EXPR_NEST_MAX	},
#endif
#ifdef _SC_LINE_MAX
  { "LINE_MAX",			SYSCONF,	_SC_LINE_MAX		},
#endif
#  ifdef NL_ARGMAX
  { "NL_ARGMAX",		CONSTANT,	NL_ARGMAX		},
#endif
#ifdef NL_LANGMAX
  { "NL_LANGMAX",		CONSTANT,	NL_LANGMAX		},
#endif
#ifdef NL_MSGMAX
  { "NL_MSGMAX",		CONSTANT,	NL_MSGMAX		},
#endif
#ifdef NL_NMAX
  { "NL_NMAX",			CONSTANT,	NL_NMAX			},
#endif
#ifdef NL_SETMAX
  { "NL_SETMAX",		CONSTANT,	NL_SETMAX		},
#endif
#ifdef NL_TEXTMAX
  { "NL_TEXTMAX",		CONSTANT,	NL_TEXTMAX		},
#endif
#ifdef _SC_RAW_SOCKET
  { "RAW_SOCKET",		SYSCONF,	_SC_RAW_SOCKET		},
#endif
#ifdef _SC_RE_DUP_MAX
  { "RE_DUP_MAX",		SYSCONF,	_SC_RE_DUP_MAX		},
#endif

  /* POSIX.2 Optional Facility Configuration Values */
#ifdef _SC_2_C_BIND
  { "POSIX2_C_BIND",		SYSCONF,	_SC_2_C_BIND		},
#else
  { "POSIX2_C_BIND",		G_UNDEF,	-1			},
#endif
#ifdef _SC_2_C_DEV
  { "POSIX2_C_DEV",		SYSCONF,	_SC_2_C_DEV		},
#else
  { "POSIX2_C_DEV",		G_UNDEF,	-1			},
#endif
#if defined (_SC_2_C_VERSION)
  { "POSIX2_C_VERSION",		SYSCONF,	_SC_2_C_VERSION		},
#else
  { "POSIX2_C_VERSION",		G_UNDEF,	-1			},
#endif
#if defined (_SC_2_CHAR_TERM)
  { "POSIX2_CHAR_TERM",		SYSCONF,	_SC_2_CHAR_TERM		},
#else
  { "POSIX2_CHAR_TERM",		G_UNDEF,	-1			},
#endif
#ifdef _SC_2_FORT_DEV
  { "POSIX2_FORT_DEV",		SYSCONF,	_SC_2_FORT_DEV		},
#else
  { "POSIX2_FORT_DEV",		G_UNDEF,	-1			},
#endif
#ifdef _SC_2_FORT_RUN
  { "POSIX2_FORT_RUN",		SYSCONF,	_SC_2_FORT_RUN		},
#else
  { "POSIX2_FORT_RUN",		G_UNDEF,	-1			},
#endif
#ifdef _SC_2_LOCALEDEF
  { "POSIX2_LOCALEDEF",	SYSCONF,	_SC_2_LOCALEDEF		},
#else
  { "POSIX2_LOCALEDEF",	G_UNDEF,	-1			},
#endif
#ifdef _SC_2_SW_DEV
  { "POSIX2_SW_DEV",		SYSCONF,	_SC_2_SW_DEV		},
#else
  { "POSIX2_SW_DEV",		G_UNDEF,	-1			},
#endif
#if defined (_SC2_UPE)
  { "POSIX2_UPE",		SYSCONF,	_SC_2_UPE		},
#else
  { "POSIX2_UPE",		G_UNDEF,	-1			},
#endif
#if !defined (_POSIX2_VERSION) && defined (_SC_2_VERSION)
  { "_POSIX2_VERSION",		SYSCONF,	_SC_2_VERSION		},
#else
  { "_POSIX2_VERSION",		G_UNDEF,	-1			},
#endif
#if defined (_SC_REGEX_VERSION)
  { "REGEX_VERSION",		SYSCONF,	_SC_REGEX_VERSION	},
  { "_REGEX_VERSION",		SYSCONF,	_SC_REGEX_VERSION	},
#else
  { "REGEX_VERSION",		G_UNDEF,	-1			},
  { "_REGEX_VERSION",		G_UNDEF,	-1			},
#endif

#if defined (_SC_2_PBS)
  { "_POSIX2_PBS",		SYSCONF,	_SC_2_PBS		},
  { "_POSIX2_PBS_ACCOUNTING",	SYSCONF,	_SC_2_PBS_ACCOUNTING	},
#  if defined (_SC_2_PBS_CHECKPOINT)
  { "_POSIX2_PBS_CHECKPOINT",	SYSCONF,	_SC_2_PBS_CHECKPOINT	},
#  endif
  { "_POSIX2_PBS_LOCATE",	SYSCONF,	_SC_2_PBS_LOCATE	},
  { "_POSIX2_PBS_MESSAGE",	SYSCONF,	_SC_2_PBS_MESSAGE	},
  { "_POSIX2_PBS_TRACK",	SYSCONF,	_SC_2_PBS_TRACK		},
#endif

  /* POSIX.1 Configurable System Variables */
#ifdef _SC_ARG_MAX
  { "ARG_MAX",			SYSCONF,	_SC_ARG_MAX 		},
#endif
#ifdef _SC_CHILD_MAX
  { "CHILD_MAX",		SYSCONF,	_SC_CHILD_MAX		},
#endif
#ifdef _SC_CLK_TCK
  { "CLK_TCK",			SYSCONF,	_SC_CLK_TCK		},
#endif
#ifdef _SC_DELAYTIMER_MAX
  { "DELAYTIMER_MAX",		SYSCONF,	_SC_DELAYTIMER_MAX	},
#endif
#ifdef _SC_NGROUPS_MAX
  { "NGROUPS_MAX",		SYSCONF,	_SC_NGROUPS_MAX		},
#endif
#ifdef NZERO
  { "NZERO",			CONSTANT,	NZERO			},
#endif
#ifdef _SC_OPEN_MAX
  { "OPEN_MAX",			SYSCONF,	_SC_OPEN_MAX		},
#endif
#ifdef PASS_MAX
  { "PASS_MAX",			CONSTANT,	PASS_MAX		},
#endif
#ifdef _SC_STREAM_MAX
  { "STREAM_MAX",		SYSCONF,	_SC_STREAM_MAX		},
#endif
#ifdef TMP_MAX
  { "TMP_MAX",			CONSTANT,	TMP_MAX			},
#endif
#ifdef _SC_TZNAME_MAX
  { "TZNAME_MAX",		SYSCONF,	_SC_TZNAME_MAX		},
#endif

  /* POSIX.1 Optional Facility Configuration Values */
#if defined (_SC_ADVISORY_INFO)
  { "_POSIX_ADVISORY_INFO",	SYSCONF,	_SC_ADVISORY_INFO	},
#endif
#if defined (_SC_ASYNCHRONOUS_IO)
  { "_POSIX_ASYNCHRONOUS_IO",	SYSCONF,	_SC_ASYNCHRONOUS_IO	},
#endif
#if defined (_SC_BARRIERS)
  { "_POSIX_BARRIERS",		SYSCONF,	_SC_BARRIERS		},
#endif
#if defined (_SC_BASE)
  { "_POSIX_BASE",		SYSCONF,	_SC_BASE		},
#endif
#if defined (_SC_C_LANG_SUPPORT)
  { "_POSIX_C_LANG_SUPPORT",	SYSCONF,	_SC_C_LANG_SUPPORT	},
#endif
#if defined (_SC_C_LANG_SUPPORT_R)
  { "_POSIX_C_LANG_SUPPORT_R",	SYSCONF,	_SC_C_LANG_SUPPORT_R	},
#endif
#if defined (_SC_CLOCK_SELECTION)
  { "_POSIX_CLOCK_SELECTION",	SYSCONF,	_SC_CLOCK_SELECTION	},
#endif
#if defined (_SC_CPUTIME)
  { "_POSIX_CPUTIME",		SYSCONF,	_SC_CPUTIME		},
#endif
#if defined (_SC_DEVICE_IO)
  { "_POSIX_DEVICE_IO",		SYSCONF,	_SC_DEVICE_IO		},
#endif
#if defined (_SC_DEVICE_SPECIFIC)
  { "_POSIX_DEVICE_SPECIFIC",	SYSCONF,	_SC_DEVICE_SPECIFIC	},
#endif
#if defined (_SC_DEVICE_SPECIFIC_R)
  { "_POSIX_DEVICE_SPECIFIC_R",	SYSCONF,	_SC_DEVICE_SPECIFIC_R	},
#endif
#if defined (_SC_FD_MGMT)
  { "_POSIX_FD_MGMT",		SYSCONF,	_SC_FD_MGMT		},
#endif
#if defined (_SC_FIFO)
  { "_POSIX_FIFO",		SYSCONF,	_SC_FIFO		},
#endif
#if defined (_SC_FILE_ATTRIBUTES)
  { "_POSIX_FILE_ATTRIBUTES",	SYSCONF,	_SC_FILE_ATTRIBUTES	},
#endif
#if defined (_SC_FILE_LOCKING)
  { "_POSIX_FILE_LOCKING",	SYSCONF,	_SC_FILE_LOCKING	},
#endif
#if defined (_SC_FILE_SYSTEM)
  { "_POSIX_FILE_SYSTEM",	SYSCONF,	_SC_FILE_SYSTEM		},
#endif
#if defined (_SC_FSYNC)
  { "_POSIX_FSYNC",		SYSCONF,	_SC_FSYNC		},
#endif
#if defined (_SC_IPV6)
  { "_POSIX_IPV6",		SYSCONF,	_SC_IPV6 		},
#endif
#if defined (_SC_JOB_CONTROL)
  { "_POSIX_JOB_CONTROL",	SYSCONF,	_SC_JOB_CONTROL 	},
#endif
#if defined (_SC_MAPPED_FILES)
  { "_POSIX_MAPPED_FILES",	SYSCONF,	_SC_MAPPED_FILES	},
#endif
#if defined (_SC_MEMLOCK)
  { "_POSIX_MEMLOCK",		SYSCONF,	_SC_MEMLOCK		},
#endif
#if defined (_SC_MEMLOCK_RANGE)
  { "_POSIX_MEMLOCK_RANGE",	SYSCONF,	_SC_MEMLOCK_RANGE	},
#endif
#if defined (_SC_MEMORY_PROTECTION)
  { "_POSIX_MEMORY_PROTECTION",	SYSCONF,	_SC_MEMORY_PROTECTION	},
#endif
#if defined (_SC_MESSAGE_PASSING)
  { "_POSIX_MESSAGE_PASSING",	SYSCONF,	_SC_MESSAGE_PASSING	},
#endif
#if defined (_SC_MONOTONIC_CLOCK)
  { "_POSIX_MONOTONIC_CLOCK",	SYSCONF,	_SC_MONOTONIC_CLOCK	},
#endif
#if defined (_SC_MULTI_PROCESS)
  { "_POSIX_MULTI_PROCESS",	SYSCONF,	_SC_MULTI_PROCESS	},
#endif
#if defined (_SC_NETWORKING)
  { "_POSIX_NETWORKING",	SYSCONF,	_SC_NETWORKING		},
#endif
#if defined (_SC_PIPE)
  { "_POSIX_PIPE",		SYSCONF,	_SC_PIPE		},
#endif
#if defined (SC_PRIORITIZED_IO)
  { "_POSIX_PRIORITIZED_IO",	SYSCONF,	_SC_PRIORITIZED_IO	},
#endif
#if defined (_SC_PRIORITY_SCHEDULING)
  { "_POSIX_PRIORITY_SCHEDULING", SYSCONF,	_SC_PRIORITY_SCHEDULING	},
#endif
#if defined (_SC_READER_WRITER_LOCKS)
  { "_POSIX_READER_WRITER_LOCKS", SYSCONF,	_SC_READER_WRITER_LOCKS	},
#endif
#if defined (_SC_RAW_SOCKETS)
  { "_POSIX_RAW_SOCKETS",	SYSCONF,	_SC_RAW_SOCKETS	},
#endif
#if defined (_SC_REALTIME_SIGNALS)
  { "_POSIX_REALTIME_SIGNALS",	SYSCONF,	_SC_REALTIME_SIGNALS	},
#endif
#if defined (_SC_REGEXP)
  { "_POSIX_REGEXP",		SYSCONF,	_SC_REGEXP		},
#endif
#if defined (_SC_SAVED_IDS)
  { "_POSIX_SAVED_IDS",		SYSCONF,	_SC_SAVED_IDS		},
#endif
#if defined (_SC_SEMAPHORES)
  { "_POSIX_SEMAPHORES",	SYSCONF,	_SC_SEMAPHORES		},
#endif
#if defined (_SC_SHARED_MEMORY_OBJECTS)
  { "_POSIX_SHARED_MEMORY_OBJECTS", SYSCONF,	_SC_SHARED_MEMORY_OBJECTS },
#endif
  { "_POSIX_SHELL",		CONSTANT,	1			},
#if defined (_SC_SIGNALS)
  { "_POSIX_SIGNALS",		SYSCONF,	_SC_SIGNALS		},
#endif
#if defined (_SC_SINGLE_PROCESS)
  { "_POSIX_SINGLE_PROCESS",	SYSCONF,	_SC_SINGLE_PROCESS	},
#endif
#if defined (_SC_SPAWN)
  { "_POSIX_SPAWN",		SYSCONF,	_SC_SPAWN		},
#endif
#if defined (_SC_SPIN_LOCKS)
  { "_POSIX_SPIN_LOCKS",	SYSCONF,	_SC_SPIN_LOCKS		},
#endif
#if defined (_SC_SPORADIC_SERVER)
  { "_POSIX_SPORADIC_SERVER",	SYSCONF,	_SC_SPORADIC_SERVER	},
#endif
#if defined (_SC_SYMLOOP_MAX)
  { "_POSIX_SYMLOOP_MAX",	SYSCONF,	_SC_SYMLOOP_MAX		},
#endif
#if defined (_SC_SYNCHRONIZED_IO)
  { "_POSIX_SYNCHRONIZED_IO",	SYSCONF,	_SC_SYNCHRONIZED_IO	},
#endif
#if defined (_SC_SYSTEM_DATABASE)
  { "_POSIX_SYSTEM_DATABASE",	SYSCONF,	_SC_SYSTEM_DATABASE	},
#endif
#if defined (_SC_SYSTEM_DATABASE_R)
  { "_POSIX_SYSTEM_DATABASE_R",	SYSCONF,	_SC_SYSTEM_DATABASE_R	},
#endif
#if defined (_SC_THREAD_ATTR_STACKADDR)
  { "_POSIX_THREAD_ATTR_STACKADDR", SYSCONF,	_SC_THREAD_ATTR_STACKADDR },
#endif
#if defined (_SC_THREAD_ATTR_STACKSIZE)
  { "_POSIX_THREAD_ATTR_STACKSIZE", SYSCONF,	_SC_THREAD_ATTR_STACKSIZE },
#endif
#if defined (_SC_THREAD_CPUTIME)
  { "_POSIX_THREAD_CPUTIME",	SYSCONF,	_SC_THREAD_CPUTIME },
#endif
#if defined (_SC_THREAD_PRIO_INHERIT)
  { "_POSIX_THREAD_PRIO_INHERIT", SYSCONF,	_SC_THREAD_PRIO_INHERIT	},
#endif
#if defined (_SC_THREAD_PRIO_PROTECT)
  { "_POSIX_THREAD_PRIO_PROTECT", SYSCONF,	_SC_THREAD_PRIO_PROTECT	},
#endif
#if defined (_SC_THREAD_PRIORITY_SCHEDULING)
  { "_POSIX_THREAD_PRIORITY_SCHEDULING", SYSCONF, _SC_THREAD_PRIORITY_SCHEDULING },
#endif
#if defined (_SC_THREAD_PROCESS_SHARED)
  { "_POSIX_THREAD_PROCESS_SHARED", SYSCONF,	_SC_THREAD_PROCESS_SHARED },
#endif
#if defined (_SC_THREAD_SAFE_FUNCTIONS)
  { "_POSIX_THREAD_SAFE_FUNCTIONS", SYSCONF,	_SC_THREAD_SAFE_FUNCTIONS },
#endif
#if defined (_SC_THREAD_SPORADIC_SERVER)
  { "_POSIX_THREAD_SPORADIC_SERVER", SYSCONF,	_SC_THREAD_SPORADIC_SERVER },
#endif
#if defined (_SC_THREADS)
  { "_POSIX_THREADS",		SYSCONF,	_SC_THREADS		},
#endif
#if defined (_SC_TIMEOUTS)
  { "_POSIX_TIMEOUTS",		SYSCONF,	_SC_TIMEOUTS		},
#endif
#if defined (_SC_TIMERS)
  { "_POSIX_TIMERS",		SYSCONF,	_SC_TIMERS		},
#endif
#if defined (_SC_TRACE)
  { "_POSIX_TRACE",		SYSCONF,	_SC_TRACE		},
#endif
#if defined (_SC_TRACE)
  { "_POSIX_TRACE_EVENT_FILTER",SYSCONF,	_SC_TRACE_EVENT_FILTER	},
#endif
#if defined (_SC_TRACE)
  { "_POSIX_TRACE_INHERIT",	SYSCONF,	_SC_TRACE_INHERIT	},
#endif
#if defined (_SC_TRACE)
  { "_POSIX_TRACE_LOG",		SYSCONF,	_SC_TRACE_LOG		},
#endif
#if defined (_SC_TYPED_MEMORY_OBJECTS)
  { "_POSIX_TYPED_MEMORY_OBJECTS", SYSCONF,	_SC_TYPED_MEMORY_OBJECTS },
#endif
#if defined (_SC_VERSION)
  { "_POSIX_VERSION",		SYSCONF,	_SC_VERSION		},
#endif

  /* XPG 4.2 Configurable System Variables. */
#if defined (_SC_ATEXIT_MAX)
  { "ATEXIT_MAX",		SYSCONF,	_SC_ATEXIT_MAX		},
#endif
#if defined (_SC_GETGR_R_SIZE_MAX)
  { "GETGR_R_SIZE_MAX",		SYSCONF,	_SC_GETGR_R_SIZE_MAX	},
#endif
#if defined (_SC_GETPW_R_SIZE_MAX)
  { "GETPW_R_SIZE_MAX",		SYSCONF,	_SC_GETPW_R_SIZE_MAX	},
#endif
#if defined (_SC_HOST_NAME_MAX)
  { "HOST_NAME_MAX",		SYSCONF,	_SC_HOST_NAME_MAX	},
#endif
#if defined (_SC_IOV_MAX)
  { "IOV_MAX",			SYSCONF,	_SC_IOV_MAX		},
#endif
#if defined (_SC_LOGIN_NAME_MAX)
  { "LOGIN_NAME_MAX",		SYSCONF,	_SC_LOGIN_NAME_MAX	},
#endif
#if defined (_SC_LOGNAME_MAX)
  { "LOGNAME_MAX",		SYSCONF,	_SC_LOGNAME_MAX		},
#endif
#if defined (_SC_PAGESIZE)
  { "PAGESIZE",			SYSCONF,	_SC_PAGESIZE		},
#endif
#if defined (_SC_PAGE_SIZE)
  { "PAGE_SIZE",		SYSCONF,	_SC_PAGE_SIZE		},
#endif
#if defined (_SC_SYMLOOP_MAX)
  { "SYMLOOP_MAX",		SYSCONF,	_SC_SYMLOOP_MAX		},
#endif
#if defined (_SC_TTY_NAME_MAX)
  { "TTY_NAME_MAX",		SYSCONF,	_SC_TTY_NAME_MAX	},
#endif
#if defined (_SC_USER_GROUPS)
  { "_POSIX_USER_GROUPS",	SYSCONF,	_SC_USER_GROUPS		},
#endif
#if defined (_SC_USER_GROUPS_R)
  { "_POSIX_USER_GROUPS_R",	SYSCONF,	_SC_USER_GROUPS_R	},
#endif

#if defined (_SC_AIO_LISTIO_MAX)
  { "AIO_LISTIO_MAX",		SYSCONF,	_SC_AIO_LISTIO_MAX	},
#endif
#if defined (_SC_AIO_MAX)
  { "AIO_MAX",			SYSCONF,	_SC_AIO_MAX		},
#endif
#if defined (_SC_AIO_PRIO_DELTA_MAX)
  { "AIO_PRIO_DELTA_MAX",	SYSCONF,	_SC_AIO_PRIO_DELTA_MAX	},
#endif
#if defined (_SC_MQ_OPEN_MAX)
  { "MQ_OPEN_MAX",		SYSCONF,	_SC_MQ_OPEN_MAX		},
#endif
#if defined (_SC_MQ_PRIO_MAX)
  { "MQ_PRIO_MAX",		SYSCONF,	_SC_MQ_PRIO_MAX		},
#endif
#if defined (_SC_RTSIG_MAX)
  { "RTSIG_MAX",		SYSCONF,	_SC_RTSIG_MAX		},
#endif
#if defined (_SC_SEM_NSEMS_MAX)
  { "SEM_NSEMS_MAX",		SYSCONF,	_SC_SEM_NSEMS_MAX	},
#endif
#if defined (_SC_SEM_VALUE_MAX)
  { "SEM_VALUE_MAX",		SYSCONF,	_SC_SEM_VALUE_MAX	},
#endif
#if defined (_SC_SIGQUEUE_MAX)
  { "SIGQUEUE_MAX",		SYSCONF,	_SC_SIGQUEUE_MAX	},
#endif
#if defined (_SC_TIMER_MAX)
  { "TIMER_MAX",		SYSCONF,	_SC_TIMER_MAX		},
#endif

#if defined (_SC_THREAD_DESTRUCTOR_ITERATIONS)
  { "PTHREAD_DESTRUCTOR_ITERATIONS", SYSCONF,	_SC_THREAD_DESTRUCTOR_ITERATIONS },
#endif
#if defined (_SC_THREAD_KEYS_MAX)
  { "PTHREAD_KEYS_MAX",		SYSCONF,	_SC_THREAD_KEYS_MAX },
#endif
#if defined (_SC_THREAD_STACK_MIN)
  { "PTHREAD_STACK_MIN",	SYSCONF,	_SC_THREAD_STACK_MIN },
#endif
#if defined (_SC_THREAD_THREADS_MAX)
  { "PTHREAD_THREADS_MAX",	SYSCONF,	_SC_THREAD_THREADS_MAX },
#endif

  /* XPG 4.2 (and later) Optional Facility Configuration Values */
#if defined (_SC_XOPEN_CRYPT)
  { "_XOPEN_CRYPT",		SYSCONF,	_SC_XOPEN_CRYPT		},
#endif
#if defined (_SC_XOPEN_ENH_I18N)
  { "_XOPEN_ENH_I18N",		SYSCONF,	_SC_XOPEN_ENH_I18N	},
#endif
#if defined (_SC_XOPEN_LEGACY)
  { "_XOPEN_LEGACY",		SYSCONF,	_SC_XOPEN_LEGACY	},
#endif /* _SC_XOPEN_LEGACY */
#if defined (_SC_XOPEN_REALTIME)
  { "_XOPEN_REALTIME",		SYSCONF,	_SC_XOPEN_REALTIME	},
#endif
#if defined (_SC_XOPEN_REALTIME_THREADS)
  { "_XOPEN_REALTIME_THREADS",	SYSCONF,	_SC_XOPEN_REALTIME_THREADS },
#endif
#if defined (_SC_XOPEN_SHM)
  { "_XOPEN_SHM",		SYSCONF,	_SC_XOPEN_SHM		},
#endif
#if defined (_SC_XOPEN_STREAMS)
  { "_XOPEN_STREAMS",		SYSCONF,	_SC_XOPEN_STREAMS	},
#endif
#if defined (_SC_XOPEN_UNIX)
  { "_XOPEN_UNIX",		SYSCONF,	_SC_XOPEN_UNIX		},
#endif
#if defined (_SC_XOPEN_VERSION)
  { "_XOPEN_VERSION",		SYSCONF,	_SC_XOPEN_VERSION	},
#endif
#if defined (_SC_XOPEN_XCU_VERSION)
  { "_XOPEN_XCU_VERSION",	SYSCONF,	_SC_XOPEN_XCU_VERSION	},
#endif
#if defined (_SC_XOPEN_XPG2)
  { "_XOPEN_XPG2",		SYSCONF,	_SC_XOPEN_XPG2		},
#endif
#if defined (_SC_XOPEN_XPG3)
  { "_XOPEN_XPG3",		SYSCONF,	_SC_XOPEN_XPG3		},
#endif
#if defined (_SC_XOPEN_XPG4)
  { "_XOPEN_XPG4",		SYSCONF,	_SC_XOPEN_XPG4		},
#endif
#if defined (_SC_XOPEN_XPG5)
  { "_XOPEN_XPG5",		SYSCONF,	_SC_XOPEN_XPG5		},
#endif

  /* POSIX.1 Configurable Pathname Values */
#ifdef _PC_LINK_MAX
  { "LINK_MAX",			PATHCONF,	_PC_LINK_MAX		},
#endif
#ifdef _PC_MAX_CANON
  { "MAX_CANON",		PATHCONF,	_PC_MAX_CANON		},
#endif
#ifdef _PC_MAX_INPUT
  { "MAX_INPUT",		PATHCONF,	_PC_MAX_INPUT		},
#endif
#ifdef _PC_NAME_MAX
  { "NAME_MAX",			PATHCONF,	_PC_NAME_MAX		},
#endif
#ifdef _PC_PATH_MAX
  { "PATH_MAX",			PATHCONF,	_PC_PATH_MAX		},
#endif
#ifdef _PC_PIPE_BUF
  { "PIPE_BUF",			PATHCONF,	_PC_PIPE_BUF		},
#endif
#ifdef _PC_SYMLINK_MAX
  { "SYMLINK_MAX",		PATHCONF,	_PC_SYMLINK_MAX		},
#endif
#ifdef _PC_CHOWN_RESTRICTED
  { "_POSIX_CHOWN_RESTRICTED",	PATHCONF,	_PC_CHOWN_RESTRICTED	},
#endif
#ifdef _PC_NO_TRUNC
  { "_POSIX_NO_TRUNC",		PATHCONF,	_PC_NO_TRUNC		},
#endif
#ifdef _PC_VDISABLE
  { "_POSIX_VDISABLE",		PATHCONF,	_PC_VDISABLE		},
#endif

  /* XPG 4.2 Configurable Pathname Values */
#if defined (_PC_FILESIZEBITS)
  { "FILESIZEBITS",		PATHCONF,	_PC_FILESIZEBITS },
#endif
#if defined (_PC_ASYNC_IO)
  { "_POSIX_ASYNC_IO",		PATHCONF,	_PC_ASYNC_IO },
#endif
#if defined (_PC_PRIO_IO)
  { "_POSIX_PRIO_IO",		PATHCONF,	_PC_PRIO_IO },
#endif
#if defined (_PC_SYNC_IO)
  { "_POSIX_SYNC_IO",		PATHCONF,	_PC_SYNC_IO },
#endif

  /* POSIX.1-200x configurable pathname values */
#if defined (_PC_ALLOC_SIZE_MIN)
  { "POSIX_ALLOC_SIZE_MIN",	PATHCONF,	_PC_ALLOC_SIZE_MIN },
  { "POSIX_REC_INCR_XFER_SIZE",	PATHCONF,	_PC_REC_INCR_XFER_SIZE },
  { "POSIX_REC_MAX_XFER_SIZE",	PATHCONF,	_PC_REC_MAX_XFER_SIZE },
  { "POSIX_REC_MIN_XFER_SIZE",	PATHCONF,	_PC_REC_MIN_XFER_SIZE },
  { "POSIX_REC_XFER_ALIGN",	PATHCONF,	_PC_REC_XFER_ALIGN },
#endif

  /* ANSI/ISO C, POSIX.1-200x, XPG 4.2 (and later) C language type limits. */
  { "CHAR_BIT",			CONSTANT,	CHAR_BIT	},
  { "CHAR_MAX",			CONSTANT,	CHAR_MAX	},
  { "CHAR_MIN",			CONSTANT,	CHAR_MIN	},
  { "INT_BIT",			CONSTANT,	INT_BIT		},
  { "INT_MAX",			CONSTANT,	INT_MAX		},
  { "INT_MIN",			CONSTANT,	INT_MIN		},
#ifdef LLONG_MAX
  { "LLONG_MAX",		LLCONST,	VAL_LLONG_MAX	},
  { "LLONG_MIN",		LLCONST,	VAL_LLONG_MIN	},
#endif
  { "LONG_BIT",			CONSTANT,	LONG_BIT	},
  { "LONG_MAX",			CONSTANT,	LONG_MAX	},
  { "LONG_MIN",			CONSTANT,	LONG_MIN	},
#ifdef MB_LEN_MAX
  { "MB_LEN_MAX",		CONSTANT,	MB_LEN_MAX	},
#endif
  { "SCHAR_MAX",		CONSTANT,	SCHAR_MAX	},
  { "SCHAR_MIN",		CONSTANT,	SCHAR_MIN	},
  { "SHRT_MAX",			CONSTANT,	SHRT_MAX	},
  { "SHRT_MIN",			CONSTANT,	SHRT_MIN	},
  { "SIZE_MAX",			CONSTANT,	SIZE_MAX	},
  { "SSIZE_MAX",		CONSTANT,	SSIZE_MAX	},
  { "UCHAR_MAX",		CONSTANT,	UCHAR_MAX	},
  { "UINT_MAX",			CONSTANT,	UINT_MAX	},
#ifdef ULLONG_MAX
  { "ULLONG_MAX",		LLCONST,	VAL_ULLONG_MAX	},
#endif
  { "ULONG_MAX",		CONSTANT,	ULONG_MAX	},
  { "USHRT_MAX",		CONSTANT,	USHRT_MAX	},
  { "WORD_BIT",			CONSTANT,	WORD_BIT	},

  { NULL }
};

static int num_getconf_variables = sizeof(conf_table) / sizeof(struct conf_variable) - 1;

extern char *this_command_name;
extern char **make_builtin_argv ();

static void getconf_help ();
static int getconf_print ();
static int getconf_one ();
static int getconf_all ();

int
getconf_builtin (list)
     WORD_LIST *list;
{
  int c, r, opt, aflag;
  char **v;

  aflag = 0;
  reset_internal_getopt();
  while ((opt = internal_getopt (list, "ahv:")) != -1) {
  	switch (opt) {
  	case 'a':
  		aflag = 1;
  		break;
  	case 'h':
  		getconf_help();
  		return(EXECUTION_SUCCESS);
  	case 'v':
  		break;		/* ignored */
  	default:
  		builtin_usage();
  		return(EX_USAGE);
  	}
  }
 
  list = loptend;
  if ((aflag == 0 && list == 0) || (aflag && list) || list_length(list) > 2) {
  	builtin_usage();
  	return(EX_USAGE);
  }

  r = aflag ? getconf_all() : getconf_one(list);

  return r;
}

static void
getconf_help()
{
	const struct conf_variable *cp;
	register int i, column;

	builtin_usage();
	printf("Acceptable variable names are:\n");
	for (cp = conf_table; cp->name != NULL; cp++) {
		if (cp->type == PATHCONF)
			printf("%s pathname\n", cp->name);
		else
			printf("%s\n", cp->name);
	}
}

static int
getconf_print(cp, vpath, all)
struct conf_variable *cp;
char *vpath;
int all;
{
	long val;
	char *sval;
	size_t slen;

	switch (cp->type) {
	case G_UNDEF:
		printf("undefined\n");
		break;

#ifdef LLONG_MAX
	case LLCONST:
		switch (cp->value) {
			default:
			case VAL_LLONG_MIN:
				printf ("%lld\n", LLONG_MIN);
				break;
			case VAL_LLONG_MAX:
				printf ("%lld\n", LLONG_MAX);
				break;
#  if (ULLONG_MAX != LLONG_MAX)
			case VAL_ULLONG_MAX:
				printf ("%llu\n", ULLONG_MAX);
				break;
#  endif
		}
		break;
#endif
	case CONSTANT:
		switch (cp->value) {
			case UCHAR_MAX:
			case USHRT_MAX:
			case UINT_MAX:
#if (ULONG_MAX != UINT_MAX)			
			case ULONG_MAX:
#endif
#if (SIZE_MAX != UINT_MAX) && (SIZE_MAX != ULONG_MAX)
			case SIZE_MAX:
#endif

				printf("%lu\n", cp->value);
				break;
			default:
				printf("%ld\n", cp->value);
				break;
		}
		break;

	case CONFSTR:
		errno = 0;
		slen = confstr (cp->value, (char *) 0, (size_t) 0);
		if (slen == 0) {
			if (errno != 0) {
				if (all)
					printf ("getconf: %s\n", strerror(errno));
				else
					builtin_error ("%s", strerror(errno));
			} else
				printf ("undefined\n");
			return (EXECUTION_FAILURE);
		}
		sval = xmalloc(slen);

		confstr(cp->value, sval, slen);
		printf("%s\n", sval);
		free(sval);
		break;

	case SYSCONF:
		errno = 0;
		if ((val = sysconf(cp->value)) == -1) {
			if (errno != 0) {
				if (all)
					printf("getconf: %s\n", strerror (errno));
				else
					builtin_error ("%s", strerror (errno));
				return (EXECUTION_FAILURE);
			}

			printf ("undefined\n");
		} else {
			printf("%ld\n", val);
		}
		break;

	case PATHCONF:
		errno = 0;
		if ((val = pathconf(vpath, cp->value)) == -1) {
			if (errno != 0) {
				if (all)
					printf("getconf: %s: %s\n", vpath, strerror (errno));
				else
					builtin_error ("%s: %s", vpath, strerror (errno));
				return (EXECUTION_FAILURE);
			}

			printf ("undefined\n");
		} else {
			printf ("%ld\n", val);
		}
		break;
	}

	return (ferror(stdout) ? EXECUTION_FAILURE : EXECUTION_SUCCESS);
}

static int
getconf_all()
{
	const struct conf_variable *cp;
	int ret;

	ret = EXECUTION_SUCCESS;
	for (cp = conf_table; cp->name != NULL; cp++) {
		printf("%-35s", cp->name);
		if (getconf_print(cp, "/", 1) == EXECUTION_FAILURE)
			ret = EXECUTION_FAILURE;
	}
	return ret;
}

static int
getconf_one(list)
	WORD_LIST *list;
{
	const struct conf_variable *cp;
	char *vname, *vpath;

	vname = list->word->word;
	vpath = (list->next && list->next->word) ? list->next->word->word
						 : (char *)NULL;

	for (cp = conf_table; cp->name != NULL; cp++) {
		if (strcmp(vname, cp->name) == 0)
			break;
	}
	if (cp->name == NULL) {
		builtin_error ("%s: unknown variable", vname);
		return (EXECUTION_FAILURE);
	}

	if (cp->type == PATHCONF) {
		if (list->next == 0) {
			builtin_usage();
			return(EX_USAGE);
		}
	} else {
		if (list->next) {
			builtin_usage();
			return(EX_USAGE);
		}
	}

	return (getconf_print(cp, vpath, 0));
}

static char *getconf_doc[] = {
	"Display values of system limits and options.",
	"",
	"getconf writes the current value of a configurable system limit or",
	"option variable to the standard output.",
	(char *)NULL
};

struct builtin getconf_struct = {
	"getconf",
	getconf_builtin,
	BUILTIN_ENABLED,
	getconf_doc,
	"getconf -[ah] or getconf [-v spec] sysvar or getconf [-v spec] pathvar pathname",
	0
};

#ifndef HAVE_CONFSTR
static size_t
confstr (name, buf, len)
     int name;
     char *buf;
     size_t len;
{
  switch (name)
    {
    case _CS_PATH:
      if (len > 0 && buf)
	{
          strncpy (buf, STANDARD_UTILS_PATH, len - 1);
          buf[len - 1] = '\0';
	}
      return (sizeof (STANDARD_UTILS_PATH) + 1);
    default:
      errno = EINVAL;
      return 0;
    }
}
#endif

#ifndef HAVE_SYSCONF
extern long get_clk_tck __P((void));

static long
sysconf (name)
     int name;
{
#  if defined (_POSIX_VERSION)
  switch (name)
    {
    case _SC_ARG_MAX:
      return _POSIX_ARG_MAX;
    case _SC_CHILD_MAX:
      return _POSIX_CHILD_MAX;
    case _SC_CLK_TCK:
      return get_clk_tck();
    case _SC_NGROUPS_MAX:
      return _POSIX_NGROUPS_MAX;
    case _SC_OPEN_MAX:
      return _POSIX_OPEN_MAX;
    case _SC_JOB_CONTROL:
      return _POSIX_JOB_CONTROL;
    case _SC_SAVED_IDS:
      return _POSIX_SAVED_IDS;
    case _SC_VERSION:
      return _POSIX_VERSION;
    case _SC_BC_BASE_MAX:
      return _POSIX2_BC_BASE_MAX;
    case _SC_BC_DIM_MAX:
      return _POSIX2_BC_DIM_MAX;
    case _SC_BC_SCALE_MAX:
      return  _POSIX2_BC_SCALE_MAX;
    case _SC_BC_STRING_MAX:
      return _POSIX2_BC_STRING_MAX;
    case _SC_COLL_WEIGHTS_MAX:
      return  -1;
    case _SC_EXPR_NEST_MAX:
      return _POSIX2_EXPR_NEST_MAX;
    case _SC_LINE_MAX:
      return _POSIX2_LINE_MAX;
    case _SC_RE_DUP_MAX:
      return _POSIX2_RE_DUP_MAX;
    case _SC_STREAM_MAX:
      return _POSIX_STREAM_MAX;
    case _SC_TZNAME_MAX:
      return _POSIX_TZNAME_MAX;
    default:
      errno = EINVAL;
      return -1;
    }
#else
  errno = EINVAL;
  return -1;
#endif
}
#endif

#ifndef HAVE_PATHCONF
static long
pathconf (path, name)
     const char *path;
     int name;
{
#if defined (_POSIX_VERSION)
  switch (name)
    {
    case _PC_LINK_MAX:
      return _POSIX_LINK_MAX;
    case _PC_MAX_CANON:
      return _POSIX_MAX_CANON;
    case _PC_MAX_INPUT:
      return _POSIX_MAX_INPUT;
    case _PC_NAME_MAX:
      return _POSIX_NAME_MAX;
    case _PC_PATH_MAX:
      return _POSIX_PATH_MAX;
    case _PC_PIPE_BUF:
      return _POSIX_PIPE_BUF;
    case _PC_CHOWN_RESTRICTED:
#ifdef _POSIX_CHOWN_RESTRICTED
      return _POSIX_CHOWN_RESTRICTED;
#else
      return -1;
#endif
    case _PC_NO_TRUNC:
#ifdef _POSIX_NO_TRUNC
      return _POSIX_NO_TRUNC;
#else
      return -1;
#endif
    case _PC_VDISABLE:
#ifdef _POSIX_VDISABLE
      return _POSIX_VDISABLE;
#else
      return -1;
#endif
    default:
      errno = EINVAL;
      return -1;
    }
#else
  errno = EINVAL;
  return -1;
#endif
}
#endif
