#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PIDFILE="/var/run/pure-ftpd.pid"
case "$1" in
  start)
    stat_busy "Starting Pure-FTPd"
    [ ! -f $PIDFILE ] && /usr/sbin/pure-config.pl /etc/pure-ftpd.conf &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon pure-ftpd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Pure-FTPd"
    # Just kill the master server, preserve existing connections.
    #
    [ -f $PIDFILE ] && kill `cat $PIDFILE` &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon pure-ftpd
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
