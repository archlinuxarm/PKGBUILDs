#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID="$( cat /var/run/watchdog.pid 2>/dev/null )"
[ -r /etc/conf.d/watchdog ] && source /etc/conf.d/watchdog

case "$1" in
  start)
    stat_busy "Starting Watchdog Daemon"

    [ -z "$PID" ] && /usr/sbin/watchdog ${WATCHDOG_OPTIONS}

    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon watchdog
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Watchdog Daemon"

    [ -n "$PID" ] && kill $PID &> /dev/null

    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon watchdog
      stat_done
    fi

    rm -f /var/run/watchdog.pid
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
