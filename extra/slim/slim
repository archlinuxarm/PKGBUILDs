#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/bin/slim`
case "$1" in
  start)
    stat_busy "Starting Simple Login Manager"
    [ -z "$PID" ] && /usr/bin/slim -d &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon slim
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Simple Login Manager"
    [ ! -z "$PID" ] && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon slim
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
