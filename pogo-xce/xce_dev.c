/********************************************************************
 * Cloud Engines XCE Linux Kernel Support Driver
 * Cloud Engines Confidential
 *
 * Copyright 2006-2010
 * All Rights Reserved
 */

/**
 * @file xce_dev.c
 * @author Brad Dietrich
 * @brief Cloud Engines XCE Linux Kernel Support Driver
 */
#include "xce.h"

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/pagemap.h>

#include <linux/mtd/mtd.h>

#ifdef MODULE_ALIAS_MISCDEV
MODULE_ALIAS_MISCDEV(XCE_MINOR);
#endif

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

//NOTE: Beta boards only had one LED
static const struct xce_led_info_s __xceled_pogoplug_beta = {
  0, 0x00020000, 0x00020000, 0x00000000, 0x00020000, 0x00020000
};
// NOTE: Production v1 boards have 4 LEDs
static const struct xce_led_info_s __xceled_pogoplug_v1 = {
  0, 0x00008000, 0x00010000, 0x00004000, 0x00020000, 0x0003c000
};
// NOTE: Production v2 boards have 2
static const struct xce_led_info_s __xceled_pogoplug_v2 = {
  0, 0x00010000, 0x00030000, 0x00020000, 0x00030000, 0x00030000
};
// NOTE: Redstone boards have 2
static const struct xce_led_info_s __xceled_redstone = {
  0, 0x00004000, 0x0000C000, 0x00008000, 0x0000C000, 0x0000c000
};
// NOTE: Redstone 6280 boards have 2
static const struct xce_led_info_s __xceled_redstone6280 = {
  1, 0x00000100, 0x00000300, 0x00000200, 0x00000300, 0x00000300
};

static const struct xce_board_info_s xce_board_infos[] = {
  { "PP_GTI:2.0",       &__xceled_pogoplug_beta,  NULL },
  { "PP_GTI:3.0",       &__xceled_pogoplug_v1,    NULL },
  { "PPV2_EVT",         &__xceled_pogoplug_v2,    NULL },
  { "PPV2",             &__xceled_pogoplug_v2,    NULL },
  { "REDSTONE:0.1",     &__xceled_redstone,       NULL },
  { "REDSTONE:1.0",     &__xceled_redstone,       NULL },
  { "REDSTONE6280_DVT", &__xceled_redstone6280,   NULL },
  { NULL, NULL}
};

#if defined(__arm__)
static char * g_xce_boardid;
/* static const struct xce_board_info_s * g_xce_board; */
const struct xce_board_info_s * g_xce_board;
static const struct xce_led_info_s * g_xce_led;
#endif

#define LED_MESSAGE       0x0001
#define LED_CONNECTED     0x0002
#define LED_DISCONNECTED  0x0004
#define LED_ERROR         0x0008

#define DEFAULT_BLPARAM_OFFSET 0xa0000

static ssize_t xce_dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t xce_dev_write(struct file *, const char __user *, size_t, loff_t *);
static int xce_dev_open(struct inode *, struct file *);
static int xce_dev_release(struct inode *, struct file *);
static int xce_dev_ioctl(struct inode * i, struct file * f, unsigned int r, unsigned long p);
static int xce_dev_mmap(struct file *, struct vm_area_struct *);
static unsigned int xce_dev_poll(struct file *, struct poll_table_struct *);

static void xce_mm_open(struct vm_area_struct *vma);
static void xce_mm_close(struct vm_area_struct *vma);
static unsigned char * xce_dev_loadprops(struct xce_dev_getblparams * blparams);
#if defined(__arm__)
static void xce_dev_led(int ledmode);
#endif

#define xce_dev_dumpGPIO()

#if defined(__arm__)
static void xce_dev_dumpGPIO__(void);
static volatile unsigned int * const __cpu_rst_mask = (unsigned int *)(KIRKWOOD_REGS_VIRT_BASE|0x20108);
static volatile unsigned int * const __cpu_rst      = (unsigned int *)(KIRKWOOD_REGS_VIRT_BASE|0x2010c);

