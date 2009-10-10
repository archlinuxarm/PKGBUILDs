#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

DAEMON_NAME="hulamanager"
DAEMON_PATH="/usr/sbin/${DAEMON_NAME}"

PID=`pidof -o %PPID ${DAEMON_NAME}`
case "$1" in
  start)
    stat_busy "Starting ${DAEMON_NAME}"
    [ -z "$PID" ] && ${DAEMON_PATH} -d
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon ${DAEMON_NAME}
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping ${DAEMON_NAME}"
    [ ! -z "$PID" ]  && ${DAEMON_PATH} --stop &>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon ${DAEMON_NAME}
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
