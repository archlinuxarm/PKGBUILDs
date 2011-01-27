#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

myhostname=`hostname|sed 's/\..*$//'`

PID=`pidof -o %PPID /usr/sbin/atalkd`
case "$1" in
  start)
    stat_busy "Starting atalkd Daemon"
    [ -z "$PID" ] && /usr/sbin/atalkd 
    if [ $? -gt 0 ]; then
      stat_fail
    else
      echo $PID > /var/run/atalkd.pid

      # Setting AppleTalk info with nbprgstr
      add_daemon atalkd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping atalkd Daemon"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm /var/run/atalkd.pid
      rm_daemon atalkd
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
