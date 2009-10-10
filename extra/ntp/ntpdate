#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/ntp-client.conf

case "$1" in
  start)
    stat_busy "Starting NTP Client"
    /usr/bin/ntpdate $NTP_CLIENT_OPTION -t $NTPCLIENT_TIMEOUT $NTP_CLIENT_SERVER > /dev/null 2>&1
    if [  $? -gt 0 ]; then
      stat_fail
    else
      add_daemon ntpdate
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping NTP Client"
    rm_daemon ntpdate
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
