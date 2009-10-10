#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PARAMS=
DRIVES=
[ -f /etc/conf.d/hddtemp ] && . /etc/conf.d/hddtemp
PID=$(pidof -o %PPID /usr/sbin/hddtemp)
case "$1" in
  start)
    stat_busy "Starting HDDTemp"
    [ -z "$PID" ] && /usr/sbin/hddtemp -d $PARAMS $DRIVES
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon hddtemp
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping HDDTemp"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon hddtemp
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
