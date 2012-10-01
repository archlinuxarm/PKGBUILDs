#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=$(pidof -o %PPID /usr/sbin/lightdm)
case "$1" in
  start)
    stat_busy "Starting LightDM"
    [ -z "$PID" ] && /usr/sbin/lightdm &>/dev/null &
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon lightdm
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping LightDM"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon lightdm
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
