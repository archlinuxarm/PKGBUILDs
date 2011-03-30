#!/bin/bash

# general config
. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/asterisk`
case "$1" in
  start)
    stat_busy "Starting Asterisk"
    [ -z "$PID" ] && cd /var/lib/asterisk && /usr/sbin/asterisk -G asterisk -U asterisk
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon asterisk
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Asterisk"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm -f /var/run/asterisk/asterisk.pid &>/dev/null
      rm_daemon asterisk
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
