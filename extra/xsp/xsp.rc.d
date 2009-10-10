#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

# source application-specific settings
[ -f /etc/conf.d/xsp ] && . /etc/conf.d/xsp

PID=`pidof -o %PPID /usr/bin/mono`
case "$1" in
  start)
    stat_busy "Starting Xsp Daemon"
    if [ -z "$PID" ]; then 
       /usr/bin/xsp $XSP_PARAMS --nonstop &> /dev/null &
    fi
    if [ ! -z "$PID" -o $? -gt 0 ]; then
      stat_fail
    else
      add_daemon xsp
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Xsp Daemon"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon xsp
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
