/********************************************************************
 * Cloud Engines XCE Linux Kernel Support Driver
 * Cloud Engines Confidential
 *
 * Copyright 2006-2010
 * All Rights Reserved
 */

/**
 * @file xce_thread.c
 * @author Brad Dietrich
 * @brief Cloud Engines XCE Linux Kernel Support Driver
 */
#include "xce.h"

#include <linux/module.h>
#include <linux/kthread.h>

static struct task_struct *xce_thread;
static DECLARE_WAIT_QUEUE_HEAD(xce_thread_wait);
static DECLARE_WAIT_QUEUE_HEAD(xce_thread_waitstop);
static volatile int xce_thread_todo;
static volatile int xce_thread_running;

static int xce_threadmain(void *);

int xce_thread_start(void)
{
  struct task_struct *tsk;
  int err = 0;
  //__module_get(THIS_MODULE);
  xce_thread_todo = 0;
  xce_thread_running = 1;
  tsk = kthread_run(xce_threadmain, NULL, "xce");
  if(IS_ERR(tsk)) {
    err = PTR_ERR(tsk);
    xce_thread_running = 0;
    //module_put(THIS_MODULE);
  }
  if(err==0) {
    xce_thread = tsk;
  }
  return err;
}

int xce_thread_stop(void)
{
  if(xce_thread!=NULL) {
    XCE_DLOG("Signalling thread to stop PID: %d\n", xce_thread->pid);
    xce_thread_todo = -1;
    wake_up(&xce_thread_wait);
    wait_event_interruptible(xce_thread_waitstop, xce_thread_running==0);
    xce_thread=NULL;
  }
  return xce_thread_running;
}

static int xce_threadmain(void * arg)
{
  XCE_DLOG("Kernel thread starting PID: %d\n", current->pid);
  for(;;) {
    wait_event_interruptible(xce_thread_wait, xce_thread_todo);
    XCE_DLOG("Kernel thread TODO: %d\n", xce_thread_todo);
    if(signal_pending(current)) {
      break;
    }
    if(xce_thread_todo<0) {
      break;
    }
    xce_thread_todo = 0;
  }
  XCE_DLOG("Kernel thread exiting PID: %d\n", current->pid);
  xce_thread_running = 0;
  module_put_and_exit(0);
}