static volatile unsigned int * const __wdog_ctrl    = (unsigned int *)(KIRKWOOD_REGS_VIRT_BASE|0x20300);
static volatile unsigned int * const __wdog         = (unsigned int *)(KIRKWOOD_REGS_VIRT_BASE|0x20324);

static volatile unsigned int * const __gpio   = (unsigned int *)(KIRKWOOD_REGS_VIRT_BASE|0x10100);
static volatile unsigned int * const __gpio_h = (unsigned int *)(KIRKWOOD_REGS_VIRT_BASE|0x10140);
static volatile unsigned int *       __led_gpio;
#endif

static struct file_operations xce_dev_operations = {
  .owner   = THIS_MODULE,
  .llseek  = no_llseek,
  .read    = xce_dev_read,
  .write   = xce_dev_write,
  .open    = xce_dev_open,
  .release = xce_dev_release,
  .ioctl   = xce_dev_ioctl,
  .mmap    = xce_dev_mmap,
  .poll    = xce_dev_poll,
};

static struct miscdevice xce_miscdevice = {
  .minor = XCE_MINOR,
  .name  = "xce",
  .fops  = &xce_dev_operations,
};

static struct vm_operations_struct xce_mmap_ops = {
  .open  = xce_mm_open,
  .close = xce_mm_close,
};

int __init xce_dev_init(void)
{
  int err = -ENOMEM;

  err = misc_register(&xce_miscdevice);

#if defined(__arm__)
  {
    struct xce_dev_getblparams blp;
    char * buf = NULL;

    memset(&blp, 0, sizeof(blp));
    if((buf = xce_dev_loadprops(&blp))!=NULL) {
      char * thisline;
      char * nextline;
      size_t blsize;

      // We loaded proeprties, now look for the ones we care about here...
      //memcpy(&blsize, buf, 4);
      blsize = blp.outsize;

      XCE_DLOG("Loaded Property Size: %u\n", blsize);

      thisline = buf+4;
      while(thisline-buf < blsize && *thisline != '\0') {
        char * eq = NULL, * key = NULL, * val = NULL;
        nextline = thisline+strlen(thisline)+1;
        eq = strchr(thisline, '=');
        if(eq != NULL) {
          key = thisline;
          *eq = '\0';
          val = eq + 1;
          if(key[0]=='c' && key[1]=='e') {
            XCE_DLOG("   - '%s' -> '%s'\n", key, val);
            if(strcmp(key, "ceboardver")==0) {
              g_xce_boardid = kmalloc(strlen(val)+1, GFP_KERNEL);
              strcpy(g_xce_boardid, val);
            }
          }
        } 
        thisline = nextline;
      }

      kfree(buf);
    }
  }

  if(g_xce_boardid!=NULL) {
    int i;
    for(i=0;xce_board_infos[i].id!=NULL;i++) {
      if(strcmp(g_xce_boardid, xce_board_infos[i].id)==0) {
        g_xce_board = &xce_board_infos[i];
        g_xce_led = xce_board_infos[i].led;
        goto foundone;
      }
    }
  }
  printk(KERN_WARNING "XCE: No board spec found matching boardid '%s' -- Using '%s'\n", 
         g_xce_boardid, xce_board_infos[0].id);
  g_xce_led = xce_board_infos[0].led;

 foundone:

  // Initialize our GPIO pins to a known state
  if(g_xce_led->lowgpio) {
    __led_gpio = __gpio;
  } else {
    __led_gpio = __gpio_h;
  }
  //   make them all output pins
  __led_gpio[1] = __led_gpio[1] & ~(g_xce_led->mask);
  //   disable any blinking state
  __led_gpio[2] = __led_gpio[2] & ~(g_xce_led->mask);
  //   start with them all high (i.e. LED off)
  __led_gpio[0] = __led_gpio[0] | (g_xce_led->mask);

  xce_dev_dumpGPIO__();

#endif // arm

  return err;
}

