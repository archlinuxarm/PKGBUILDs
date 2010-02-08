/* Copyright (C) 1993, 1997-2003,2004,2005,2006,2007
	Free Software Foundation, Inc.
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
   02111-1307 USA.

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.  */

#include <errno.h>
#ifndef __set_errno
# define __set_errno(Val) errno = (Val)
#endif
#if defined __GLIBC__ && __GLIBC__ >= 2
# include <bits/libc-lock.h>
#else
/*# include <comthread.h>*/
#endif

#include <math_ldbl_opt.h>

#include "iolibio.h"

/* Control of exported symbols.  Used in glibc.  By default we don't
   do anything.  */
#ifndef INTUSE
# define INTUSE(name) name
#endif
#ifndef INTDEF
# define INTDEF(name)
#endif
#ifndef INTDEF2
# define INTDEF2(name)
#endif
#ifndef libc_hidden_proto
# define libc_hidden_proto(name)
#endif
#ifndef libc_hidden_def
# define libc_hidden_def(name)
#endif
#ifndef libc_hidden_weak
# define libc_hidden_weak(name)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define _IO_seek_set 0
#define _IO_seek_cur 1
#define _IO_seek_end 2

/* THE JUMPTABLE FUNCTIONS.

 * The _IO_FILE type is used to implement the FILE type in GNU libc,
 * as well as the streambuf class in GNU iostreams for C++.
 * These are all the same, just used differently.
 * An _IO_FILE (or FILE) object is allows followed by a pointer to
 * a jump table (of pointers to functions).  The pointer is accessed
 * with the _IO_JUMPS macro.  The jump table has a eccentric format,
 * so as to be compatible with the layout of a C++ virtual function table.
 * (as implemented by g++).  When a pointer to a streambuf object is
 * coerced to an (_IO_FILE*), then _IO_JUMPS on the result just
 * happens to point to the virtual function table of the streambuf.
 * Thus the _IO_JUMPS function table used for C stdio/libio does
 * double duty as the virtual function table for C++ streambuf.
 *
 * The entries in the _IO_JUMPS function table (and hence also the
 * virtual functions of a streambuf) are described below.
 * The first parameter of each function entry is the _IO_FILE/streambuf
 * object being acted on (i.e. the 'this' parameter).
 */

#ifdef _LIBC
# include <shlib-compat.h>
# if !SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_1)
   /* Setting this macro disables the use of the _vtable_offset
      bias in _IO_JUMPS_FUNCS, below.  That is only needed if we
      want to support old binaries (see oldfileops.c).  */
#  define _G_IO_NO_BACKWARD_COMPAT 1
# endif
#endif

#if (!defined _IO_USE_OLD_IO_FILE \
     && (!defined _G_IO_NO_BACKWARD_COMPAT || _G_IO_NO_BACKWARD_COMPAT == 0))
# define _IO_JUMPS_OFFSET 1
#endif

#define _IO_JUMPS(THIS) (THIS)->vtable
#define _IO_WIDE_JUMPS(THIS) ((struct _IO_FILE *) (THIS))->_wide_data->_wide_vtable
#define _IO_CHECK_WIDE(THIS) (((struct _IO_FILE *) (THIS))->_wide_data != NULL)

#if _IO_JUMPS_OFFSET
# define _IO_JUMPS_FUNC(THIS) \
 (*(struct _IO_jump_t **) ((void *) &_IO_JUMPS ((struct _IO_FILE_plus *) (THIS)) \
			   + (THIS)->_vtable_offset))
# define _IO_vtable_offset(THIS) (THIS)->_vtable_offset
#else
# define _IO_JUMPS_FUNC(THIS) _IO_JUMPS ((struct _IO_FILE_plus *) (THIS))
# define _IO_vtable_offset(THIS) 0
#endif
#define _IO_WIDE_JUMPS_FUNC(THIS) _IO_WIDE_JUMPS(THIS)
#ifdef _G_USING_THUNKS
# define JUMP_FIELD(TYPE, NAME) TYPE NAME
# define JUMP0(FUNC, THIS) (_IO_JUMPS_FUNC(THIS)->FUNC) (THIS)
# define JUMP1(FUNC, THIS, X1) (_IO_JUMPS_FUNC(THIS)->FUNC) (THIS, X1)
# define JUMP2(FUNC, THIS, X1, X2) (_IO_JUMPS_FUNC(THIS)->FUNC) (THIS, X1, X2)
# define JUMP3(FUNC, THIS, X1,X2,X3) (_IO_JUMPS_FUNC(THIS)->FUNC) (THIS, X1,X2, X3)
# define JUMP_INIT(NAME, VALUE) VALUE
# define JUMP_INIT_DUMMY JUMP_INIT(dummy, 0), JUMP_INIT (dummy2, 0)

# define WJUMP0(FUNC, THIS) (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC) (THIS)
# define WJUMP1(FUNC, THIS, X1) (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC) (THIS, X1)
# define WJUMP2(FUNC, THIS, X1, X2) (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC) (THIS, X1, X2)
# define WJUMP3(FUNC, THIS, X1,X2,X3) (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC) (THIS, X1,X2, X3)
#else
/* These macros will change when we re-implement vtables to use "thunks"! */
# define JUMP_FIELD(TYPE, NAME) struct { short delta1, delta2; TYPE pfn; } NAME
# define JUMP0(FUNC, THIS) _IO_JUMPS_FUNC(THIS)->FUNC.pfn (THIS)
# define JUMP1(FUNC, THIS, X1) _IO_JUMPS_FUNC(THIS)->FUNC.pfn (THIS, X1)
# define JUMP2(FUNC, THIS, X1, X2) _IO_JUMPS_FUNC(THIS)->FUNC.pfn (THIS, X1, X2)
# define JUMP3(FUNC, THIS, X1,X2,X3) _IO_JUMPS_FUNC(THIS)->FUNC.pfn (THIS, X1,X2,X3)
# define JUMP_INIT(NAME, VALUE) {0, 0, VALUE}
# define JUMP_INIT_DUMMY JUMP_INIT(dummy, 0)

# define WJUMP0(FUNC, THIS) _IO_WIDE_JUMPS_FUNC(THIS)->FUNC.pfn (THIS)
# define WJUMP1(FUNC, THIS, X1) _IO_WIDE_JUMPS_FUNC(THIS)->FUNC.pfn (THIS, X1)
# define WJUMP2(FUNC, THIS, X1, X2) _IO_WIDE_JUMPS_FUNC(THIS)->FUNC.pfn (THIS, X1, X2)
# define WJUMP3(FUNC, THIS, X1,X2,X3) _IO_WIDE_JUMPS_FUNC(THIS)->FUNC.pfn (THIS, X1,X2,X3)
#endif

