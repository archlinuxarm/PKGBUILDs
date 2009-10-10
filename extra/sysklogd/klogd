#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/klogd`
case "$1" in
  start)
    stat_busy "Starting Kernel Logger"
    [ -z "$PID" ] && /usr/sbin/klogd -c 4
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon klogd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Kernel Logger"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm -f /var/run/klogd.pid
      rm_daemon klogd
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