void xce_dev_exit(void)
{
#if defined(__arm__)
  // Deinitialize our GPIO pins to a known state
  //   disable any blinking state
  __led_gpio[2] = __led_gpio[2] & ~(g_xce_led->mask);
  //   reset them all hight (i.e. LED off)
  __led_gpio[0] = __led_gpio[0] | (g_xce_led->mask);

  xce_dev_dumpGPIO__();
#endif // arm

  misc_deregister(&xce_miscdevice);
}

/* static void hexdump(const void * data, int size, int offset) */
/* { */
/*   const unsigned char * cd = (const unsigned char *)data; */
/*   int i, j; */
/*   char linebuf[128]; */
/*   char *lb; */

/*   for(i=0; i<size; i+=16) { */
/*     lb = linebuf; */
/*     sprintf(lb, "%08x", offset+i); */
/*     //brultostr(offset+i, lb, 8, 16, '0'); */
/*     lb[8]=':'; */
/*     lb[9]=' '; */
/*     lb+=10; */
/*     for(j=0; j<16; j++) { */
/*       if(i+j >= size) */
/*         break; */
/*       sprintf(lb, "%02x", cd[i+j]); */
/*       //brultostr(cd[i+j], lb, 2, 16, '0'); */
/*       lb[2]=' '; */
/*       lb+=3; */
/*     } */
/*     if(i+j >= size) { */
/*       for(; j<16; j++) { */
/*         lb[0]=' '; */
/*         lb[1]=' '; */
/*         lb[2]=' '; */
/*         lb+=3; */
/*       } */
/*     } */
/*     lb[0]=' '; */
/*     lb[1]=' '; */
/*     lb+=2; */
/*     for(j=0; j<16; j++) { */
/*       if(i+j >= size) */
/*         break; */
/*       if(cd[i+j]=='%') { */
/*         lb[0] = '.'; */
/*       } else if(cd[i+j]>=0x20 && cd[i+j]<=0x7e) { */
/*         lb[0] = cd[i+j]; */
/*       } else { */
/*         lb[0] = '.'; */
/*       } */
/*       lb++; */
/*     } */
/*     lb[0]='\n'; */
/*     lb[1]='\0'; */
/*     XCE_DLOG("%s", linebuf); */
/*   } */
/* } */

#if defined(__arm__)
static void xce_dev_dumpGPIO__(void)
{
  XCE_DLOG("Current GPIO State:\n");
  XCE_DLOG(" GPIO L OUT:    0x%08x\n", __gpio[0]);
  XCE_DLOG(" GPIO L OE:     0x%08x\n", __gpio[1]);
  XCE_DLOG(" GPIO L BLINK:  0x%08x\n", __gpio[2]);
  XCE_DLOG(" GPIO L POL:    0x%08x\n", __gpio[3]);
  XCE_DLOG(" GPIO L IN:     0x%08x\n", __gpio[4]);
  XCE_DLOG(" GPIO H OUT:    0x%08x\n", __gpio_h[0]);
  XCE_DLOG(" GPIO H OE:     0x%08x\n", __gpio_h[1]);
  XCE_DLOG(" GPIO H BLINK:  0x%08x\n", __gpio_h[2]);
  XCE_DLOG(" GPIO H POL:    0x%08x\n", __gpio_h[3]);
  XCE_DLOG(" GPIO H IN:     0x%08x\n", __gpio_h[4]);
}
#endif

