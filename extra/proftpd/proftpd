#!/bin/bash

# source application-specific settings
PROFTPD_ARGS=
[ -f /etc/conf.d/proftpd ] && . /etc/conf.d/proftpd

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
  start)
    stat_busy "Starting ProFTPd Server"
    /usr/sbin/proftpd ${PROFTPD_ARGS}
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon proftpd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping ProFTPd Server"
    [ -f /var/run/proftpd.pid ] && kill $(cat /var/run/proftpd.pid) &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm -f /var/run/proftpd.pid
      rm_daemon proftpd
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
exit 0