/* The 'finish' function does any final cleaning up of an _IO_FILE object.
   It does not delete (free) it, but does everything else to finalize it.
   It matches the streambuf::~streambuf virtual destructor.  */
typedef void (*_IO_finish_t) (_IO_FILE *, int); /* finalize */
#define _IO_FINISH(FP) JUMP1 (__finish, FP, 0)
#define _IO_WFINISH(FP) WJUMP1 (__finish, FP, 0)

/* The 'overflow' hook flushes the buffer.
   The second argument is a character, or EOF.
   It matches the streambuf::overflow virtual function. */
typedef int (*_IO_overflow_t) (_IO_FILE *, int);
#define _IO_OVERFLOW(FP, CH) JUMP1 (__overflow, FP, CH)
#define _IO_WOVERFLOW(FP, CH) WJUMP1 (__overflow, FP, CH)

/* The 'underflow' hook tries to fills the get buffer.
   It returns the next character (as an unsigned char) or EOF.  The next
   character remains in the get buffer, and the get position is not changed.
   It matches the streambuf::underflow virtual function. */
typedef int (*_IO_underflow_t) (_IO_FILE *);
#define _IO_UNDERFLOW(FP) JUMP0 (__underflow, FP)
#define _IO_WUNDERFLOW(FP) WJUMP0 (__underflow, FP)

/* The 'uflow' hook returns the next character in the input stream
   (cast to unsigned char), and increments the read position;
   EOF is returned on failure.
   It matches the streambuf::uflow virtual function, which is not in the
   cfront implementation, but was added to C++ by the ANSI/ISO committee. */
#define _IO_UFLOW(FP) JUMP0 (__uflow, FP)
#define _IO_WUFLOW(FP) WJUMP0 (__uflow, FP)

/* The 'pbackfail' hook handles backing up.
   It matches the streambuf::pbackfail virtual function. */
typedef int (*_IO_pbackfail_t) (_IO_FILE *, int);
#define _IO_PBACKFAIL(FP, CH) JUMP1 (__pbackfail, FP, CH)
#define _IO_WPBACKFAIL(FP, CH) WJUMP1 (__pbackfail, FP, CH)

/* The 'xsputn' hook writes upto N characters from buffer DATA.
   Returns the number of character actually written.
   It matches the streambuf::xsputn virtual function. */
typedef _IO_size_t (*_IO_xsputn_t) (_IO_FILE *FP, const void *DATA,
				    _IO_size_t N);
#define _IO_XSPUTN(FP, DATA, N) JUMP2 (__xsputn, FP, DATA, N)
#define _IO_WXSPUTN(FP, DATA, N) WJUMP2 (__xsputn, FP, DATA, N)

/* The 'xsgetn' hook reads upto N characters into buffer DATA.
   Returns the number of character actually read.
   It matches the streambuf::xsgetn virtual function. */
typedef _IO_size_t (*_IO_xsgetn_t) (_IO_FILE *FP, void *DATA, _IO_size_t N);
#define _IO_XSGETN(FP, DATA, N) JUMP2 (__xsgetn, FP, DATA, N)
#define _IO_WXSGETN(FP, DATA, N) WJUMP2 (__xsgetn, FP, DATA, N)

/* The 'seekoff' hook moves the stream position to a new position
   relative to the start of the file (if DIR==0), the current position
   (MODE==1), or the end of the file (MODE==2).
   It matches the streambuf::seekoff virtual function.
   It is also used for the ANSI fseek function. */
typedef _IO_off64_t (*_IO_seekoff_t) (_IO_FILE *FP, _IO_off64_t OFF, int DIR,
				      int MODE);
#define _IO_SEEKOFF(FP, OFF, DIR, MODE) JUMP3 (__seekoff, FP, OFF, DIR, MODE)
#define _IO_WSEEKOFF(FP, OFF, DIR, MODE) WJUMP3 (__seekoff, FP, OFF, DIR, MODE)

/* The 'seekpos' hook also moves the stream position,
   but to an absolute position given by a fpos64_t (seekpos).
   It matches the streambuf::seekpos virtual function.
   It is also used for the ANSI fgetpos and fsetpos functions.  */
/* The _IO_seek_cur and _IO_seek_end options are not allowed. */
typedef _IO_off64_t (*_IO_seekpos_t) (_IO_FILE *, _IO_off64_t, int);
#define _IO_SEEKPOS(FP, POS, FLAGS) JUMP2 (__seekpos, FP, POS, FLAGS)
#define _IO_WSEEKPOS(FP, POS, FLAGS) WJUMP2 (__seekpos, FP, POS, FLAGS)

/* The 'setbuf' hook gives a buffer to the file.
   It matches the streambuf::setbuf virtual function. */
typedef _IO_FILE* (*_IO_setbuf_t) (_IO_FILE *, char *, _IO_ssize_t);
#define _IO_SETBUF(FP, BUFFER, LENGTH) JUMP2 (__setbuf, FP, BUFFER, LENGTH)
#define _IO_WSETBUF(FP, BUFFER, LENGTH) WJUMP2 (__setbuf, FP, BUFFER, LENGTH)

/* The 'sync' hook attempts to synchronize the internal data structures
   of the file with the external state.
   It matches the streambuf::sync virtual function. */
typedef int (*_IO_sync_t) (_IO_FILE *);
#define _IO_SYNC(FP) JUMP0 (__sync, FP)
#define _IO_WSYNC(FP) WJUMP0 (__sync, FP)

/* The 'doallocate' hook is used to tell the file to allocate a buffer.
   It matches the streambuf::doallocate virtual function, which is not
   in the ANSI/ISO C++ standard, but is part traditional implementations. */
typedef int (*_IO_doallocate_t) (_IO_FILE *);
#define _IO_DOALLOCATE(FP) JUMP0 (__doallocate, FP)
#define _IO_WDOALLOCATE(FP) WJUMP0 (__doallocate, FP)

/* The following four hooks (sysread, syswrite, sysclose, sysseek, and
   sysstat) are low-level hooks specific to this implementation.
   There is no correspondence in the ANSI/ISO C++ standard library.
   The hooks basically correspond to the Unix system functions
   (read, write, close, lseek, and stat) except that a _IO_FILE*
   parameter is used instead of a integer file descriptor;  the default
   implementation used for normal files just calls those functions.
   The advantage of overriding these functions instead of the higher-level
   ones (underflow, overflow etc) is that you can leave all the buffering
   higher-level functions.  */

/* The 'sysread' hook is used to read data from the external file into
   an existing buffer.  It generalizes the Unix read(2) function.
   It matches the streambuf::sys_read virtual function, which is
   specific to this implementation. */
typedef _IO_ssize_t (*_IO_read_t) (_IO_FILE *, void *, _IO_ssize_t);
#define _IO_SYSREAD(FP, DATA, LEN) JUMP2 (__read, FP, DATA, LEN)
#define _IO_WSYSREAD(FP, DATA, LEN) WJUMP2 (__read, FP, DATA, LEN)

