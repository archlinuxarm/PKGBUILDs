#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/dovecot`
case "$1" in
  start)
  stat_busy "Starting Dovecot"
    [ -z "$PID" ] && /usr/sbin/dovecot
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon dovecot
      stat_done
    fi
    ;;
  stop)
  stat_busy "Stopping Dovecot"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon dovecot
      stat_done
    fi
    ;;
  restart)
    $0 stop
    sleep 2
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"  
esac
exit 0
