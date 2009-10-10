#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/fcron`
case "$1" in
  start)
    stat_busy "Starting Fcron Daemon"
    [ -z "$PID" ] && /usr/sbin/fcron -b
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon fcron
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Fcron Daemon"
    [ -n "$PID" ]  && kill $PID >/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon fcron
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