/* The 'syswrite' hook is used to write data from an existing buffer
   to an external file.  It generalizes the Unix write(2) function.
   It matches the streambuf::sys_write virtual function, which is
   specific to this implementation. */
typedef _IO_ssize_t (*_IO_write_t) (_IO_FILE *, const void *, _IO_ssize_t);
#define _IO_SYSWRITE(FP, DATA, LEN) JUMP2 (__write, FP, DATA, LEN)
#define _IO_WSYSWRITE(FP, DATA, LEN) WJUMP2 (__write, FP, DATA, LEN)

/* The 'sysseek' hook is used to re-position an external file.
   It generalizes the Unix lseek(2) function.
   It matches the streambuf::sys_seek virtual function, which is
   specific to this implementation. */
typedef _IO_off64_t (*_IO_seek_t) (_IO_FILE *, _IO_off64_t, int);
#define _IO_SYSSEEK(FP, OFFSET, MODE) JUMP2 (__seek, FP, OFFSET, MODE)
#define _IO_WSYSSEEK(FP, OFFSET, MODE) WJUMP2 (__seek, FP, OFFSET, MODE)

/* The 'sysclose' hook is used to finalize (close, finish up) an
   external file.  It generalizes the Unix close(2) function.
   It matches the streambuf::sys_close virtual function, which is
   specific to this implementation. */
typedef int (*_IO_close_t) (_IO_FILE *); /* finalize */
#define _IO_SYSCLOSE(FP) JUMP0 (__close, FP)
#define _IO_WSYSCLOSE(FP) WJUMP0 (__close, FP)

/* The 'sysstat' hook is used to get information about an external file
   into a struct stat buffer.  It generalizes the Unix fstat(2) call.
   It matches the streambuf::sys_stat virtual function, which is
   specific to this implementation. */
typedef int (*_IO_stat_t) (_IO_FILE *, void *);
#define _IO_SYSSTAT(FP, BUF) JUMP1 (__stat, FP, BUF)
#define _IO_WSYSSTAT(FP, BUF) WJUMP1 (__stat, FP, BUF)

/* The 'showmany' hook can be used to get an image how much input is
   available.  In many cases the answer will be 0 which means unknown
   but some cases one can provide real information.  */
typedef int (*_IO_showmanyc_t) (_IO_FILE *);
#define _IO_SHOWMANYC(FP) JUMP0 (__showmanyc, FP)
#define _IO_WSHOWMANYC(FP) WJUMP0 (__showmanyc, FP)

/* The 'imbue' hook is used to get information about the currently
   installed locales.  */
typedef void (*_IO_imbue_t) (_IO_FILE *, void *);
#define _IO_IMBUE(FP, LOCALE) JUMP1 (__imbue, FP, LOCALE)
#define _IO_WIMBUE(FP, LOCALE) WJUMP1 (__imbue, FP, LOCALE)


#define _IO_CHAR_TYPE char /* unsigned char ? */
#define _IO_INT_TYPE int

struct _IO_jump_t
{
    JUMP_FIELD(_G_size_t, __dummy);
#ifdef _G_USING_THUNKS
    JUMP_FIELD(_G_size_t, __dummy2);
#endif
    JUMP_FIELD(_IO_finish_t, __finish);
    JUMP_FIELD(_IO_overflow_t, __overflow);
    JUMP_FIELD(_IO_underflow_t, __underflow);
    JUMP_FIELD(_IO_underflow_t, __uflow);
    JUMP_FIELD(_IO_pbackfail_t, __pbackfail);
    /* showmany */
    JUMP_FIELD(_IO_xsputn_t, __xsputn);
    JUMP_FIELD(_IO_xsgetn_t, __xsgetn);
    JUMP_FIELD(_IO_seekoff_t, __seekoff);
    JUMP_FIELD(_IO_seekpos_t, __seekpos);
    JUMP_FIELD(_IO_setbuf_t, __setbuf);
    JUMP_FIELD(_IO_sync_t, __sync);
    JUMP_FIELD(_IO_doallocate_t, __doallocate);
    JUMP_FIELD(_IO_read_t, __read);
    JUMP_FIELD(_IO_write_t, __write);
    JUMP_FIELD(_IO_seek_t, __seek);
    JUMP_FIELD(_IO_close_t, __close);
    JUMP_FIELD(_IO_stat_t, __stat);
    JUMP_FIELD(_IO_showmanyc_t, __showmanyc);
    JUMP_FIELD(_IO_imbue_t, __imbue);
#if 0
    get_column;
    set_column;
#endif
};

/* We always allocate an extra word following an _IO_FILE.
   This contains a pointer to the function jump table used.
   This is for compatibility with C++ streambuf; the word can
   be used to smash to a pointer to a virtual function table. */

struct _IO_FILE_plus
{
  _IO_FILE file;
  const struct _IO_jump_t *vtable;
};

#ifdef _IO_USE_OLD_IO_FILE
/* This structure is used by the compatibility code as if it were an
   _IO_FILE_plus, but has enough space to initialize the _mode argument
   of an _IO_FILE_complete.  */
struct _IO_FILE_complete_plus
{
  struct _IO_FILE_complete file;
  const struct _IO_jump_t *vtable;
};
#endif

/* Special file type for fopencookie function.  */
struct _IO_cookie_file
{
  struct _IO_FILE_plus __fp;
  void *__cookie;
  _IO_cookie_io_functions_t __io_functions;
};

_IO_FILE *_IO_fopencookie (void *cookie, const char *mode,
			   _IO_cookie_io_functions_t io_functions);


/* Iterator type for walking global linked list of _IO_FILE objects. */

typedef struct _IO_FILE *_IO_ITER;

/* Generic functions */

extern void _IO_switch_to_main_get_area (_IO_FILE *) __THROW;
extern void _IO_switch_to_backup_area (_IO_FILE *) __THROW;
extern int _IO_switch_to_get_mode (_IO_FILE *);
extern void _IO_init (_IO_FILE *, int) __THROW;
extern int _IO_sputbackc (_IO_FILE *, int) __THROW;
extern int _IO_sungetc (_IO_FILE *) __THROW;
extern void _IO_un_link (struct _IO_FILE_plus *) __THROW;
extern void _IO_link_in (struct _IO_FILE_plus *) __THROW;
extern void _IO_doallocbuf (_IO_FILE *) __THROW;
extern void _IO_unsave_markers (_IO_FILE *) __THROW;
extern void _IO_setb (_IO_FILE *, char *, char *, int) __THROW;
extern unsigned _IO_adjust_column (unsigned, const char *, int) __THROW;
#define _IO_sputn(__fp, __s, __n) _IO_XSPUTN (__fp, __s, __n)

