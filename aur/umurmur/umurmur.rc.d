#!/bin/bash

CONF=/etc/conf.d/umurmur

. /etc/rc.conf
. /etc/rc.d/functions

[ -f $CONF ] && . $CONF

PID=$(pidof -o %PPID /usr/sbin/umurmurd)
case "$1" in
  start)
    stat_busy "Starting umurmur"
    [ -z "$PID" ] && /usr/sbin/umurmurd $UMURMUR_ARGS
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon umurmur
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping umurmur"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon umurmur
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
