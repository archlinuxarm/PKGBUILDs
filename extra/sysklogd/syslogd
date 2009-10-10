#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/syslogd`
case "$1" in
  start)
    stat_busy "Starting System Logger"
    [ -z "$PID" ] && /usr/sbin/syslogd -m 0
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon syslogd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping System Logger"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm -f /var/run/syslogd.pid
      rm_daemon syslogd
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
