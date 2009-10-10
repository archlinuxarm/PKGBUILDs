#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/lircd

PID=$(pidof -o %PPID /usr/sbin/lircd)
case "$1" in
  start)
    stat_busy "Starting LIRC Daemon"
    [ -n "$LIRC_DRIVER" ] && LIRC_EXTRAOPTS="-H $LIRC_DRIVER $LIRC_EXTRAOPTS"
    [ -z "$PID" ] && 
      if [ -n "$LIRC_DEVICE" ] ; then
        /usr/sbin/lircd -d "$LIRC_DEVICE" $LIRC_EXTRAOPTS $LIRC_CONFIGFILE
      else
        /usr/sbin/lircd $LIRC_EXTRAOPTS $LIRC_CONFIGFILE
      fi
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon lircd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping LIRC Daemon"
    [ ! -z "$PID" ] && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon lircd
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
