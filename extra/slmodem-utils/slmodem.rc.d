#!/bin/bash

# source application-specific settings
SLMODEM_ARGS=
[ -f /etc/conf.d/slmodem ] && . /etc/conf.d/slmodem

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
  start)
    stat_busy "Starting SL-Modem Drivers" 
    if ! [ -e /dev/slamr0 ]; then
      mknod /dev/slamr0 c 242 0
    fi
    slmodemd ${SLMODEM_ARGS} > /dev/null 2>&1 &
    if [ $? -gt 0 ]; then 
      stat_fail
    else 
      add_daemon slmodem
      stat_done
    fi  
    ;;
  stop)
    stat_busy "Stopping SL-Modem Drivers"
    killall -9 slmodemd
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon slmodem
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
