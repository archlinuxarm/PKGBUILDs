/* Definitions for thread-local data handling.  NPTL/sparc version.
   Copyright (C) 2003, 2005, 2006, 2007 Free Software Foundation, Inc.
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

#ifndef _TLS_H
#define _TLS_H

#include <dl-sysdep.h>
#ifndef __ASSEMBLER__
# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include <stdlib.h>
# include <list.h>
# include <kernel-features.h>

/* Type for the dtv.  */
typedef union dtv
{
  size_t counter;
  struct
  {
    void *val;
    bool is_static;
  } pointer;
} dtv_t;

typedef struct
{
  void *tcb;		/* Pointer to the TCB.  Not necessary the
			   thread descriptor used by libpthread.  */
  dtv_t *dtv;
  void *self;
  int multiple_threads;
#if __WORDSIZE == 64
  int gscope_flag;
#endif
  uintptr_t sysinfo;
  uintptr_t stack_guard;
  uintptr_t pointer_guard;
#if __WORDSIZE != 64
  int gscope_flag;
#endif
#ifndef __ASSUME_PRIVATE_FUTEX
  int private_futex;
#endif
} tcbhead_t;

#else /* __ASSEMBLER__ */
# include <tcb-offsets.h>
#endif /* __ASSEMBLER__ */

/* We require TLS support in the tools.  */
#ifndef HAVE_TLS_SUPPORT
# error "TLS support is required."
#endif

#ifndef __ASSEMBLER__
/* Get system call information.  */
# include <sysdep.h>

/* Get the thread descriptor definition.  */
# include <nptl/descr.h>

register struct pthread *__thread_self __asm__("%g7");

/* This is the size of the initial TCB.  Can't be just sizeof (tcbhead_t),
   because NPTL getpid, __libc_alloca_cutoff etc. need (almost) the whole
   struct pthread even when not linked with -lpthread.  */
# define TLS_INIT_TCB_SIZE sizeof (struct pthread)

/* Alignment requirements for the initial TCB.  */
# define TLS_INIT_TCB_ALIGN __alignof__ (struct pthread)

/* This is the size of the TCB.  */
# define TLS_TCB_SIZE sizeof (struct pthread)

/* Alignment requirements for the TCB.  */
# define TLS_TCB_ALIGN __alignof__ (struct pthread)

/* The TCB can have any size and the memory following the address the
   thread pointer points to is unspecified.  Allocate the TCB there.  */
# define TLS_TCB_AT_TP	1

/* Install the dtv pointer.  The pointer passed is to the element with
   index -1 which contain the length.  */
# define INSTALL_DTV(descr, dtvp) \
  ((tcbhead_t *) (descr))->dtv = (dtvp) + 1

/* Install new dtv for current thread.  */
# define INSTALL_NEW_DTV(DTV) \
  (((tcbhead_t *) __thread_self)->dtv = (DTV))

/* Return dtv of given thread descriptor.  */
# define GET_DTV(descr) \
  (((tcbhead_t *) (descr))->dtv)

/* Code to initially initialize the thread pointer.  */
# define TLS_INIT_TP(descr, secondcall) \
  (__thread_self = (__typeof (__thread_self)) (descr), NULL)

/* Return the address of the dtv for the current thread.  */
# define THREAD_DTV() \
  (((tcbhead_t *) __thread_self)->dtv)

/* Return the thread descriptor for the current thread.  */
#define THREAD_SELF  __thread_self

/* Magic for libthread_db to know how to do THREAD_SELF.  */
# define DB_THREAD_SELF_INCLUDE <sys/ucontext.h>
# define DB_THREAD_SELF \
  REGISTER (32, 32, REG_G7 * 4, 0) REGISTER (64, 64, REG_G7 * 8, 0)

/* Access to data in the thread descriptor is easy.  */
#define THREAD_GETMEM(descr, member) \
  descr->member
#define THREAD_GETMEM_NC(descr, member, idx) \
  descr->member[idx]
#define THREAD_SETMEM(descr, member, value) \
  descr->member = (value)
#define THREAD_SETMEM_NC(descr, member, idx, value) \
  descr->member[idx] = (value)

/* Set the stack guard field in TCB head.  */
#define THREAD_SET_STACK_GUARD(value) \
  THREAD_SETMEM (THREAD_SELF, header.stack_guard, value)
# define THREAD_COPY_STACK_GUARD(descr) \
  ((descr)->header.stack_guard \
   = THREAD_GETMEM (THREAD_SELF, header.stack_guard))

/* Get/set the stack guard field in TCB head.  */
#define THREAD_GET_POINTER_GUARD() \
  THREAD_GETMEM (THREAD_SELF, header.pointer_guard)
#define THREAD_SET_POINTER_GUARD(value) \
  THREAD_SETMEM (THREAD_SELF, header.pointer_guard, value)
# define THREAD_COPY_POINTER_GUARD(descr) \
  ((descr)->header.pointer_guard = THREAD_GET_POINTER_GUARD ())

/* Get and set the global scope generation counter in struct pthread.  */
#define THREAD_GSCOPE_FLAG_UNUSED 0
#define THREAD_GSCOPE_FLAG_USED   1
#define THREAD_GSCOPE_FLAG_WAIT   2
#define THREAD_GSCOPE_RESET_FLAG() \
  do									     \
    { int __res								     \
	= atomic_exchange_rel (&THREAD_SELF->header.gscope_flag,	     \
			       THREAD_GSCOPE_FLAG_UNUSED);		     \
      if (__res == THREAD_GSCOPE_FLAG_WAIT)				     \
	lll_futex_wake (&THREAD_SELF->header.gscope_flag, 1, LLL_PRIVATE);   \
    }									     \
  while (0)
#define THREAD_GSCOPE_SET_FLAG() \
  do									     \
    {									     \
      THREAD_SELF->header.gscope_flag = THREAD_GSCOPE_FLAG_USED;	     \
      atomic_write_barrier ();						     \
    }									     \
  while (0)
#define THREAD_GSCOPE_WAIT() \
  GL(dl_wait_lookup_done) ()

#endif /* !ASSEMBLER */

#endif	/* tls.h */