extern void _IO_switch_to_main_wget_area (_IO_FILE *) __THROW;
extern void _IO_switch_to_wbackup_area (_IO_FILE *) __THROW;
extern int _IO_switch_to_wget_mode (_IO_FILE *);
extern void _IO_wsetb (_IO_FILE *, wchar_t *, wchar_t *, int) __THROW;
extern wint_t _IO_sputbackwc (_IO_FILE *, wint_t) __THROW;
extern wint_t _IO_sungetwc (_IO_FILE *) __THROW;
extern void _IO_wdoallocbuf (_IO_FILE *) __THROW;
extern void _IO_unsave_wmarkers (_IO_FILE *) __THROW;
extern unsigned _IO_adjust_wcolumn (unsigned, const wchar_t *, int) __THROW;

/* Marker-related function. */

extern void _IO_init_marker (struct _IO_marker *, _IO_FILE *);
extern void _IO_init_wmarker (struct _IO_marker *, _IO_FILE *);
extern void _IO_remove_marker (struct _IO_marker *) __THROW;
extern int _IO_marker_difference (struct _IO_marker *, struct _IO_marker *)
     __THROW;
extern int _IO_marker_delta (struct _IO_marker *) __THROW;
extern int _IO_wmarker_delta (struct _IO_marker *) __THROW;
extern int _IO_seekmark (_IO_FILE *, struct _IO_marker *, int) __THROW;
extern int _IO_seekwmark (_IO_FILE *, struct _IO_marker *, int) __THROW;

/* Functions for iterating global list and dealing with its lock */

extern _IO_ITER _IO_iter_begin (void) __THROW;
libc_hidden_proto (_IO_iter_begin)
extern _IO_ITER _IO_iter_end (void) __THROW;
libc_hidden_proto (_IO_iter_end)
extern _IO_ITER _IO_iter_next (_IO_ITER) __THROW;
libc_hidden_proto (_IO_iter_next)
extern _IO_FILE *_IO_iter_file (_IO_ITER) __THROW;
libc_hidden_proto (_IO_iter_file)
extern void _IO_list_lock (void) __THROW;
libc_hidden_proto (_IO_list_lock)
extern void _IO_list_unlock (void) __THROW;
libc_hidden_proto (_IO_list_unlock)
extern void _IO_list_resetlock (void) __THROW;
libc_hidden_proto (_IO_list_resetlock)

/* Default jumptable functions. */

extern int _IO_default_underflow (_IO_FILE *) __THROW;
extern int _IO_default_uflow (_IO_FILE *);
extern wint_t _IO_wdefault_uflow (_IO_FILE *);
extern int _IO_default_doallocate (_IO_FILE *) __THROW;
extern int _IO_wdefault_doallocate (_IO_FILE *) __THROW;
extern void _IO_default_finish (_IO_FILE *, int) __THROW;
extern void _IO_wdefault_finish (_IO_FILE *, int) __THROW;
extern int _IO_default_pbackfail (_IO_FILE *, int) __THROW;
extern wint_t _IO_wdefault_pbackfail (_IO_FILE *, wint_t) __THROW;
extern _IO_FILE* _IO_default_setbuf (_IO_FILE *, char *, _IO_ssize_t);
extern _IO_size_t _IO_default_xsputn (_IO_FILE *, const void *, _IO_size_t);
extern _IO_size_t _IO_wdefault_xsputn (_IO_FILE *, const void *, _IO_size_t);
extern _IO_size_t _IO_default_xsgetn (_IO_FILE *, void *, _IO_size_t);
extern _IO_size_t _IO_wdefault_xsgetn (_IO_FILE *, void *, _IO_size_t);
extern _IO_off64_t _IO_default_seekoff (_IO_FILE *, _IO_off64_t, int, int)
     __THROW;
extern _IO_off64_t _IO_default_seekpos (_IO_FILE *, _IO_off64_t, int);
extern _IO_ssize_t _IO_default_write (_IO_FILE *, const void *, _IO_ssize_t);
extern _IO_ssize_t _IO_default_read (_IO_FILE *, void *, _IO_ssize_t);
extern int _IO_default_stat (_IO_FILE *, void *) __THROW;
extern _IO_off64_t _IO_default_seek (_IO_FILE *, _IO_off64_t, int) __THROW;
extern int _IO_default_sync (_IO_FILE *) __THROW;
#define _IO_default_close ((_IO_close_t) _IO_default_sync)
extern int _IO_default_showmanyc (_IO_FILE *) __THROW;
extern void _IO_default_imbue (_IO_FILE *, void *) __THROW;

extern const struct _IO_jump_t _IO_file_jumps;
libc_hidden_proto (_IO_file_jumps)
extern const struct _IO_jump_t _IO_file_jumps_mmap attribute_hidden;
extern const struct _IO_jump_t _IO_file_jumps_maybe_mmap attribute_hidden;
extern const struct _IO_jump_t _IO_wfile_jumps;
libc_hidden_proto (_IO_wfile_jumps)
extern const struct _IO_jump_t _IO_wfile_jumps_mmap attribute_hidden;
extern const struct _IO_jump_t _IO_wfile_jumps_maybe_mmap attribute_hidden;
extern const struct _IO_jump_t _IO_old_file_jumps attribute_hidden;
extern const struct _IO_jump_t _IO_streambuf_jumps;
extern const struct _IO_jump_t _IO_old_proc_jumps attribute_hidden;
extern const struct _IO_jump_t _IO_str_jumps attribute_hidden;
extern const struct _IO_jump_t _IO_wstr_jumps attribute_hidden;
extern const struct _IO_codecvt __libio_codecvt attribute_hidden;
extern int _IO_do_write (_IO_FILE *, const char *, _IO_size_t);
extern int _IO_new_do_write (_IO_FILE *, const char *, _IO_size_t);
extern int _IO_old_do_write (_IO_FILE *, const char *, _IO_size_t);
extern int _IO_wdo_write (_IO_FILE *, const wchar_t *, _IO_size_t);
extern int _IO_flush_all_lockp (int);
extern int _IO_flush_all (void);
extern int _IO_cleanup (void);
extern void _IO_flush_all_linebuffered (void);
extern int _IO_new_fgetpos (_IO_FILE *, _IO_fpos_t *);
extern int _IO_old_fgetpos (_IO_FILE *, _IO_fpos_t *);
extern int _IO_new_fsetpos (_IO_FILE *, const _IO_fpos_t *);
extern int _IO_old_fsetpos (_IO_FILE *, const _IO_fpos_t *);
extern int _IO_new_fgetpos64 (_IO_FILE *, _IO_fpos64_t *);
extern int _IO_old_fgetpos64 (_IO_FILE *, _IO_fpos64_t *);
extern int _IO_new_fsetpos64 (_IO_FILE *, const _IO_fpos64_t *);
extern int _IO_old_fsetpos64 (_IO_FILE *, const _IO_fpos64_t *);
extern void _IO_old_init (_IO_FILE *fp, int flags) __THROW;