static unsigned char * xce_dev_loadprops(struct xce_dev_getblparams * blparams)
{
  unsigned char * buf = NULL;
  struct mtd_info * mtd = NULL;
  int ret;

  //XCE_DLOG("BLPARAMS:   -- Loading properties [%p].\n", blparams);

  mtd = get_mtd_device(NULL, 0);
  if(mtd!=NULL) {
    // OK, let's attempt to get the bootloader parameters:
    struct mtd_oob_ops ops;
    size_t len          = mtd->writesize;
    loff_t ppos         = DEFAULT_BLPARAM_OFFSET;
    struct xce_dev_getblparams __blp;
    /*       unsigned char oobcalc[mtd->oobsize]; */
    int eccerrs;
    buf = kmalloc(len + mtd->oobsize, GFP_KERNEL);
    if(blparams==NULL) {
      memset(&__blp, 0, sizeof(__blp));
      blparams = &__blp;
    }
    if(blparams->bufmaxsize == 0) {
      blparams->bufmaxsize = len;
    }

    while(blparams->bufmaxsize>0) {
      memset(&ops, 0, sizeof(ops));
      ops.mode   = MTD_OOB_RAW;
      ops.len    = len;
      ops.datbuf = buf;
      ops.oobbuf = buf+mtd->writesize;
      ops.ooblen = mtd->oobsize;
            
      XCE_DLOG("BLPARAMS: reading %d bytes @ %llx\n", len, ppos);
      ret = mtd->read_oob(mtd, ppos, &ops);
      if(ret>=0) {
        // OK, let's correct any data ECC finds wrong
        eccerrs = xce_nand_correct_data_rs(buf, ops.oobbuf+24);
        if(eccerrs==0) {
        } else if(eccerrs>0) {
          XCE_DLOG("BLPARAMS:   -- Found and corrected %d bit errors.\n", eccerrs);
        } else {
          XCE_DLOG("BLPARAMS:   -- Too many bit errors -- Can't correct!\n");
          ret = -EIO;
          break;
        }

        if(blparams->bufptr!=0) {
          if((ret=copy_to_user((char*)blparams->bufptr+blparams->outsize,
                               buf, ops.retlen))<0) {
            break;
          }
        }
        blparams->outsize += mtd->writesize;
        blparams->bufmaxsize -= ops.retlen;
        ppos += mtd->writesize;
      } else {
        break;
      }
    }

    put_mtd_device(mtd);
  } else {
    ret = -ENOENT;
  }
  return buf;
}

static ssize_t xce_dev_read(struct file * f, char __user * buf, size_t sz, loff_t * off)
{
  XCE_DLOG("Device read %zu bytes by PID: %d\n", sz, current->pid);
  return -EIO;
}

#if defined(__arm__)
static void xce_dev_led(int ledmode)
{
  switch(ledmode) {
  case LED_MESSAGE:
    XCE_DLOG("XCE: LED -> MESSAGE\n");
    __led_gpio[2] = (__led_gpio[2]) |  g_xce_led->msg;
    break;
  case LED_CONNECTED:
    XCE_DLOG("XCE: LED -> CONNECTED\n");
    __led_gpio[0] = (__led_gpio[0]) & ~g_xce_led->msg;
    __led_gpio[2] = (__led_gpio[2]) & ~g_xce_led->msg;
    __led_gpio[0] = (__led_gpio[0]) |  g_xce_led->offline;
    __led_gpio[2] = (__led_gpio[2]) & ~g_xce_led->offline;
    __led_gpio[0] = (__led_gpio[0]) & ~g_xce_led->online;
    __led_gpio[2] = (__led_gpio[2]) & ~g_xce_led->online;
    break;
  case LED_DISCONNECTED:
    XCE_DLOG("XCE: LED -> DISCONNECTED\n");
    __led_gpio[0] = (__led_gpio[0]) |  g_xce_led->online;
    __led_gpio[2] = (__led_gpio[2]) & ~g_xce_led->online;
    __led_gpio[0] = (__led_gpio[0]) & ~g_xce_led->offline;
    __led_gpio[2] = (__led_gpio[2]) & ~g_xce_led->offline;
    break;
  case LED_ERROR:
    XCE_DLOG("XCE: LED -> ERROR\n");
    __led_gpio[0] = (__led_gpio[0]) & ~g_xce_led->error;
    __led_gpio[2] = (__led_gpio[2]) &  g_xce_led->error;
    break;
  }
}
#endif

