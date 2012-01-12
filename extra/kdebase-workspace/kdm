#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=$(pidof -o %PPID /usr/bin/kdm)
case "$1" in
  start)
    stat_busy "Starting KDE Desktop Manager"
    [ -z "$PID" ] && /usr/bin/kdm &>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon kdm
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping KDE Desktop Manager"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon kdm
      stat_done
    fi
    ;;
  restart)
    $0 stop
    sleep 3
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"
esac
exit 0