#if defined _LIBC || defined _GLIBCPP_USE_WCHAR_T
# define _IO_do_flush(_f) \
  ((_f)->_mode <= 0							      \
   ? INTUSE(_IO_do_write)(_f, (_f)->_IO_write_base,			      \
			  (_f)->_IO_write_ptr-(_f)->_IO_write_base)	      \
   : INTUSE(_IO_wdo_write)(_f, (_f)->_wide_data->_IO_write_base,	      \
			   ((_f)->_wide_data->_IO_write_ptr		      \
			    - (_f)->_wide_data->_IO_write_base)))
#else
# define _IO_do_flush(_f) \
   INTUSE(_IO_do_write)(_f, (_f)->_IO_write_base,			      \
			(_f)->_IO_write_ptr-(_f)->_IO_write_base)
#endif
#define _IO_old_do_flush(_f) \
  _IO_old_do_write(_f, (_f)->_IO_write_base, \
		   (_f)->_IO_write_ptr-(_f)->_IO_write_base)
#define _IO_in_put_mode(_fp) ((_fp)->_flags & _IO_CURRENTLY_PUTTING)
#define _IO_mask_flags(fp, f, mask) \
       ((fp)->_flags = ((fp)->_flags & ~(mask)) | ((f) & (mask)))
#define _IO_setg(fp, eb, g, eg)  ((fp)->_IO_read_base = (eb),\
	(fp)->_IO_read_ptr = (g), (fp)->_IO_read_end = (eg))
#define _IO_wsetg(fp, eb, g, eg)  ((fp)->_wide_data->_IO_read_base = (eb),\
	(fp)->_wide_data->_IO_read_ptr = (g), \
	(fp)->_wide_data->_IO_read_end = (eg))
#define _IO_setp(__fp, __p, __ep) \
       ((__fp)->_IO_write_base = (__fp)->_IO_write_ptr \
	= __p, (__fp)->_IO_write_end = (__ep))
#define _IO_wsetp(__fp, __p, __ep) \
       ((__fp)->_wide_data->_IO_write_base \
	= (__fp)->_wide_data->_IO_write_ptr = __p, \
	(__fp)->_wide_data->_IO_write_end = (__ep))
#define _IO_have_backup(fp) ((fp)->_IO_save_base != NULL)
#define _IO_have_wbackup(fp) ((fp)->_wide_data->_IO_save_base != NULL)
#define _IO_in_backup(fp) ((fp)->_flags & _IO_IN_BACKUP)
#define _IO_have_markers(fp) ((fp)->_markers != NULL)
#define _IO_blen(fp) ((fp)->_IO_buf_end - (fp)->_IO_buf_base)
#define _IO_wblen(fp) ((fp)->_wide_data->_IO_buf_end \
		       - (fp)->_wide_data->_IO_buf_base)

/* Jumptable functions for files. */

extern int _IO_file_doallocate (_IO_FILE *) __THROW;
extern _IO_FILE* _IO_file_setbuf (_IO_FILE *, char *, _IO_ssize_t);
extern _IO_off64_t _IO_file_seekoff (_IO_FILE *, _IO_off64_t, int, int);
extern _IO_off64_t _IO_file_seekoff_mmap (_IO_FILE *, _IO_off64_t, int, int)
     __THROW;
extern _IO_size_t _IO_file_xsputn (_IO_FILE *, const void *, _IO_size_t);
extern _IO_size_t _IO_file_xsgetn (_IO_FILE *, void *, _IO_size_t);
extern int _IO_file_stat (_IO_FILE *, void *) __THROW;
extern int _IO_file_close (_IO_FILE *) __THROW;
extern int _IO_file_close_mmap (_IO_FILE *) __THROW;
extern int _IO_file_underflow (_IO_FILE *);
extern int _IO_file_underflow_mmap (_IO_FILE *);
extern int _IO_file_underflow_maybe_mmap (_IO_FILE *);
extern int _IO_file_overflow (_IO_FILE *, int);
#define _IO_file_is_open(__fp) ((__fp)->_fileno != -1)
extern void _IO_file_init (struct _IO_FILE_plus *) __THROW;
extern _IO_FILE* _IO_file_attach (_IO_FILE *, int);
extern _IO_FILE* _IO_file_open (_IO_FILE *, const char *, int, int, int, int);
libc_hidden_proto (_IO_file_open)
extern _IO_FILE* _IO_file_fopen (_IO_FILE *, const char *, const char *, int);
extern _IO_ssize_t _IO_file_write (_IO_FILE *, const void *, _IO_ssize_t);
extern _IO_ssize_t _IO_file_read (_IO_FILE *, void *, _IO_ssize_t);
extern int _IO_file_sync (_IO_FILE *);
extern int _IO_file_close_it (_IO_FILE *);
extern _IO_off64_t _IO_file_seek (_IO_FILE *, _IO_off64_t, int) __THROW;
extern void _IO_file_finish (_IO_FILE *, int);

extern _IO_FILE* _IO_new_file_attach (_IO_FILE *, int);
extern int _IO_new_file_close_it (_IO_FILE *);
extern void _IO_new_file_finish (_IO_FILE *, int);
extern _IO_FILE* _IO_new_file_fopen (_IO_FILE *, const char *, const char *,
				     int);
extern void _IO_no_init (_IO_FILE *, int, int, struct _IO_wide_data *,
			 const struct _IO_jump_t *) __THROW;
extern void _IO_new_file_init (struct _IO_FILE_plus *) __THROW;
extern _IO_FILE* _IO_new_file_setbuf (_IO_FILE *, char *, _IO_ssize_t);
extern _IO_FILE* _IO_file_setbuf_mmap (_IO_FILE *, char *, _IO_ssize_t);
extern int _IO_new_file_sync (_IO_FILE *);
extern int _IO_new_file_underflow (_IO_FILE *);
extern int _IO_new_file_overflow (_IO_FILE *, int);
extern _IO_off64_t _IO_new_file_seekoff (_IO_FILE *, _IO_off64_t, int, int);
extern _IO_ssize_t _IO_new_file_write (_IO_FILE *, const void *, _IO_ssize_t);
extern _IO_size_t _IO_new_file_xsputn (_IO_FILE *, const void *, _IO_size_t);

