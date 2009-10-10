#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/dhcp

PIDFILE="/var/run/dhcpd.pid"
PID=`cat $PIDFILE 2>/dev/null`
case "$1" in
  start)
    stat_busy "Starting DHCP Server"
    if [ "$PID" = "" ]; then 
       /usr/sbin/dhcpd $DHCP_ARGS 
    fi
    if [ "$PID" != "" -o $? -gt 0 ]; then
      stat_fail
    else
      add_daemon dhcpd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping DHCP Server"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    rm -f $PIDFILE
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon dhcpd
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
