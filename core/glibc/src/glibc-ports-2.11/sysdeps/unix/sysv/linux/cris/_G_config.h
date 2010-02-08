/* This file is needed by libio to define various configuration parameters.
   These are always the same in the GNU C library.  */

/* We have to keep a separate copy for CRIS, because we don't use thunks,
   and libstdc++-v2 (which we currently use) cares.  The C++ ABI is
   changed totally with GCC 3.0, where we should not need a separate file.  */

#ifndef _G_config_h
#define _G_config_h 1

/* Define types for libio in terms of the standard internal type names.  */

#include <bits/types.h>
#define __need_size_t
#define __need_wchar_t
#define __need_wint_t
#define __need_NULL
#include <stddef.h>
#ifndef _WINT_T
/* Integral type unchanged by default argument promotions that can
   hold any value corresponding to members of the extended character
   set, as well as at least one value that does not correspond to any
   member of the extended character set.  */
# define _WINT_T
typedef unsigned int wint_t;
#endif
#define __need_mbstate_t
#include <wchar.h>
#define _G_size_t	size_t
typedef struct
{
  __off_t __pos;
  __mbstate_t __state;
} _G_fpos_t;
typedef struct
{
  __off64_t __pos;
  __mbstate_t __state;
} _G_fpos64_t;
#define _G_ssize_t	__ssize_t
#define _G_off_t	__off_t
#define _G_off64_t	__off64_t
#define	_G_pid_t	__pid_t
#define	_G_uid_t	__uid_t
#define _G_wchar_t	wchar_t
#define _G_wint_t	wint_t
#define _G_stat64	stat64
#include <gconv.h>
typedef union
{
  struct __gconv_info __cd;
  struct
  {
    struct __gconv_info __cd;
    struct __gconv_step_data __data;
  } __combined;
} _G_iconv_t;

typedef int _G_int16_t __attribute__ ((__mode__ (__HI__)));
typedef int _G_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int _G_uint16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int _G_uint32_t __attribute__ ((__mode__ (__SI__)));

#define _G_HAVE_BOOL 1


/* These library features are always available in the GNU C library.  */
#define _G_HAVE_ATEXIT 1
#define _G_HAVE_SYS_CDEFS 1
#define _G_HAVE_SYS_WAIT 1
#define _G_NEED_STDARG_H 1
#define _G_va_list __gnuc_va_list

#define _G_HAVE_PRINTF_FP 1
#define _G_HAVE_MMAP 1
#define _G_HAVE_LONG_DOUBLE_IO 1
#define _G_HAVE_IO_FILE_OPEN 1
#define _G_HAVE_IO_GETLINE_INFO 1

#define _G_IO_IO_FILE_VERSION 0x20001

#define _G_OPEN64	__open64
#define _G_LSEEK64	__lseek64
#define _G_MMAP64	__mmap64
#define _G_FSTAT64(fd,buf) __fxstat64 (_STAT_VER, fd, buf)

/* This is defined by <bits/stat.h> if `st_blksize' exists.  */
#define _G_HAVE_ST_BLKSIZE defined (_STATBUF_ST_BLKSIZE)

#define _G_BUFSIZ 8192

/* These are the vtbl details for ELF.  */
#define _G_NAMES_HAVE_UNDERSCORE 0
#define _G_VTABLE_LABEL_HAS_LENGTH 1
#undef _G_USING_THUNKS
#define _G_VTABLE_LABEL_PREFIX "_vt."
#define _G_VTABLE_LABEL_PREFIX_ID _vt.


#if defined __cplusplus || defined __STDC__
# define _G_ARGS(ARGLIST) ARGLIST
#else
# define _G_ARGS(ARGLIST) ()
#endif

#endif	/* _G_config.h */
