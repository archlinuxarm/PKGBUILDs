/* Copyright (C) 2001,2002,2004,2006,2007,2008 Free Software Foundation, Inc.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

static ucontext_t ctx[3];

static int was_in_f1;
static int was_in_f2;

static char st2[32768];

static void
f1 (int a0, int a1, int a2, int a3)
{
  printf ("start f1(a0=%x,a1=%x,a2=%x,a3=%x)\n", a0, a1, a2, a3);

  if (a0 != 1 || a1 != 2 || a2 != 3 || a3 != -4)
    {
      puts ("arg mismatch");
      exit (-1);
    }

  if (swapcontext (&ctx[1], &ctx[2]) != 0)
    {
      printf ("%s: swapcontext: %m\n", __FUNCTION__);
      exit (1);
    }
  puts ("finish f1");
  was_in_f1 = 1;
}

static void
f2 (void)
{
  char on_stack[1];

  puts ("start f2");

  printf ("&on_stack=%p\n", on_stack);
  if (on_stack < st2 || on_stack >= st2 + sizeof (st2))
    {
      printf ("%s: memory stack is not where it belongs!", __FUNCTION__);
      exit (1);
    }

  if (swapcontext (&ctx[2], &ctx[1]) != 0)
    {
      printf ("%s: swapcontext: %m\n", __FUNCTION__);
      exit (1);
    }
  puts ("finish f2");
  was_in_f2 = 1;
}

void
test_stack(volatile int a, volatile int b,
           volatile int c, volatile int d)
{
  volatile int e = 5;
  volatile int f = 6;
  ucontext_t uc;

  /* Test for cases where getcontext is clobbering the callers
     stack, including parameters.  */
  getcontext(&uc);

  if (a != 1)
    {
      printf ("%s: getcontext clobbers parm a\n", __FUNCTION__);
      exit (1);
    }

  if (b != 2)
    {
      printf ("%s: getcontext clobbers parm b\n", __FUNCTION__);
      exit (1);
    }

  if (c != 3)
    {
      printf ("%s: getcontext clobbers parm c\n", __FUNCTION__);
      exit (1);
    }

  if (d != 4)
    {
      printf ("%s: getcontext clobbers parm d\n", __FUNCTION__);
      exit (1);
    }

  if (e != 5)
    {
      printf ("%s: getcontext clobbers varible e\n", __FUNCTION__);
      exit (1);
    }

  if (f != 6)
    {
      printf ("%s: getcontext clobbers variable f\n", __FUNCTION__);
      exit (1);
    }
}

volatile int global;


static int back_in_main;


static void
check_called (void)
{
  if (back_in_main == 0)
    {
      puts ("program did no reach main again");
      _exit (1);
    }
}


int
main (void)
{
  atexit (check_called);

  char st1[32768];

  puts ("making contexts");
  if (getcontext (&ctx[1]) != 0)
    {
      if (errno == ENOSYS)
	{
	  back_in_main = 1;
	  exit (0);
	}

      printf ("%s: getcontext: %m\n", __FUNCTION__);
      exit (1);
    }

  test_stack (1, 2, 3, 4);

  /* Play some tricks with this context.  */
  if (++global == 1)
    if (setcontext (&ctx[1]) != 0)
      {
	printf ("%s: setcontext: %m\n", __FUNCTION__);
	exit (1);
      }
  if (global != 2)
    {
      printf ("%s: 'global' not incremented twice\n", __FUNCTION__);
      exit (1);
    }

  ctx[1].uc_stack.ss_sp = st1;
  ctx[1].uc_stack.ss_size = sizeof st1;
  ctx[1].uc_link = &ctx[0];
  {
    ucontext_t tempctx = ctx[1];
    makecontext (&ctx[1], (void (*) (void)) f1, 4, 1, 2, 3, -4);

    /* Without this check, a stub makecontext can make us spin forever.  */
    if (memcmp (&tempctx, &ctx[1], sizeof ctx[1]) == 0)
      {
	puts ("makecontext was a no-op, presuming not implemented");
	return 0;
      }
  }

  if (getcontext (&ctx[2]) != 0)
    {
      printf ("%s: second getcontext: %m\n", __FUNCTION__);
      exit (1);
    }
  ctx[2].uc_stack.ss_sp = st2;
  ctx[2].uc_stack.ss_size = sizeof st2;
  ctx[2].uc_link = &ctx[1];
  makecontext (&ctx[2], f2, 0);

  puts ("swapping contexts");
  if (swapcontext (&ctx[0], &ctx[2]) != 0)
    {
      printf ("%s: swapcontext: %m\n", __FUNCTION__);
      exit (1);
    }
  puts ("back at main program");
  back_in_main = 1;

  if (was_in_f1 == 0)
    {
      puts ("didn't reach f1");
      exit (1);
    }
  if (was_in_f2 == 0)
    {
      puts ("didn't reach f2");
      exit (1);
    }

  puts ("test succeeded");
  return 0;
}
