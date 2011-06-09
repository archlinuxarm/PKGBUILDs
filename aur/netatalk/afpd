#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=$(pidof -o %PPID /usr/sbin/afpd)
case $1 in
  start)
    stat_busy "Starting afpd Daemon"
    [ -z "$PID" ] && /usr/sbin/afpd
    if [ $? -gt 0 ]; then
      stat_fail
    else
      PID=$(pidof -o %PPID /usr/sbin/afpd)
      echo "$PID" > /var/run/afpd.pid
      add_daemon afpd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping afpd Daemon"
    [ ! -z "$PID" ]  && kill "$PID" &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon afpd
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
