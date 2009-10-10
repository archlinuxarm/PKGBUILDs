#!/bin/bash

daemon_name=autofs

. /etc/rc.conf
. /etc/rc.d/functions

# source application-specific settings
[ -f /etc/conf.d/autofs ] && . /etc/conf.d/autofs

if [ ! -z "$TIMEOUT" ]; then
  daemonoptions="--timeout=$TIMEOUT $daemonoptions"
fi

PID=`cat /var/run/autofs-running 2> /dev/null`
case "$1" in
  start)
    stat_busy "Starting $daemon_name daemon"
    [ -z "$PID" ] && /usr/sbin/automount $daemonoptions &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon $daemon_name
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping $daemon_name daemon"
    [ ! -z "$PID" ] && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon $daemon_name
      stat_done
    fi
    ;;
  restart)
    stat_busy "Restarting $daemon_name daemon"
    $0 stop
    sleep 1
    $0 start
    ;;
  status)
    stat_busy "Checking $daemon_name status";
    ck_status $daemon_name
    ;;
  *)
    echo "usage: $0 {start|stop|restart|status}"  
esac
exit 0
