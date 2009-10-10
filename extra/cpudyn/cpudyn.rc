#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

# source application-specific settings
[ -f /etc/conf.d/cpudyn ] && . /etc/conf.d/cpudyn

PID=`pidof -o %PPID /usr/sbin/cpudynd`
case "$1" in
  start)
    stat_busy "Starting Cpudyn"
    [ -z "$PID" ] && /usr/sbin/cpudynd -d $CPUDYN_OPTS 2>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon cpudyn
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Cpudyn"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon cpudyn
      stat_done
    fi
    ;;
  restart)
    $0 stop
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"  
esac
exit 0
