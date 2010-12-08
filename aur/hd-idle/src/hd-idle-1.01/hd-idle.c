/*
 * hd-idle.c - external disk idle daemon
 *
 * Copyright (c) 2007 Christian Mueller.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * hd-idle is a utility program for spinning-down external disks after a period
 * of idle time. Since most external IDE disk enclosures don't support setting
 * the IDE idle timer, a program like hd-idle is required to spin down idle
 * disks automatically.
 *
 * A word of caution: hard disks don't like spinning-up too often. Laptop disks
 * are more robust in this respect than desktop disks but if you set your disks
 * to spin down after a few seconds you may damage the disk over time due to the
 * stress the spin-up causes on the spindle motor and bearings. It seems that
 * manufacturers recommend a minimum idle time of 3-5 minutes, the default in
 * hd-idle is 10 minutes.
 *
 * Please note that hd-idle can spin down any disk accessible via the SCSI
 * layer (USB, IEEE1394, ...) but it will NOT work with real SCSI disks because
 * they don't spin up automatically. Thus it's not called scsi-idle and I don't
 * recommend using it on a real SCSI system unless you have a kernel patch that
 * automatically starts the SCSI disks after receiving a sense buffer indicating
 * the disk has been stopped. Without such a patch, real SCSI disks won't start
 * again and you can as well pull the plug.
 *
 * You have been warned...
 *
 * CVS Change Log:
 * ---------------
 *
 * $Log: hd-idle.c,v $
 * Revision 1.4  2010/02/26 14:03:44  cjmueller
 * Version 1.01
 * ------------
 *
 * Features
 * - The parameter "-a" now also supports symlinks for disk names. Thus, disks
 *   can be specified using something like /dev/disk/by-uuid/... Use "-d" to
 *   verify that the resulting disk name is what you want.
 *
 *   Please note that disk names are resolved to device nodes at startup. Also,
 *   since many entries in /dev/disk/by-xxx are actually partitions, partition
 *   numbers are automatically removed from the resulting device node.
 *
 * Bugs
 * - Not really a bug, but the disk name comparison used strstr which is a bit
 *   useless because only disks starting with "sd" and a single letter after
 *   that are currently considered. Replaced the comparison with strcmp()
 *
 * Revision 1.3  2009/11/18 20:53:17  cjmueller
 * Features
 * - New parameter "-a" to allow selecting idle timeouts for individual disks;
 *   compatibility to previous releases is maintained by having an implicit
 *   default which matches all SCSI disks
 *
 * Bugs
 * - Changed comparison operator for idle periods from '>' to '>=' to prevent
 *   adding one polling interval to idle time
 * - Changed sleep time before calling sync after updating the log file to 1s
 *   (from 3s) to accumulate fewer dirty blocks before synching. It's still
 *   a compromize but the log file is for debugging purposes, anyway. A test
 *   with fsync() was unsuccessful because the next bdflush-initiated sync
 *   still caused spin-ups.
 *
 * Revision 1.2  2007/04/23 22:14:27  cjmueller
 * Bug fixes
 * - Comment changes; no functionality changes...
 *
 * Revision 1.1.1.1  2007/04/23 21:49:43  cjmueller
 * initial import into CVS
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>

#define STAT_FILE "/proc/diskstats"
#define DEFAULT_IDLE_TIME 600

#define dprintf if (debug) printf

/* typedefs and structures */
typedef struct IDLE_TIME {
  struct IDLE_TIME  *next;
  char              *name;
  int                idle_time;
} IDLE_TIME;

typedef struct DISKSTATS {
  struct DISKSTATS  *next;
  char               name[50];
  int                idle_time;
  time_t             last_io;
  time_t             spindown;
  time_t             spinup;
  unsigned int       spun_down : 1;
  unsigned int       reads;
  unsigned int       writes;
} DISKSTATS;

/* function prototypes */
static void        daemonize       (void);
static DISKSTATS  *get_diskstats   (const char *name);
static void        spindown_disk   (const char *name);
static void        log_spinup      (DISKSTATS *ds);
static char       *disk_name       (char *name);

/* global/static variables */
IDLE_TIME *it_root;
DISKSTATS *ds_root;
char *logfile = "/dev/null";
int debug;

