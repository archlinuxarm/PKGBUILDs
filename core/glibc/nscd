#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/nscd`
case "$1" in
  start)
    stat_busy "Starting nscd"
    # create necessary directories if they don't already exist
    mkdir -p /var/run/nscd /var/db/nscd 2>/dev/null
    # remove stale files
    rm -f /var/db/nscd/* /var/run/nscd/* 2>/dev/null
    [ -z "$PID" ] && /usr/sbin/nscd
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon nscd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping nscd"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon nscd
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
