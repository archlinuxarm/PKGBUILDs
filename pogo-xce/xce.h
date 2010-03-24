/********************************************************************
 * Cloud Engines XCE Linux Kernel Support Driver
 * Cloud Engines Confidential
 *
 * Copyright 2006-2010
 * All Rights Reserved
 */

/**
 * @file xce.h
 * @author Brad Dietrich
 * @brief Cloud Engines XCE Linux Kernel Support Driver
 */
#include <linux/types.h>
#include <linux/mm.h>

#include "xce_user.h"
#include "xce_version.h"

#ifdef XCE_DO_DEBUG
#  define XCE_DEBUG KERN_INFO
#  define XCE_DLOG(f, a...) printk(XCE_DEBUG "XCE: " f, ##a)
#else
#  define XCE_DEBUG KERN_DEBUG
#  define XCE_DLOG(f, a...) do { ; } while(0)
#endif

struct xce_dev {
  atomic_t mapped;
  union {
    unsigned long apage;
    struct xce_dev_uhdr * uhdr;
  };
};

struct xce_led_info_s {
  int          lowgpio;
  unsigned int online;
  unsigned int msg;
  unsigned int offline;
  unsigned int error;
  unsigned int mask;
};

struct xce_board_info_s {
  const char * id;
  const struct xce_led_info_s * led;
  const void * extra;
};

const struct xce_board_info_s * xce_getboard(void);

int xce_dev_init(void);
void xce_dev_exit(void);

int xce_thread_start(void);
int xce_thread_stop(void);

int xce_nand_calculate_ecc_rs(const u_char *data, u_char *ecc_code);
int xce_nand_correct_data_rs(u_char *data, u_char *store_ecc);
