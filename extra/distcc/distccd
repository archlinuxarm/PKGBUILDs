#!/bin/bash

[ -f /etc/conf.d/distccd ] && . /etc/conf.d/distccd

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/bin/distccd`
case "$1" in
  start)
    stat_busy "Starting distcc Daemon"
    [ -z "$PID" ] && /usr/bin/distccd --daemon ${DISTCC_ARGS}
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon distccd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping distcc Daemon"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon distccd
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
