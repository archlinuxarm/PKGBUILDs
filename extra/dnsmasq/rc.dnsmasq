#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/dnsmasq`
case "$1" in
  start)
    stat_busy "Starting DNS/DHCP daemon"
    [ -z "$PID" ] && /usr/sbin/dnsmasq
    if [ $? -gt 0 ] ; then
      stat_fail
    else
      add_daemon dnsmasq                     # create the 'state' dir
      stat_done 
    fi
    ;;
  stop)
    stat_busy "Stopping DNS/DHCP daemon"
    [ "$PID" ] && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon dnsmasq                      # remove the 'state' dir
      stat_done
    fi
    ;;
  restart)
    $0 stop
    sleep 5
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"
esac
exit 0