static ssize_t xce_dev_write(struct file * f, const char __user * buf, size_t sz, loff_t * off)
{
  int err = -EIO;
  struct page *pg = NULL;
  unsigned long poff;
  char * mapped = NULL;
  char * ploc = NULL;
  char sbuf[128];
/*   XCE_DLOG("Device write %p -- %z bytes by PID: %d\n", buf, sz, current->pid); */

  if(sz<128) {
    err = copy_from_user(sbuf, buf, sz);
    if(err < 0)
      goto bail;
    sbuf[sz]='\0';
    ploc = sbuf;
  } else {
    err = get_user_pages(current, current->mm, (unsigned long)buf, 1, 0, 0, &pg, NULL);
    if(err < 0)
      goto bail;
    poff = ((unsigned long)buf) % PAGE_SIZE;
    mapped = kmap_atomic(pg, KM_USER0);
    ploc = mapped+poff;
/*   XCE_DLOG("Device write mapped to %p in kernel memory\n", ploc); */
  }
  //XCE_DLOG("   -- '%.*s'\n", ((int)sz), ploc);
  err = sz;

#if defined(__arm__)
  // ARM should give still user-mapped address space (avoids cache pollution -- CPU cache, not TLB cache)
  // XCE_DLOG("Attempt to read user address: %p\n", buf);
  // XCE_DLOG("   -- '%.*s'\n", sz, buf);

  if(sz>=4) {

    // TODO: this is a little too hard-coded right now...
    if(strncmp(ploc, "led=", 4)==0) {
      sz-=4;
      ploc+=4;
      if(sz>=3 && strncmp(ploc, "msg", 3)==0) {
        xce_dev_led(LED_MESSAGE);
      } else if(sz>=3 && strncmp(ploc, "con", 3)==0) {
        xce_dev_led(LED_CONNECTED);
      } else if(sz>=3 && strncmp(ploc, "dis", 3)==0) {
        xce_dev_led(LED_DISCONNECTED);
      } else if(sz>=3 && strncmp(ploc, "err", 3)==0) {
        xce_dev_led(LED_ERROR);
      }
    } else if(sz>=6 && strncmp(ploc, "led0=", 5)==0) {
      // We need to simulate the old behaviour for now...
      if(ploc[5]=='1') {
        // This really meant connected...
        xce_dev_led(LED_CONNECTED);
      } else if(ploc[5]=='0') {
        // This really meant disconnected...
        xce_dev_led(LED_DISCONNECTED);
      } else if(ploc[5]=='b') {
        // This really meant message wating
        xce_dev_led(LED_MESSAGE);
      }
      // Dump the current state
      xce_dev_dumpGPIO();
    } else
    if(strncmp(ploc, "hardreset", 9)==0) {
      XCE_DLOG("   -- Performing hard reset...\n");

      *__cpu_rst_mask = *__cpu_rst_mask | 0x04;
      *__cpu_rst      = 0x01;

      // This will never return -- we just kicked the board hard!
      
    } else if(strncmp(ploc, "wdog", 4)==0) {
      if(sz>=8 && strncmp(ploc, "wdog=get", 8)==0) {
        XCE_DLOG("   -- Getting Watch Dog:   0x%08x\n", *__wdog);
      } else {
        // Always reset watchdog timer
        *__wdog         = 0x7fffffff;
        // See if they want it turned on or off
        if(sz>=7 && strncmp(ploc, "wdog=on", 7)==0) {
          *__cpu_rst_mask = *__cpu_rst_mask | 0x02;
          *__wdog_ctrl    = *__wdog_ctrl | 0x10;
        } else if(sz>=8 && strncmp(ploc, "wdog=off", 8)==0) {
          *__wdog_ctrl    = *__wdog_ctrl & ~0x10;
          *__cpu_rst_mask = *__cpu_rst_mask & ~0x02;
        }
      }
    } else if(strncmp(ploc, "gpio", 4)==0) {
      unsigned int gpio_n = 0;
      int gpio_v = 0;
      if(sz>=7 && sscanf(ploc, "gpio%u=%d", &gpio_n, &gpio_v)==2) {
        XCE_DLOG("   -- Setting GPIO[%u] to output %d\n", gpio_n, gpio_v);
        if(gpio_n < 32) {
          __gpio[1] = (__gpio[1]) & ~(1<<gpio_n);
          if(gpio_v) {
            __gpio[0] = (__gpio[0]) | (1<<gpio_n);
          } else {
            __gpio[0] = (__gpio[0]) & ~(1<<gpio_n);
          }
        } else if(gpio_n < 64) {
          __gpio_h[1] = (__gpio_h[1]) & ~(1<<(gpio_n-32));
          if(gpio_v) {
            __gpio_h[0] = (__gpio_h[0]) | (1<<(gpio_n-32));
          } else {
            __gpio_h[0] = (__gpio_h[0]) & ~(1<<(gpio_n-32));
          }
        }
      } else if(sz>=7 && sscanf(ploc, "gpioin%u", &gpio_n)==1) {
        XCE_DLOG("   -- Setting GPIO[%u] to input mode\n", gpio_n);
        if(gpio_n < 32) {
          __gpio[1] = (__gpio[1]) | (1<<gpio_n);
          XCE_DLOG("   -- Input on GPIO[%u]: %d\n", gpio_n, ((__gpio[4]) & (1<<gpio_n))?1:0);
        } else if(gpio_n < 64) {
          __gpio_h[1] = (__gpio_h[1]) | (1<<(gpio_n-32));
          XCE_DLOG("   -- Input on GPIO[%u]: %d\n", gpio_n, ((__gpio_h[4]) & (1<<gpio_n))?1:0);
        }
      }
      // Dump the current state
      xce_dev_dumpGPIO();
    } else if(strncmp(ploc, "reg", 3)==0) {
      unsigned int reg_n = 0;
      unsigned int reg_v = 0;
      if(sz>=6 && sscanf(ploc, "reg%x=%x", &reg_n, &reg_v)==2) {
        XCE_DLOG("   -- Setting REG[0x%05x] to 0x%08x\n", reg_n, reg_v);
        *((volatile unsigned int *)((KIRKWOOD_REGS_VIRT_BASE)+reg_n)) = reg_v;
      } else if(sz>=6 && sscanf(ploc, "regin%x", &reg_n)==1) {
        XCE_DLOG("   -- Getting REG[0x%05x]:   0x%08x\n", 
                 reg_n, *((volatile unsigned int *)((KIRKWOOD_REGS_VIRT_BASE)+reg_n)));
      }
    }
  }
#endif // arm
  if(mapped!=NULL) 
    kunmap_atomic(mapped, KM_USER0);
  if(pg!=NULL)
    put_page(pg);

 bail:
  return err;
}

