#!/bin/bash
#
# fuse		Init script for Filesystem in Userspace
# Based on the script by Miklos Szeredi <miklos@szeredi.hu>

. /etc/rc.conf
. /etc/rc.d/functions
FUSECTL=/sys/fs/fuse/connections

case "$1" in
  start)
    stat_busy "Starting fuse"
    if ! grep -qw fuse /proc/filesystems; then
      modprobe fuse >/dev/null 2>&1
      if [ $? -gt 0 ]; then
        stat_fail
        exit 1
      fi
    fi
    if grep -qw fusectl /proc/filesystems && ! grep -qw $FUSECTL /proc/mounts; then
      mount -t fusectl none $FUSECTL >/dev/null 2>&1
      if [ $? -gt 0 ]; then
        stat_fail
        exit 1
      fi
    fi
    add_daemon fuse
    stat_done
    ;;
  stop)
    stat_busy "Stopping fuse"
    umount $FUSECTL >/dev/null 2>&1
    rmmod fuse >/dev/null 2>&1
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon fuse
      stat_done
    fi
    ;;
  restart)
    $0 stop
    sleep 1
    $0 start
    ;;
  *)  
    echo "usage: $0 {start|stop|restart}"
esac
exit 0
