#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

start() {
  stat_busy "Starting Authdaemond"
  /usr/sbin/authdaemond start &> /dev/null
  if [ $? -gt 0 ]; then
    stat_fail
  else
    ln -s /var/spool/authdaemon/pid /var/run/authdaemond.pid
    add_daemon authdaemond
    stat_done
  fi
}

stop() {
  stat_busy "Stopping Authdaemond"
  /usr/sbin/authdaemond stop &> /dev/null
  if [ $? -gt 0 ]; then
    stat_fail
  else
    rm -f /var/run/authdaemond.pid
    rm_daemon authdaemond
    # housecleaning; just like kids'n toys - don't care when it's needed anymore
    rm -f /var/spool/authdaemon/{pid.lock,pid,socket} &> /dev/null
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
    # calling 'stop' and 'start' without the $0 fails...
    $0 stop
    sleep 2
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"
esac
exit 0
