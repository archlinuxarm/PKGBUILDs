/* getconf.h -- replacement definitions for ones the system doesn't provide. */

#ifndef _GETCONF_H
#define _GETCONF_H

/* Some systems do not define these; use POSIX.2 minimum recommended values. */
#ifndef _POSIX2_COLL_WEIGHTS_MAX
#  define _POSIX2_COLL_WEIGHTS_MAX 2
#endif

/* If we're on a posix system, but the system doesn't define the necessary
   constants, use posix.1 minimum values. */
#if defined (_POSIX_VERSION)

#ifndef _POSIX_ARG_MAX
#  define _POSIX_ARG_MAX	4096
#endif
#ifndef _POSIX_CHILD_MAX
#  define _POSIX_CHILD_MAX	6
#endif
#ifndef _POSIX_LINK_MAX
#  define _POSIX_LINK_MAX	8
#endif
#ifndef _POSIX_MAX_CANON
#  define _POSIX_MAX_CANON	255
#endif
#ifndef _POSIX_MAX_INPUT
#  define _POSIX_MAX_INPUT	255
#endif
#ifndef _POSIX_NAME_MAX
#  define _POSIX_NAME_MAX	14
#endif
#ifndef _POSIX_NGROUPS_MAX
#  define _POSIX_NGROUPS_MAX	0
#endif
#ifndef _POSIX_OPEN_MAX
#  define _POSIX_OPEN_MAX	16
#endif
#ifndef _POSIX_PATH_MAX
#  define _POSIX_PATH_MAX	255
#endif
#ifndef _POSIX_PIPE_BUF
#  define _POSIX_PIPE_BUF	512
#endif
#ifndef _POSIX_SSIZE_MAX
#  define _POSIX_SSIZE_MAX	32767
#endif
#ifndef _POSIX_STREAM_MAX
#  define _POSIX_STREAM_MAX	8
#endif
#ifndef _POSIX_TZNAME_MAX
#  define _POSIX_TZNAME_MAX	3
#endif

#ifndef _POSIX2_BC_BASE_MAX
#  define _POSIX2_BC_BASE_MAX     99
#endif
#ifndef _POSIX2_BC_DIM_MAX
#  define _POSIX2_BC_DIM_MAX      2048
#endif
#ifndef _POSIX2_BC_SCALE_MAX
#  define _POSIX2_BC_SCALE_MAX    99
#endif
#ifndef _POSIX2_BC_STRING_MAX
#  define _POSIX2_BC_STRING_MAX   1000
#endif
#ifndef _POSIX2_EQUIV_CLASS_MAX
#  define _POSIX2_EQUIV_CLASS_MAX 2
#endif
#ifndef _POSIX2_EXPR_NEST_MAX
#  define _POSIX2_EXPR_NEST_MAX   32
#endif
#ifndef _POSIX2_LINE_MAX
#  define _POSIX2_LINE_MAX        2048
#endif
#ifndef _POSIX2_RE_DUP_MAX
#  define _POSIX2_RE_DUP_MAX      255
#endif

/* configurable system variables */
#if !defined (HAVE_SYSCONF)

#ifndef _SC_ARG_MAX
#  define _SC_ARG_MAX              1
#  define _SC_CHILD_MAX            2
#  define _SC_CLK_TCK              3
#  define _SC_NGROUPS_MAX          4
#  define _SC_OPEN_MAX             5
#  define _SC_JOB_CONTROL          6
#  define _SC_SAVED_IDS            7
#  define _SC_VERSION              8
#  define _SC_BC_BASE_MAX          9
#  define _SC_BC_DIM_MAX          10
#  define _SC_BC_SCALE_MAX        11
#  define _SC_BC_STRING_MAX       12
#  define _SC_COLL_WEIGHTS_MAX    13
#  define _SC_EXPR_NEST_MAX       14
#  define _SC_LINE_MAX            15
#  define _SC_RE_DUP_MAX          16
#if 0
#  define _SC_2_VERSION           17
#  define _SC_2_C_BIND            18
#  define _SC_2_C_DEV             19
#  define _SC_2_CHAR_TERM         20
#  define _SC_2_FORT_DEV          21
#  define _SC_2_FORT_RUN          22
#  define _SC_2_LOCALEDEF         23
#  define _SC_2_SW_DEV            24
#  define _SC_2_UPE               25
#endif /* 0 */

#  define _SC_STREAM_MAX          26
#  define _SC_TZNAME_MAX          27
#endif /* !_SC_ARG_MAX */

#endif /* !HAVE_SYSCONF */

/* configurable pathname variables */
#if !defined (HAVE_PATHCONF)

#ifndef _PC_LINK_MAX
#define _PC_LINK_MAX             1
#define _PC_MAX_CANON            2
#define _PC_MAX_INPUT            3
#define _PC_NAME_MAX             4
#define _PC_PATH_MAX             5
#define _PC_PIPE_BUF             6
#define _PC_CHOWN_RESTRICTED     7
#define _PC_NO_TRUNC             8
#define _PC_VDISABLE             9
#endif /* !_PC_LINK_MAX */

#endif /* !HAVE_PATHCONF */

#endif /* _POSIX_VERSION */

#ifndef _CS_PATH
#  define _CS_PATH	1
#endif

/* ANSI/ISO C, POSIX.1-200x, XPG 4.2 (and later) C language type limits.
   Defined only if the system include files don't.  Assume a 32-bit
   environment with signed 8-bit characters. */

#ifndef CHAR_BIT
#  define CHAR_BIT	8
#endif
#ifndef CHAR_MAX
#  define CHAR_MAX	127
#endif
#ifndef CHAR_MIN
#  define CHAR_MIN	-128
#endif

#ifndef INT_BIT
#  define INT_BIT	(sizeof (int) * CHAR_BIT)
#endif
#ifndef INT_MAX
#  define INT_MAX	2147483647
#endif
#ifndef INT_MIN
#  define INT_MIN	(-2147483647-1)
#endif

#ifndef LONG_BIT
#  define LONG_BIT	(sizeof (long int) * CHAR_BIT)
#endif
#ifndef LONG_MAX
#  define LONG_MAX	2147483647L
#endif
#ifndef LONG_MIN
#  define LONG_MIN	(-2147483647L-1L)
#endif

#ifndef SCHAR_MAX
#  define SCHAR_MAX	CHAR_MAX
#endif
#ifndef SCHAR_MIN
#  define SCHAR_MIN	CHAR_MIN
#endif

#ifndef SHRT_MAX
#  define SHRT_MAX	32767
#endif
#ifndef SHRT_MIN
#  define SHRT_MIN	(-32768)
#endif

#ifndef UCHAR_MAX
#  define UCHAR_MAX	255
#endif
#ifndef UINT_MAX
#  define UINT_MAX	4294967295U
#endif
#ifndef ULONG_MAX
#  define ULONG_MAX	4294967295UL
#endif
#ifndef USHRT_MAX
#  define UCHAR_MAX	65535
#endif

/* assume size_t is `unsigned int'; ssize_t is `int' */
#ifndef SIZE_MAX
#  define SIZE_MAX	UINT_MAX
#endif
#ifndef SSIZE_MAX
#  define SSIZE_MAX	INT_MAX
#endif

#ifndef WORD_BIT
#  define WORD_BIT	(sizeof (int) * CHAR_BIT)
#endif

#endif /* _GETCONF_H */
