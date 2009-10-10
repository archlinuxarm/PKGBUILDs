#!/bin/bash

. /etc/rc.conf
. /etc/conf.d/espeakup
. /etc/rc.d/functions

PID=`pidof espeakup`
PIDFILE=/var/run/espeakup.pid
case "$1" in
  start)
    stat_busy "Starting Espeakup"
    if [ -z "$PID" ]; then
      if [ -f $PIDFILE ]; then
        rm $PIDFILE
      fi
      /usr/bin/espeakup $ESPEAKUP_ARGS
      if [ $? -gt 0 ]; then
        stat_fail
      else
        add_daemon espeakup
        stat_done
      fi
    fi
    ;;
  stop)
    stat_busy "Stopping Espeakup"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon espeakup
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
