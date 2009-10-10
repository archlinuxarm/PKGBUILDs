#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
case "$1" in
  start)
    stat_busy "Starting wicd Daemon"
    pkill -f wicd-daemon.py &> /dev/null
    /usr/sbin/wicd &> /dev/null
    add_daemon wicd
    stat_done
    ;;
  stop)
    stat_busy "Stopping wicd Daemon"
    pkill -f wicd-daemon.py &> /dev/null
    rm_daemon wicd
    stat_done
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
