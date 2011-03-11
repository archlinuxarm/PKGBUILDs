#!/bin/bash
. /etc/rc.conf
. /etc/rc.d/functions
 
case "$1" in
  start)
    stat_busy "Registering .NET IL binaries with mono"
    if [ ! -d /proc/sys/fs/binfmt_misc ]; then
      stat_die "You need support for \"misc binaries\" in your kernel!"
    fi
    mount | grep -q binfmt_misc
    if [ $? != 0 ]; then
      mount -t binfmt_misc binfmt_misc /proc/sys/fs/binfmt_misc
      if [ $? != 0 ]; then
        stat_die
      fi
    fi
    echo ':CLR:M::MZ::/usr/bin/mono:' > /proc/sys/fs/binfmt_misc/register
    stat_done
    ;;
  stop)

    stat_busy "Unregistering .NET IL binaries"
    if [ -f /proc/sys/fs/binfmt_misc/CLR ]; then
      echo '-1' > /proc/sys/fs/binfmt_misc/CLR
    fi
    stat_done
    ;;
  restart)
    $0 stop
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"
esac 
