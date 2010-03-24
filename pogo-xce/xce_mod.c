/********************************************************************
 * Cloud Engines XCE Linux Kernel Support Driver
 * Cloud Engines Confidential
 *
 * Copyright 2006-2010
 * All Rights Reserved
 */

/**
 * @file xce_mod.c
 * @author Brad Dietrich
 * @brief Cloud Engines XCE Linux Kernel Support Driver
 */
#include "xce.h"

#include <linux/module.h>

MODULE_AUTHOR("Brad Dietrich <brad@cloudengines.com>");
MODULE_DESCRIPTION("Cloud Engines Acceleration");
MODULE_VERSION(XCE_VERSION);
MODULE_LICENSE("GPL");

#if defined(__arm__)
#ifdef CONFIG_ARCH_KIRKWOOD
#include <mach/kirkwood.h>
#ifndef KIRKWOOD_REGS_VIRT_BASE
#  define KIRKWOOD_REGS_VIRT_BASE 0xf1000000
#endif
#else /*!CONFIG_ARCH_KIRKWOOD*/
#  define KIRKWOOD_REGS_VIRT_BASE 0xf1000000
#endif
#endif

static int __init xce_init(void)
{
  printk("Cloud Engines XCE Init [Version: %s]\n", XCE_VERSION);

#if defined(__arm__)
  XCE_DLOG("CPU MEMORY MAP:\n");
  XCE_DLOG("  -- 0x%08lx - 0x%08lx (%4lu MB)  User Space Mappings\n",
           0x1000UL, TASK_SIZE-1, (TASK_SIZE-0x1000UL) / 1024 / 1024);
  XCE_DLOG("  -- 0x%08lx - 0x%08lx (%4lu MB)  Kernel module space\n",
           TASK_SIZE, PAGE_OFFSET-1, (PAGE_OFFSET-TASK_SIZE) / 1024 / 1024);
  XCE_DLOG("  -- 0x%08lx - 0x%08lx (%4lu MB)  Kernel direct-mapped ram\n", 
           PAGE_OFFSET, (unsigned long)high_memory-1, 
           ((unsigned long)high_memory - PAGE_OFFSET) / 1024 / 1024);
  XCE_DLOG("  -- 0x%08lx - 0x%08lx (%4lu MB)  Kernel vmalloc space\n",
           (unsigned long)VMALLOC_START, (unsigned long)VMALLOC_END-1, 
           ((unsigned long)VMALLOC_END-VMALLOC_START) / 1024 / 1024);
  XCE_DLOG("  -- 0x%08lx - 0x%08lx (%4lu MB)  Kernel platform space\n",
           (unsigned long)VMALLOC_END, 0xfeffffffUL, (0xfeffffffUL-VMALLOC_END) / 1024 / 1024);
  XCE_DLOG("CPU FEATURES:\n");

  /* L2Cache settings */
  {
    volatile unsigned int temp;
    static volatile unsigned int * const __cpu_config    = (unsigned int *)(KIRKWOOD_REGS_VIRT_BASE|0x20100);
    static volatile unsigned int * const __cpu_l2_config = (unsigned int *)(KIRKWOOD_REGS_VIRT_BASE|0x20128);
    volatile unsigned int cpuconfig = *__cpu_config;
    volatile unsigned int l2config = *__cpu_l2_config;
    __asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0":"=r" (temp));
    XCE_DLOG("  -- I Cache:         %s\n", (temp & CR_I)?"enabled":"disabled");
    XCE_DLOG("  -- D Cache:         %s\n", (temp & CR_C)?"enabled":"disabled");
    XCE_DLOG("  -- Branch Predict:  %s\n", (temp & CR_Z)?"disabled":"enabled");
    XCE_DLOG("  -- MMU:             %s\n", (temp & CR_M)?"enabled":"disabled");
    XCE_DLOG("  -- Alignment Abort: %s\n", (temp & CR_A)?"enabled":"disabled");
    XCE_DLOG("  -- ICache Prefetch: %s\n", (cpuconfig & (1<<16))?"enabled":"disabled");
    XCE_DLOG("  -- DCache Prefetch: %s\n", (cpuconfig & (1<<17))?"enabled":"disabled");
    __asm__ __volatile__ ("mrc p15, 1, %0, c15, c1, 0":"=r" (temp));
    XCE_DLOG("  -- L2 Cache:        %s\n", (temp & (1<<22))?"enabled":"disabled");
    XCE_DLOG("  -- L2 Prefetch:     %s\n", (temp & (1<<24))?"disabled":"enabled");
    XCE_DLOG("  -- L2 Writethrough: %s\n", (l2config & (1<<4))?"enabled":"disabled");
    XCE_DLOG("  -- Write Allocate:  %s\n", (temp & (1<<28))?"enabled":"disabled");
    XCE_DLOG("  -- Streaming:       %s\n", (temp & (1<<29))?"enabled":"disabled");
  }
#endif

  xce_dev_init();
  xce_thread_start();
  return 0;
}

static void __exit xce_exit(void)
{
  printk("Cloud Engines XCE Exit [Version: %s]\n", XCE_VERSION);
  xce_thread_stop();
  xce_dev_exit();
}

module_init(xce_init);
module_exit(xce_exit);
