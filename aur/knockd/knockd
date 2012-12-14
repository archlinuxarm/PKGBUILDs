#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/knockd`
case "$1" in
  start)
    stat_busy "Starting Port-Knocking Daemon"
    if [ -z "$PID" ]; then 
       /usr/sbin/knockd -d
    fi
    if [ ! -z "$PID" -o $? -gt 0 ]; then
      stat_fail
    else
      add_daemon knockd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Port-Knocking Daemon"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon knockd
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
