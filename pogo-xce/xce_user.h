/********************************************************************
 * Cloud Engines XCE Linux Kernel Support Driver
 * Cloud Engines Confidential
 *
 * Copyright 2006-2010
 * All Rights Reserved
 */

/**
 * @file xce_user.h
 * @author Brad Dietrich
 * @brief Cloud Engines XCE Linux Kernel Support Driver
 */
#include <linux/types.h>

#define XCE_PROTO_VERSION 0x00000100U
#define XCE_MINOR         241
#define XCE_MMAP_NPAGES   64

union xce_dev_req {
  __u8  __raw[128];
};

struct xce_dev_uhdr {
  __u8  magic[48];
  __u32 version;
  __u32 nreqs;
  __u32 reqoff;
  __u32 req_next;
};

struct xce_dev_getblparams {
  unsigned int  bufmaxsize;
  unsigned int  outsize;
  unsigned long bufptr;
};

struct xce_dev_streamparams {
  unsigned int  flags;
  unsigned int  buf;
  unsigned int  bsize;
  unsigned int  bloc;
  int           srcfd;
  int           dstfd;
};

#define XCE_IOCTL_GETBLPARAMS      _IOR ('X', 1, struct xce_dev_getblparams)
#define XCE_IOCTL_STREAM           _IOWR('X', 2, struct xce_dev_streamparams)