static int xce_dev_open(struct inode * i, struct file * f)
{
  struct xce_dev * xced = kmalloc(sizeof(struct xce_dev), GFP_KERNEL);
  memset(xced, 0, sizeof(struct xce_dev));
/*   XCE_DLOG("Device opened by PID: %d\n", current->pid); */
  f->private_data = xced;
  
  return 0;
}

static int xce_dev_release(struct inode * i, struct file * f)
{
  struct xce_dev * xced = f->private_data;
/*   XCE_DLOG("Device released by PID: %d\n", current->pid); */
  if(atomic_read(&xced->mapped)!=0) {
    XCE_DLOG("WARNING: Still mapped? %d\n", atomic_read(&xced->mapped));
  }
  if(xced->apage!=0) {
    free_pages(xced->apage, get_order(PAGE_SIZE*XCE_MMAP_NPAGES));
    xced->apage = 0;
  }
  kfree(xced);
  f->private_data = NULL;

  return 0;
}

static int xce_dev_ioctl(struct inode * i, struct file * f, unsigned int cmd, unsigned long arg)
{
  void __user *argp = (void __user *)arg;
  unsigned int size;
  int ret = -ENOSYS;

  size = (cmd & IOCSIZE_MASK) >> IOCSIZE_SHIFT;
  if (cmd & IOC_IN) {
    if (!access_ok(VERIFY_READ, argp, size))
      return -EFAULT;
  }
  if (cmd & IOC_OUT) {
    if (!access_ok(VERIFY_WRITE, argp, size))
      return -EFAULT;
  }

  switch (cmd) {
  case XCE_IOCTL_GETBLPARAMS:
    {
      unsigned char * buf = NULL;
      struct xce_dev_getblparams blparams;
      if((ret=copy_from_user(&blparams, argp, sizeof(blparams)))==0) {
        blparams.outsize = 0;
        if (!access_ok(VERIFY_WRITE, blparams.bufptr, blparams.bufmaxsize))
          return -EFAULT;
        
        if((buf = xce_dev_loadprops(&blparams))!=NULL) {
          if ((ret=copy_to_user(argp, &blparams, sizeof(blparams)))<0) {
            // Failed to write data back out?  That is weird
          }
          kfree(buf);
        } else {
          ret = -EIO;
        }
      }
    }
    break;
  default:
    XCE_DLOG("BLPARAMS:   -- No IOCTL 0x%x.\n", cmd);
  }


  return ret;
}