extern _IO_FILE* _IO_old_file_setbuf (_IO_FILE *, char *, _IO_ssize_t);
extern _IO_off64_t _IO_old_file_seekoff (_IO_FILE *, _IO_off64_t, int, int);
extern _IO_size_t _IO_old_file_xsputn (_IO_FILE *, const void *, _IO_size_t);
extern int _IO_old_file_underflow (_IO_FILE *);
extern int _IO_old_file_overflow (_IO_FILE *, int);
extern void _IO_old_file_init (struct _IO_FILE_plus *) __THROW;
extern _IO_FILE* _IO_old_file_attach (_IO_FILE *, int);
extern _IO_FILE* _IO_old_file_fopen (_IO_FILE *, const char *, const char *);
extern _IO_ssize_t _IO_old_file_write (_IO_FILE *, const void *, _IO_ssize_t);
extern int _IO_old_file_sync (_IO_FILE *);
extern int _IO_old_file_close_it (_IO_FILE *);
extern void _IO_old_file_finish (_IO_FILE *, int);

extern int _IO_wfile_doallocate (_IO_FILE *) __THROW;
extern _IO_size_t _IO_wfile_xsputn (_IO_FILE *, const void *, _IO_size_t);
extern _IO_FILE* _IO_wfile_setbuf (_IO_FILE *, wchar_t *, _IO_ssize_t);
extern wint_t _IO_wfile_sync (_IO_FILE *);
extern wint_t _IO_wfile_underflow (_IO_FILE *);
extern wint_t _IO_wfile_overflow (_IO_FILE *, wint_t);
extern _IO_off64_t _IO_wfile_seekoff (_IO_FILE *, _IO_off64_t, int, int);

/* Jumptable functions for proc_files. */
extern _IO_FILE* _IO_proc_open (_IO_FILE *, const char *, const char *)
     __THROW;
extern _IO_FILE* _IO_new_proc_open (_IO_FILE *, const char *, const char *)
     __THROW;
extern _IO_FILE* _IO_old_proc_open (_IO_FILE *, const char *, const char *);
extern int _IO_proc_close (_IO_FILE *) __THROW;
extern int _IO_new_proc_close (_IO_FILE *) __THROW;
extern int _IO_old_proc_close (_IO_FILE *);

/* Jumptable functions for strfiles. */
extern int _IO_str_underflow (_IO_FILE *) __THROW;
extern int _IO_str_overflow (_IO_FILE *, int) __THROW;
extern int _IO_str_pbackfail (_IO_FILE *, int) __THROW;
extern _IO_off64_t _IO_str_seekoff (_IO_FILE *, _IO_off64_t, int, int) __THROW;
extern void _IO_str_finish (_IO_FILE *, int) __THROW;

/* Other strfile functions */
struct _IO_strfile_;
extern void _IO_str_init_static (struct _IO_strfile_ *, char *, int, char *)
     __THROW;
extern void _IO_str_init_readonly (struct _IO_strfile_ *, const char *, int)
     __THROW;
extern _IO_ssize_t _IO_str_count (_IO_FILE *) __THROW;

/* And the wide character versions.  */
extern void _IO_wstr_init_static (_IO_FILE *, wchar_t *, _IO_size_t, wchar_t *)
     __THROW;
extern _IO_ssize_t _IO_wstr_count (_IO_FILE *) __THROW;
extern _IO_wint_t _IO_wstr_overflow (_IO_FILE *, _IO_wint_t) __THROW;
extern _IO_wint_t _IO_wstr_underflow (_IO_FILE *) __THROW;
extern _IO_off64_t _IO_wstr_seekoff (_IO_FILE *, _IO_off64_t, int, int)
     __THROW;
extern _IO_wint_t _IO_wstr_pbackfail (_IO_FILE *, _IO_wint_t) __THROW;
extern void _IO_wstr_finish (_IO_FILE *, int) __THROW;

extern int _IO_vasprintf (char **result_ptr, __const char *format,
			  _IO_va_list args) __THROW;
extern int _IO_vdprintf (int d, __const char *format, _IO_va_list arg);
extern int _IO_vsnprintf (char *string, _IO_size_t maxlen,
			  __const char *format, _IO_va_list args) __THROW;


extern _IO_size_t _IO_getline (_IO_FILE *,char *, _IO_size_t, int, int);
extern _IO_size_t _IO_getline_info (_IO_FILE *,char *, _IO_size_t,
				    int, int, int *);
extern _IO_ssize_t _IO_getdelim (char **, _IO_size_t *, int, _IO_FILE *);
extern _IO_size_t _IO_getwline (_IO_FILE *,wchar_t *, _IO_size_t, wint_t, int);
extern _IO_size_t _IO_getwline_info (_IO_FILE *,wchar_t *, _IO_size_t,
				     wint_t, int, wint_t *);

extern struct _IO_FILE_plus *_IO_list_all;
extern void (*_IO_cleanup_registration_needed) (void);

/* Prototype for functions with alternative entry point.  */
extern int _IO_flush_all_internal (void);
extern unsigned _IO_adjust_column_internal (unsigned, const char *, int);

extern int _IO_default_uflow_internal (_IO_FILE *);
extern void _IO_default_finish_internal (_IO_FILE *, int) __THROW;
extern int _IO_default_pbackfail_internal (_IO_FILE *, int) __THROW;
extern _IO_size_t _IO_default_xsputn_internal (_IO_FILE *, const void *,
					       _IO_size_t);
extern _IO_size_t _IO_default_xsgetn_internal (_IO_FILE *, void *, _IO_size_t);
extern int _IO_default_doallocate_internal (_IO_FILE *) __THROW;
extern void _IO_wdefault_finish_internal (_IO_FILE *, int) __THROW;
extern wint_t _IO_wdefault_pbackfail_internal (_IO_FILE *, wint_t) __THROW;
extern _IO_size_t _IO_wdefault_xsputn_internal (_IO_FILE *, const void *,
						_IO_size_t);
extern _IO_size_t _IO_wdefault_xsgetn_internal (_IO_FILE *, void *,
						_IO_size_t);
extern int _IO_wdefault_doallocate_internal (_IO_FILE *) __THROW;
extern wint_t _IO_wdefault_uflow_internal (_IO_FILE *);

extern int _IO_file_doallocate_internal (_IO_FILE *) __THROW;
extern _IO_FILE* _IO_file_setbuf_internal (_IO_FILE *, char *, _IO_ssize_t);
extern _IO_off64_t _IO_file_seekoff_internal (_IO_FILE *, _IO_off64_t,
					      int, int);
extern _IO_size_t _IO_file_xsputn_internal (_IO_FILE *, const void *,
					    _IO_size_t);
extern _IO_size_t _IO_file_xsgetn_internal (_IO_FILE *, void *, _IO_size_t);
extern int _IO_file_stat_internal (_IO_FILE *, void *) __THROW;
extern int _IO_file_close_internal (_IO_FILE *) __THROW;
extern int _IO_file_close_it_internal (_IO_FILE *);
extern int _IO_file_underflow_internal (_IO_FILE *);
extern int _IO_file_overflow_internal (_IO_FILE *, int);
extern void _IO_file_init_internal (struct _IO_FILE_plus *) __THROW;
extern _IO_FILE* _IO_file_attach_internal (_IO_FILE *, int);
extern _IO_FILE* _IO_file_fopen_internal (_IO_FILE *, const char *,
					  const char *, int);
