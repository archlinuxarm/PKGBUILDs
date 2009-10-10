#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /sbin/mdadm`
case "$1" in
  start)
    stat_busy "Starting mdadm RAID Monitor"
    if [ -z "$PID" ]; then 
       /sbin/mdadm --monitor --scan -i /var/run/mdadm.pid -f
    fi
    if [ ! -z "$PID" -o $? -gt 0 ]; then
      stat_fail
    else
      add_daemon mdadm
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping mdadm RAID Monitor"
    [ ! -z "$PID" ] && kill $PID &>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon mdadm
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
