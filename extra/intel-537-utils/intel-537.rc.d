#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
    start)
    stat_busy "Loading Intel Modem Driver" 
    hamregistry &
    if [ $? -gt 0 ]; then 
 stat_fail
    else 
 add_daemon intel-537
 stat_done
    fi  
    ;;
    stop)
    stat_busy "Stopping Intel Modem Driver"
    killall -9 hamregistry 
if [ $? -gt 0 ]; then
 stat_fail
    else
 rm_daemon intel-537
 stat_done
    fi
    ;;
    restart)
    $0 stop
    sleep 1
    $0 start
    ;;
    *)
    echo "Usage $0 {start|stop|restart}"
    ;;
esac