extern _IO_ssize_t _IO_file_read_internal (_IO_FILE *, void *,
					   _IO_ssize_t);
extern int _IO_file_sync_internal (_IO_FILE *);
extern _IO_off64_t _IO_file_seek_internal (_IO_FILE *, _IO_off64_t, int)
     __THROW;
extern void _IO_file_finish_internal (_IO_FILE *, int);

extern _IO_size_t _IO_wfile_xsputn_internal (_IO_FILE *, const void *,
					     _IO_size_t);
extern _IO_off64_t _IO_wfile_seekoff_internal (_IO_FILE *, _IO_off64_t,
					       int, int);
extern wint_t _IO_wfile_sync_internal (_IO_FILE *);

extern int _IO_str_underflow_internal (_IO_FILE *) __THROW;
extern int _IO_str_overflow_internal (_IO_FILE *, int) __THROW;
extern int _IO_str_pbackfail_internal (_IO_FILE *, int) __THROW;
extern _IO_off64_t _IO_str_seekoff_internal (_IO_FILE *, _IO_off64_t,
					     int, int) __THROW;
extern void _IO_str_init_static_internal (struct _IO_strfile_ *, char *,
					  _IO_size_t, char *) __THROW;

extern struct _IO_jump_t _IO_file_jumps_internal attribute_hidden;
extern struct _IO_jump_t _IO_wfile_jumps_internal attribute_hidden;

extern struct _IO_FILE_plus *_IO_list_all_internal attribute_hidden;

extern void _IO_link_in_internal (struct _IO_FILE_plus *) __THROW;
extern int _IO_sputbackc_internal (_IO_FILE *, int) __THROW;
extern void _IO_wdoallocbuf_internal (_IO_FILE *) __THROW;

extern _IO_size_t _IO_sgetn_internal (_IO_FILE *, void *, _IO_size_t);
extern void _IO_flush_all_linebuffered_internal (void) __THROW;
extern int _IO_switch_to_wget_mode_internal (_IO_FILE *);
extern void _IO_unsave_markers_internal (_IO_FILE *) __THROW;
extern void _IO_switch_to_main_wget_area_internal (_IO_FILE *) __THROW;
extern int _IO_wdo_write_internal (_IO_FILE *, const wchar_t *, _IO_size_t);
extern int _IO_do_write_internal (_IO_FILE *, const char *, _IO_size_t);
extern _IO_ssize_t _IO_padn_internal (_IO_FILE *, int, _IO_ssize_t);
extern _IO_size_t _IO_getline_info_internal (_IO_FILE *,char *, _IO_size_t,
					     int, int, int *);
extern _IO_size_t _IO_getline_internal (_IO_FILE *, char *, _IO_size_t, int,
					int);
extern void _IO_free_wbackup_area_internal (_IO_FILE *) __THROW;
extern void _IO_free_backup_area_internal (_IO_FILE *) __THROW;
extern void _IO_switch_to_wbackup_area_internal (_IO_FILE *) __THROW;
extern void _IO_setb_internal (_IO_FILE *, char *, char *, int) __THROW;
extern wint_t _IO_sputbackwc_internal (_IO_FILE *, wint_t) __THROW;
extern int _IO_switch_to_get_mode_internal (_IO_FILE *);
extern int _IO_vfscanf_internal (_IO_FILE * __restrict,
				 const char * __restrict,
				 _IO_va_list, int *__restrict);
extern int _IO_vfprintf_internal (_IO_FILE *__restrict, const char *__restrict,
				  _IO_va_list);
extern void _IO_doallocbuf_internal (_IO_FILE *) __THROW;
extern void _IO_wsetb_internal (_IO_FILE *, wchar_t *, wchar_t *, int)
     __THROW;
extern _IO_off64_t _IO_seekoff_unlocked (_IO_FILE *, _IO_off64_t, int, int)
     attribute_hidden;
extern _IO_off64_t _IO_seekpos_unlocked (_IO_FILE *, _IO_off64_t, int)
     attribute_hidden;
extern int _IO_putc_internal (int __c, _IO_FILE *__fp);
extern void _IO_init_internal (_IO_FILE *, int) __THROW;
extern void _IO_un_link_internal (struct _IO_FILE_plus *) __THROW;

#ifndef EOF
# define EOF (-1)
#endif
#ifndef NULL
# if defined __GNUG__ && \
    (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
#  define NULL (__null)
# else
#  if !defined(__cplusplus)
#   define NULL ((void*)0)
#  else
#   define NULL (0)
#  endif
# endif
#endif

#if _G_HAVE_MMAP

# include <unistd.h>
# include <fcntl.h>
# include <sys/mman.h>
# include <sys/param.h>

# if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
# endif

# if !defined(MAP_ANONYMOUS) || !defined(EXEC_PAGESIZE)
#  undef _G_HAVE_MMAP
#  define _G_HAVE_MMAP 0
# endif

#endif /* _G_HAVE_MMAP */

#if _G_HAVE_MMAP

# ifdef _LIBC
/* When using this code in the GNU libc we must not pollute the name space.  */
#  define mmap __mmap
#  define munmap __munmap
#  define ftruncate __ftruncate
# endif

# define ROUND_TO_PAGE(_S) \
       (((_S) + EXEC_PAGESIZE - 1) & ~(EXEC_PAGESIZE - 1))

# define FREE_BUF(_B, _S) \
       munmap ((_B), ROUND_TO_PAGE (_S))
# define ALLOC_BUF(_B, _S, _R) \
       do {								      \
	  (_B) = (char *) mmap (0, ROUND_TO_PAGE (_S),			      \
				PROT_READ | PROT_WRITE,			      \
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);	      \
	  if ((_B) == (char *) MAP_FAILED)				      \
	    return (_R);						      \
       } while (0)
# define ALLOC_WBUF(_B, _S, _R) \
       do {								      \
	  (_B) = (wchar_t *) mmap (0, ROUND_TO_PAGE (_S),		      \
				   PROT_READ | PROT_WRITE,		      \
				   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);	      \
	  if ((_B) == (wchar_t *) MAP_FAILED)				      \
	    return (_R);						      \
       } while (0)

#else /* _G_HAVE_MMAP */

# define FREE_BUF(_B, _S) \
       free(_B)
# define ALLOC_BUF(_B, _S, _R) \
       do {								      \
	  (_B) = (char*)malloc(_S);					      \
	  if ((_B) == NULL)						      \
	    return (_R);						      \
       } while (0)
# define ALLOC_WBUF(_B, _S, _R) \
       do {								      \
	  (_B) = (wchar_t *)malloc(_S);					      \
	  if ((_B) == NULL)						      \
	    return (_R);						      \
       } while (0)

#endif /* _G_HAVE_MMAP */

#ifndef OS_FSTAT
# define OS_FSTAT fstat
#endif
extern int _IO_vscanf (const char *, _IO_va_list) __THROW;

/* _IO_pos_BAD is an _IO_off64_t value indicating error, unknown, or EOF. */
#ifndef _IO_pos_BAD
# define _IO_pos_BAD ((_IO_off64_t) -1)
#endif
/* _IO_pos_adjust adjust an _IO_off64_t by some number of bytes. */
#ifndef _IO_pos_adjust
# define _IO_pos_adjust(pos, delta) ((pos) += (delta))
#endif
/* _IO_pos_0 is an _IO_off64_t value indicating beginning of file. */
#ifndef _IO_pos_0
# define _IO_pos_0 ((_IO_off64_t) 0)
#endif

#ifdef __cplusplus
}
#endif

