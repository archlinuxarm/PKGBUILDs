#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/lircmd`
case "$1" in
  start)
    stat_busy "Starting lircmd Daemon"
    [ -z "$PID" ] && /usr/sbin/lircmd
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon lircmd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping lircmd Daemon"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon lircmd
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
