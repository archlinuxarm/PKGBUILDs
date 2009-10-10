#!/bin/bash

# source application-specific settings
[ -f /etc/conf.d/privoxy ] && . /etc/conf.d/privoxy

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/privoxy`

start() {
  stat_busy "Starting Privoxy"

  # create missing logfiles
  for i in logfile jarfile; do 
      touch /var/log/privoxy/$i
      chgrp $PRIVOXY_GROUP /var/log/privoxy/$i
      chmod 660 /var/log/privoxy/$i
  done
  [ -z "$PID" ] && /usr/sbin/privoxy $PRIVOXY_ARGS 2>/dev/null
  if [ $? -gt 0 ]; then
    stat_fail
  else
    add_daemon privoxy
    stat_done
  fi
}

stop() {
  stat_busy "Stopping Privoxy"
  [ ! -z "$PID" ]  && kill $PID &> /dev/null
  if [ $? -gt 0 ]; then
    stat_fail
  else
    rm_daemon privoxy
    stat_done
  fi
}

case "$1" in
  start)
    start
    ;;
  stop)
    stop
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