#ifdef _IO_MTSAFE_IO
/* check following! */
# ifdef _IO_USE_OLD_IO_FILE
#  define FILEBUF_LITERAL(CHAIN, FLAGS, FD, WDP) \
       { _IO_MAGIC+_IO_LINKED+_IO_IS_FILEBUF+FLAGS, \
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (_IO_FILE *) CHAIN, FD, \
	 0, _IO_pos_BAD, 0, 0, { 0 }, &_IO_stdfile_##FD##_lock }
# else
#  if defined _LIBC || defined _GLIBCPP_USE_WCHAR_T
#   define FILEBUF_LITERAL(CHAIN, FLAGS, FD, WDP) \
       { _IO_MAGIC+_IO_LINKED+_IO_IS_FILEBUF+FLAGS, \
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (_IO_FILE *) CHAIN, FD, \
	 0, _IO_pos_BAD, 0, 0, { 0 }, &_IO_stdfile_##FD##_lock, _IO_pos_BAD,\
	 NULL, WDP, 0 }
#  else
#   define FILEBUF_LITERAL(CHAIN, FLAGS, FD, WDP) \
       { _IO_MAGIC+_IO_LINKED+_IO_IS_FILEBUF+FLAGS, \
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (_IO_FILE *) CHAIN, FD, \
	 0, _IO_pos_BAD, 0, 0, { 0 }, &_IO_stdfile_##FD##_lock, _IO_pos_BAD,\
	 0 }
#  endif
# endif
#else
# ifdef _IO_USE_OLD_IO_FILE
#  define FILEBUF_LITERAL(CHAIN, FLAGS, FD, WDP) \
       { _IO_MAGIC+_IO_LINKED+_IO_IS_FILEBUF+FLAGS, \
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (_IO_FILE *) CHAIN, FD, \
	 0, _IO_pos_BAD }
# else
#  if defined _LIBC || defined _GLIBCPP_USE_WCHAR_T
#   define FILEBUF_LITERAL(CHAIN, FLAGS, FD, WDP) \
       { _IO_MAGIC+_IO_LINKED+_IO_IS_FILEBUF+FLAGS, \
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (_IO_FILE *) CHAIN, FD, \
	 0, _IO_pos_BAD, 0, 0, { 0 }, 0, _IO_pos_BAD, \
	 NULL, WDP, 0 }
#  else
#   define FILEBUF_LITERAL(CHAIN, FLAGS, FD, WDP) \
       { _IO_MAGIC+_IO_LINKED+_IO_IS_FILEBUF+FLAGS, \
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (_IO_FILE *) CHAIN, FD, \
	 0, _IO_pos_BAD, 0, 0, { 0 }, 0, _IO_pos_BAD, \
	 0 }
#  endif
# endif
#endif

/* VTABLE_LABEL defines NAME as of the CLASS class.
   CNLENGTH is strlen(#CLASS).  */
#ifdef __GNUC__
# if _G_VTABLE_LABEL_HAS_LENGTH
#  define VTABLE_LABEL(NAME, CLASS, CNLENGTH) \
  extern char NAME[] asm (_G_VTABLE_LABEL_PREFIX #CNLENGTH #CLASS);
# else
#  define VTABLE_LABEL(NAME, CLASS, CNLENGTH) \
  extern char NAME[] asm (_G_VTABLE_LABEL_PREFIX #CLASS);
# endif
#endif /* __GNUC__ */

#if !defined(builtinbuf_vtable) && defined(__cplusplus)
# ifdef __GNUC__
VTABLE_LABEL(builtinbuf_vtable, builtinbuf, 10)
# else
#  if _G_VTABLE_LABEL_HAS_LENGTH
#   define builtinbuf_vtable _G_VTABLE_LABEL_PREFIX_ID##10builtinbuf
#  else
#   define builtinbuf_vtable _G_VTABLE_LABEL_PREFIX_ID##builtinbuf
#  endif
# endif
#endif /* !defined(builtinbuf_vtable) && defined(__cplusplus) */

#if defined(__STDC__) || defined(__cplusplus)
# define _IO_va_start(args, last) va_start(args, last)
#else
# define _IO_va_start(args, last) va_start(args)
#endif

extern struct _IO_fake_stdiobuf _IO_stdin_buf, _IO_stdout_buf, _IO_stderr_buf;

#if 1
# define COERCE_FILE(FILE) /* Nothing */
#else
/* This is part of the kludge for binary compatibility with old stdio. */
# define COERCE_FILE(FILE) \
  (((FILE)->_IO_file_flags & _IO_MAGIC_MASK) == _OLD_MAGIC_MASK \
    && (FILE) = *(FILE**)&((int*)fp)[1])
#endif

#ifdef EINVAL
# define MAYBE_SET_EINVAL __set_errno (EINVAL)
#else
# define MAYBE_SET_EINVAL /* nothing */
#endif

#ifdef IO_DEBUG
# define CHECK_FILE(FILE, RET) \
	if ((FILE) == NULL) { MAYBE_SET_EINVAL; return RET; } \
	else { COERCE_FILE(FILE); \
	       if (((FILE)->_IO_file_flags & _IO_MAGIC_MASK) != _IO_MAGIC) \
	  { MAYBE_SET_EINVAL; return RET; }}
#else
# define CHECK_FILE(FILE, RET) COERCE_FILE (FILE)
#endif

static inline void
__attribute__ ((__always_inline__))
_IO_acquire_lock_fct (_IO_FILE **p)
{
  _IO_FILE *fp = *p;
  if ((fp->_flags & _IO_USER_LOCK) == 0)
    _IO_funlockfile (fp);
}

static inline void
__attribute__ ((__always_inline__))
_IO_acquire_lock_clear_flags2_fct (_IO_FILE **p)
{
  _IO_FILE *fp = *p;
  fp->_flags2 &= ~(_IO_FLAGS2_FORTIFY | _IO_FLAGS2_SCANF_STD);
  if ((fp->_flags & _IO_USER_LOCK) == 0)
    _IO_funlockfile (fp);
}
