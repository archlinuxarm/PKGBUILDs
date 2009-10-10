#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/pdnsd`
case "$1" in
  start)
    stat_busy "Starting pdnsd Daemon"
    if [ -z "$PID" ]; then 
       /usr/sbin/pdnsd -d
    fi
    if [ ! -z "$PID" -o $? -gt 0 ]; then
      stat_fail
    else
      add_daemon pdnsd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping pdnsd Daemon"
    [ ! -z "$PID" ]  && kill -TERM $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon pdnsd
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
