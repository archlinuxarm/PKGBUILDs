/* Copyright (C) 2003, 2004, 2006-2008, 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2003.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _LOWLEVELLOCK_H
#define _LOWLEVELLOCK_H	1

#include <time.h>
#include <sys/param.h>
#include <bits/pthreadtypes.h>
#include <ia64intrin.h>
#include <atomic.h>
#include <kernel-features.h>

#define __NR_futex		1230
#define FUTEX_WAIT		0
#define FUTEX_WAKE		1
#define FUTEX_REQUEUE		3
#define FUTEX_CMP_REQUEUE	4
#define FUTEX_WAKE_OP		5
#define FUTEX_OP_CLEAR_WAKE_IF_GT_ONE	((4 << 24) | 1)
#define FUTEX_LOCK_PI		6
#define FUTEX_UNLOCK_PI		7
#define FUTEX_TRYLOCK_PI	8
#define FUTEX_WAIT_BITSET	9
#define FUTEX_WAKE_BITSET	10
#define FUTEX_PRIVATE_FLAG	128
#define FUTEX_CLOCK_REALTIME	256

#define FUTEX_BITSET_MATCH_ANY	0xffffffff

/* Values for 'private' parameter of locking macros.  Yes, the
   definition seems to be backwards.  But it is not.  The bit will be
   reversed before passing to the system call.  */
#define LLL_PRIVATE	0
#define LLL_SHARED	FUTEX_PRIVATE_FLAG


#if !defined NOT_IN_libc || defined IS_IN_rtld
/* In libc.so or ld.so all futexes are private.  */
# ifdef __ASSUME_PRIVATE_FUTEX
#  define __lll_private_flag(fl, private) \
  ((fl) | FUTEX_PRIVATE_FLAG)
# else
#  define __lll_private_flag(fl, private) \
  ((fl) | THREAD_GETMEM (THREAD_SELF, header.private_futex))
# endif
#else
# ifdef __ASSUME_PRIVATE_FUTEX
#  define __lll_private_flag(fl, private) \
  (((fl) | FUTEX_PRIVATE_FLAG) ^ (private))
# else
#  define __lll_private_flag(fl, private) \
  (__builtin_constant_p (private)					      \
   ? ((private) == 0							      \
      ? ((fl) | THREAD_GETMEM (THREAD_SELF, header.private_futex))	      \
      : (fl))								      \
   : ((fl) | (((private) ^ FUTEX_PRIVATE_FLAG)				      \
	      & THREAD_GETMEM (THREAD_SELF, header.private_futex))))
# endif
#endif


/* Delay in spinlock loop.  */
#define BUSY_WAIT_NOP          asm ("hint @pause")

#define lll_futex_wait(futex, val, private) \
  lll_futex_timed_wait (futex, val, NULL, private)

#define lll_futex_timed_wait(ftx, val, timespec, private)		\
({									\
   DO_INLINE_SYSCALL(futex, 4, (long) (ftx),				\
		     __lll_private_flag (FUTEX_WAIT, private),		\
		     (int) (val), (long) (timespec));			\
   _r10 == -1 ? -_retval : _retval;					\
})

#define lll_futex_wake(ftx, nr, private)				\
({									\
   DO_INLINE_SYSCALL(futex, 3, (long) (ftx),				\
		     __lll_private_flag (FUTEX_WAKE, private),		\
		     (int) (nr));					\
   _r10 == -1 ? -_retval : _retval;					\
})

#define lll_robust_dead(futexv, private)				\
do									\
  {									\
    int *__futexp = &(futexv);						\
    atomic_or (__futexp, FUTEX_OWNER_DIED);				\
    DO_INLINE_SYSCALL(futex, 3, (long) __futexp,			\
		      __lll_private_flag (FUTEX_WAKE, private), 1);	\
  }									\
while (0)

/* Returns non-zero if error happened, zero if success.  */
#define lll_futex_requeue(ftx, nr_wake, nr_move, mutex, val, private)	     \
({									     \
   DO_INLINE_SYSCALL(futex, 6, (long) (ftx),				     \
		     __lll_private_flag (FUTEX_CMP_REQUEUE, private),	     \
		     (int) (nr_wake), (int) (nr_move), (long) (mutex),	     \
		     (int) val);					     \
   _r10 == -1;								     \
})

/* Returns non-zero if error happened, zero if success.  */
#define lll_futex_wake_unlock(ftx, nr_wake, nr_wake2, ftx2, private)	     \
({									     \
   DO_INLINE_SYSCALL(futex, 6, (long) (ftx),				     \
		     __lll_private_flag (FUTEX_WAKE_OP, private),	     \
		     (int) (nr_wake), (int) (nr_wake2), (long) (ftx2),	     \
		     FUTEX_OP_CLEAR_WAKE_IF_GT_ONE);			     \
   _r10 == -1;								     \
})


#define __lll_trylock(futex) \
  (atomic_compare_and_exchange_val_acq (futex, 1, 0) != 0)
#define lll_trylock(futex) __lll_trylock (&(futex))


#define __lll_robust_trylock(futex, id) \
  (atomic_compare_and_exchange_val_acq (futex, id, 0) != 0)
#define lll_robust_trylock(futex, id) \
  __lll_robust_trylock (&(futex), id)


#define __lll_cond_trylock(futex) \
  (atomic_compare_and_exchange_val_acq (futex, 2, 0) != 0)