static int xce_dev_mmap(struct file * f, struct vm_area_struct * vma)
{
  struct xce_dev * xced = f->private_data;
  unsigned long size;
  unsigned long apage;
  unsigned long upage;
  int err;

  if(vma->vm_pgoff) {
    XCE_DLOG("Invalid map offset %ld: must be 0\n", vma->vm_pgoff);
    return -EINVAL;
  }
  size = vma->vm_end - vma->vm_start;
  if(size != PAGE_SIZE * XCE_MMAP_NPAGES) {
    XCE_DLOG("Invalid map size %ld: must be %ld\n", size, PAGE_SIZE*XCE_MMAP_NPAGES);
    return -EINVAL;
  }

  XCE_DLOG("Mapping our %ld bytes of ringbuffer to user mem PID: %d\n", size, current->pid);
  if(xced->apage == 0) {
    struct xce_dev_uhdr * uhdr;
    apage = __get_free_pages(GFP_KERNEL | __GFP_COMP | __GFP_ZERO, get_order(size));
    uhdr = (struct xce_dev_uhdr *)apage;
    snprintf(uhdr->magic, sizeof(uhdr->magic), "XCE: %s", XCE_VERSION);
    uhdr->version = XCE_PROTO_VERSION;
    uhdr->nreqs   = (2*PAGE_SIZE) / sizeof(union xce_dev_req);
    uhdr->reqoff  = PAGE_SIZE;
    XCE_DLOG("  -- Allocated at 0x%lx\n", apage);
  } else {
    apage = xced->apage;
    XCE_DLOG("  -- Priorly allocated at 0x%lx\n", apage);
  }
  upage = vma->vm_start;
  XCE_DLOG("  -- Mapping at 0x%lx\n", upage);
#if 1
  // Older kernels seem to use this mechanism...
  if(remap_pfn_range(vma, upage, __pa(apage) >> PAGE_SHIFT,
                     XCE_MMAP_NPAGES * PAGE_SIZE, vma->vm_page_prot)) {
    err = -ENOMEM;
    goto bail;
  }
#else
  // But newer kernels seem to prefer this mechanism...
  {
    struct page *page;
    int i;
    page = virt_to_page(apage);
    for(i=0; i < XCE_MMAP_NPAGES; i++, page++) {
      err = vm_insert_page(vma, upage, page);
      if(unlikely(err)) {
        goto bail;
      }
      upage += PAGE_SIZE;
    }
  }
#endif
  atomic_inc(&xced->mapped);
  xced->apage = apage;
  vma->vm_ops = &xce_mmap_ops;
  return 0;
 bail:
  if(xced->apage==0) {
    free_pages(apage, get_order(size));
  }
  return err;
}

static unsigned int xce_dev_poll(struct file * f, struct poll_table_struct * pts)
{
  return 0;
}

static void xce_mm_open(struct vm_area_struct *vma)
{
  struct file * f = vma->vm_file;
  struct xce_dev * xced = f->private_data;
  XCE_DLOG("Device MM_OPEN by PID: %d\n", current->pid);
  atomic_inc(&xced->mapped);
}

static void xce_mm_close(struct vm_area_struct *vma)
{
  struct file * f = vma->vm_file;
  struct xce_dev * xced = f->private_data;
  XCE_DLOG("Device MM_CLOSE by PID: %d\n", current->pid);
  atomic_dec(&xced->mapped);
}