/* main function */
int main(int argc, char *argv[])
{
  IDLE_TIME *it;
  int have_logfile = 0;
  int min_idle_time;
  int sleep_time;
  int opt;

  /* create default idle-time parameter entry */
  if ((it = malloc(sizeof(*it))) == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  it->next = NULL;
  it->name = NULL;
  it->idle_time = DEFAULT_IDLE_TIME;
  it_root = it;

  /* process command line options */
  while ((opt = getopt(argc, argv, "t:a:i:l:dh")) != -1) {
    switch (opt) {

    case 't':
      /* just spin-down the specified disk and exit */
      spindown_disk(optarg);
      return(0);

    case 'a':
      /* add a new set of idle-time parameters for this particular disk */
      if ((it = malloc(sizeof(*it))) == NULL) {
        fprintf(stderr, "out of memory\n");
        return(2);
      }
      it->name = disk_name(optarg);
      it->idle_time = DEFAULT_IDLE_TIME;
      it->next = it_root;
      it_root = it;
      break;

    case 'i':
      /* set idle-time parameters for current (or default) disk */
      it->idle_time = atoi(optarg);
      break;

    case 'l':
      logfile = optarg;
      have_logfile = 1;
      break;

    case 'd':
      debug = 1;
      break;

    case 'h':
      printf("usage: hd-idle [-t <disk>] [-a <name>] [-i <idle_time>] [-l <logfile>] [-d] [-h]\n");
      return(0);

    case ':':
      fprintf(stderr, "error: option -%c requires an argument\n", optopt);
      return(1);

    case '?':
      fprintf(stderr, "error: unknown option -%c\n", optopt);
      return(1);
    }
  }

  /* set sleep time to 1/10th of the shortest idle time */
  min_idle_time = 1 << 30;
  for (it = it_root; it != NULL; it = it->next) {
    if (it->idle_time != 0 && it->idle_time < min_idle_time) {
      min_idle_time = it->idle_time;
    }
  }
  if ((sleep_time = min_idle_time / 10) == 0) {
    sleep_time = 1;
  }

  /* daemonize unless we're running in debug mode */
  if (!debug) {
    daemonize();
  }

  /* main loop: probe for idle disks and stop them */
  for (;;) {
    DISKSTATS tmp;
    FILE *fp;
    char buf[200];

    if ((fp = fopen(STAT_FILE, "r")) == NULL) {
      perror(STAT_FILE);
      return(2);
    }

    memset(&tmp, 0x00, sizeof(tmp));

    while (fgets(buf, sizeof(buf), fp) != NULL) {
      if (sscanf(buf, "%*d %*d %s %*u %*u %u %*u %*u %*u %u %*u %*u %*u %*u",
                 tmp.name, &tmp.reads, &tmp.writes) == 3) {
        DISKSTATS *ds;
        time_t now = time(NULL);

        /* make sure this is a SCSI disk (sd[a-z]) */
        if (tmp.name[0] != 's' ||
            tmp.name[1] != 'd' ||
            !isalpha(tmp.name[2]) ||
            tmp.name[3] != '\0') {
          continue;
        }

        dprintf("probing %s: reads: %d, writes: %d\n", tmp.name, tmp.reads, tmp.writes);

        /* get previous statistics for this disk */
        ds = get_diskstats(tmp.name);

        if (ds == NULL) {
          /* new disk; just add it to the linked list */
          if ((ds = malloc(sizeof(*ds))) == NULL) {
            fprintf(stderr, "out of memory\n");
            return(2);
          }
          memcpy(ds, &tmp, sizeof(*ds));
          ds->last_io = now;
          ds->spinup = ds->last_io;
          ds->next = ds_root;
          ds_root = ds;

          /* find idle time for this disk (falling-back to default; default means
           * 'it->name == NULL' and this entry will always be the last due to the
           * way this single-linked list is built when parsing command line
           * arguments)
           */
          for (it = it_root; it != NULL; it = it->next) {
            if (it->name == NULL || !strcmp(ds->name, it->name)) {
              ds->idle_time = it->idle_time;
              break;
            }
          }

        } else if (ds->reads == tmp.reads && ds->writes == tmp.writes) {
          if (!ds->spun_down) {
            /* no activity on this disk and still running */
            if (ds->idle_time != 0 && now - ds->last_io >= ds->idle_time) {
              spindown_disk(ds->name);
              ds->spindown = now;
              ds->spun_down = 1;
            }
          }

        } else {
          /* disk had some activity */
          if (ds->spun_down) {
            /* disk was spun down, thus it has just spun up */
            if (have_logfile) {
              log_spinup(ds);
            }
            ds->spinup = now;
          }
          ds->reads = tmp.reads;
          ds->writes = tmp.writes;
          ds->last_io = now;
          ds->spun_down = 0;
        }
      }
    }

    fclose(fp);
    sleep(sleep_time);
  }

  return(0);
}

/* become a daemon */
static void daemonize(void)
{
  int maxfd;
  int i;

  /* fork #1: exit parent process and continue in the background */
  if ((i = fork()) < 0) {
    perror("couldn't fork");
    exit(2);
  } else if (i > 0) {
    _exit(0);
  }

  /* fork #2: detach from terminal and fork again so we can never regain
   * access to the terminal */
  setsid();
  if ((i = fork()) < 0) {
    perror("couldn't fork #2");
    exit(2);
  } else if (i > 0) {
    _exit(0);
  }

  /* change to root directory and close file descriptors */
  chdir("/");
  maxfd = getdtablesize();
  for (i = 0; i < maxfd; i++) {
    close(i);
  }

  /* use /dev/null for stdin, stdout and stderr */
  open("/dev/null", O_RDONLY);
  open("/dev/null", O_WRONLY);
  open("/dev/null", O_WRONLY);
}

/* get DISKSTATS entry by name of disk */
static DISKSTATS *get_diskstats(const char *name)
{
  DISKSTATS *ds;

  for (ds = ds_root; ds != NULL; ds = ds->next) {
    if (!strcmp(ds->name, name)) {
      return(ds);
    }
  }

  return(NULL);
}

/* spin-down a disk */
static void spindown_disk(const char *name)
{
  struct sg_io_hdr io_hdr;
  unsigned char sense_buf[255];
  char dev_name[100];
  int fd;

  dprintf("spindown: %s\n", name);

  /* fabricate SCSI IO request */
  memset(&io_hdr, 0x00, sizeof(io_hdr));
  io_hdr.interface_id = 'S';
  io_hdr.dxfer_direction = SG_DXFER_NONE;

  /* SCSI stop unit command */
  io_hdr.cmdp = (unsigned char *) "\x1b\x00\x00\x00\x00\x00";

  io_hdr.cmd_len = 6;
  io_hdr.sbp = sense_buf;
  io_hdr.mx_sb_len = (unsigned char) sizeof(sense_buf);

  /* open disk device (kernel 2.4 will probably need "sg" names here) */
  snprintf(dev_name, sizeof(dev_name), "/dev/%s", name);
  if ((fd = open(dev_name, O_RDONLY)) < 0) {
    perror(dev_name);
    return;
  }

  /* execute SCSI request */
  if (ioctl(fd, SG_IO, &io_hdr) < 0) {
    char buf[100];
    snprintf(buf, sizeof(buf), "ioctl on %s:", name);
    perror(buf);

  } else if (io_hdr.masked_status != 0) {
    fprintf(stderr, "error: SCSI command failed with status 0x%02x\n",
            io_hdr.masked_status);
  }

  close(fd);
}

/* write a spin-up event message to the log file */
static void log_spinup(DISKSTATS *ds)
{
  FILE *fp;

  if ((fp = fopen(logfile, "a")) != NULL) {
    /* Print statistics to logfile
     *
     * Note: This doesn't work too well if there are multiple disks
     *       because the I/O we're dealing with might be on another
     *       disk so we effectively wake up the disk the log file is
     *       stored on as well. Then again the logfile is a debugging
     *       option, so what...
     */
    time_t now = time(NULL);
    char tstr[20];
    char dstr[20];

    strftime(dstr, sizeof(dstr), "%Y-%m-%d", localtime(&now));
    strftime(tstr, sizeof(tstr), "%H:%M:%S", localtime(&now));
    fprintf(fp,
            "date: %s, time: %s, disk: %s, running: %ld, stopped: %ld\n",
            dstr, tstr, ds->name,
            (long) ds->spindown - (long) ds->spinup,
            (long) time(NULL) - (long) ds->spindown);

    /* Sync to make sure writing to the logfile won't cause another
     * spinup in 30 seconds (or whatever bdflush uses as flush interval).
     */
    fclose(fp);
    sleep(1);
    sync();
  }
}

/* Resolve disk names specified as "/dev/disk/by-xxx" or some other symlink.
 * Please note that this function is only called during command line parsing
 * and hd-idle per se does not support dynamic disk additions or removals at
 * runtime.
 *
 * This might change in the future but would require some fiddling to avoid
 * needless overhead -- after all, this was designed to run on tiny embedded
 * devices, too.
 */
static char *disk_name(char *path)
{
  ssize_t len;
  char buf[256];
  char *s;

  if (*path != '/') {
    /* just a disk name without /dev prefix */
    return(path);
  }

  if ((len = readlink(path, buf, sizeof(buf) - 1)) <= 0) {
    if (errno != EINVAL) {
      /* couldn't resolve disk name */
      return(path);
    }

    /* 'path' is not a symlink */
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf)-1] = '\0';
    len = strlen(buf);
  }
  buf[len] = '\0';

  /* remove partition numbers, if any */
  for (s = buf + strlen(buf) - 1; s >= buf && isdigit(*s); s--) {
    *s = '\0';
  }

  /* Extract basename of the disk in /dev. Note that this assumes that the
   * final target of the symlink (if any) resolves to /dev/sd*
   */
  if ((s = strrchr(buf, '/')) != NULL) {
    s++;
  } else {
    s = buf;
  }

  if ((s = strdup(s)) == NULL) {
    fprintf(stderr, "out of memory");
    exit(2);
  }

  if (debug) {
    printf("using %s for %s\n", s, path);
  }
  return(s);
}
