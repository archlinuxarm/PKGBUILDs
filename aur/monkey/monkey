#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

DAEMON=monkey

PID=$(get_pid $DAEMON)

case "$1" in
  start)
    stat_busy "Starting $DAEMON"
    [[ -z "$PID" ]] && /usr/bin/monkey -D &>/dev/null \
    && { add_daemon $DAEMON; stat_done; } \
    || { stat_fail; exit 1; }
    ;;
  stop)
    stat_busy "Stopping $DAEMON"
    [[ ! -z "$PID" ]] && kill $PID &>/dev/null \
    && { rm_daemon $DAEMON; stat_done; } \
    || { stat_fail; exit 1; }
    ;;
  restart)
    $0 stop
    sleep 1
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"
    ;;
esac
exit 0
