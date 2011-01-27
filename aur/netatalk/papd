#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/papd`
case "$1" in
  start)
    stat_busy "Starting papd Daemon"
    [ -z "$PID" ] && /usr/sbin/papd
    if [ $? -gt 0 ]; then
      stat_fail
    else
      echo $PID > /var/run/papd.pid
      add_daemon papd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping papd Daemon"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm /var/run/papd.pid
      rm_daemon papd
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