#define lll_cond_trylock(futex) __lll_cond_trylock (&(futex))


extern void __lll_lock_wait_private (int *futex) attribute_hidden;
extern void __lll_lock_wait (int *futex, int private) attribute_hidden;
extern int __lll_robust_lock_wait (int *futex, int private) attribute_hidden;


#define __lll_lock(futex, private)					      \
  ((void) ({								      \
    int *__futex = (futex);						      \
    if (__builtin_expect (atomic_compare_and_exchange_bool_acq (__futex,      \
								1, 0), 0))    \
      {									      \
	if (__builtin_constant_p (private) && (private) == LLL_PRIVATE)	      \
	  __lll_lock_wait_private (__futex);				      \
	else								      \
	  __lll_lock_wait (__futex, private);				      \
      }									      \
  }))
#define lll_lock(futex, private) __lll_lock (&(futex), private)


#define __lll_robust_lock(futex, id, private)				      \
  ({									      \
    int *__futex = (futex);						      \
    int __val = 0;							      \
									      \
    if (__builtin_expect (atomic_compare_and_exchange_bool_acq (__futex, id,  \
								0), 0))	      \
      __val = __lll_robust_lock_wait (__futex, private);		      \
    __val;								      \
  })
#define lll_robust_lock(futex, id, private) \
  __lll_robust_lock (&(futex), id, private)


#define __lll_cond_lock(futex, private)					      \
  ((void) ({								      \
    int *__futex = (futex);						      \
    if (__builtin_expect (atomic_compare_and_exchange_bool_acq (__futex, 2,   \
								0), 0))	      \
      __lll_lock_wait (__futex, private);				      \
  }))
#define lll_cond_lock(futex, private) __lll_cond_lock (&(futex), private)


#define __lll_robust_cond_lock(futex, id, private)			      \
  ({									      \
    int *__futex = (futex);						      \
    int __val = 0;							      \
    int __id = (id) | FUTEX_WAITERS;					      \
									      \
    if (__builtin_expect (atomic_compare_and_exchange_bool_acq (__futex,      \
								__id, 0), 0)) \
      __val = __lll_robust_lock_wait (__futex, private);		      \
    __val;								      \
  })
#define lll_robust_cond_lock(futex, id, private) \
  __lll_robust_cond_lock (&(futex), id, private)


extern int __lll_timedlock_wait (int *futex, const struct timespec *,
				 int private) attribute_hidden;
extern int __lll_robust_timedlock_wait (int *futex, const struct timespec *,
					int private) attribute_hidden;


#define __lll_timedlock(futex, abstime, private)			      \
  ({									      \
     int *__futex = (futex);						      \
     int __val = 0;							      \
									      \
     if (__builtin_expect (atomic_compare_and_exchange_bool_acq (__futex, 1,  \
								 0), 0))      \
       __val = __lll_timedlock_wait (__futex, abstime, private);	      \
     __val;								      \
  })
#define lll_timedlock(futex, abstime, private) \
  __lll_timedlock (&(futex), abstime, private)


#define __lll_robust_timedlock(futex, abstime, id, private)		      \
  ({									      \
    int *__futex = (futex);						      \
    int __val = 0;							      \
									      \
    if (__builtin_expect (atomic_compare_and_exchange_bool_acq (__futex, id,  \
								0), 0))	      \
      __val = __lll_robust_timedlock_wait (__futex, abstime, private);	      \
    __val;								      \
  })
#define lll_robust_timedlock(futex, abstime, id, private) \
  __lll_robust_timedlock (&(futex), abstime, id, private)


#define __lll_unlock(futex, private)					      \
  ((void) ({								      \
    int *__futex = (futex);						      \
    int __val = atomic_exchange_rel (__futex, 0);			      \
									      \
    if (__builtin_expect (__val > 1, 0))				      \
      lll_futex_wake (__futex, 1, private);				      \
  }))
#define lll_unlock(futex, private) __lll_unlock(&(futex), private)


#define __lll_robust_unlock(futex, private)				      \
  ((void) ({								      \
    int *__futex = (futex);						      \
    int __val = atomic_exchange_rel (__futex, 0);			      \
									      \
    if (__builtin_expect (__val & FUTEX_WAITERS, 0))			      \
      lll_futex_wake (__futex, 1, private);				      \
  }))
#define lll_robust_unlock(futex, private) \
  __lll_robust_unlock(&(futex), private)


#define lll_islocked(futex) \
  (futex != 0)

/* Initializers for lock.  */
#define LLL_LOCK_INITIALIZER		(0)
#define LLL_LOCK_INITIALIZER_LOCKED	(1)

/* The kernel notifies a process with uses CLONE_CLEARTID via futex
   wakeup when the clone terminates.  The memory location contains the
   thread ID while the clone is running and is reset to zero
   afterwards.	*/
#define lll_wait_tid(tid) \
  do							\
    {							\
      __typeof (tid) __tid;				\
      while ((__tid = (tid)) != 0)			\
	lll_futex_wait (&(tid), __tid, LLL_SHARED);	\
    }							\
  while (0)

extern int __lll_timedwait_tid (int *, const struct timespec *)
     attribute_hidden;

#define lll_timedwait_tid(tid, abstime) \
  ({							\
    int __res = 0;					\
    if ((tid) != 0)					\
      __res = __lll_timedwait_tid (&(tid), (abstime));	\
    __res;						\
  })

#endif	/* lowlevellock.h */
