#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
  start)
    stat_busy "Starting Anacron Daemon"
    /usr/sbin/anacron -s >> /var/log/anacrond.log 2>&1
    if [ $? -gt 0 ]; then
      stat_fail
    else
    stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Anacron Daemon"
    stat_done
    ;;
  restart)
    $0 stop
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"  
esac
exit 0
