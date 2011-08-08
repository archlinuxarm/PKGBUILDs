#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/irexec.conf

PID=`pidof -o %PPID /usr/bin/irexec`
case "$1" in
  start)
    stat_busy "Starting IREXEC Daemon"
    [ -z "$PID" ] && /usr/bin/irexec --daemon $IREXEC_OPTS
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon irexecd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping IREXEC Daemon"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon irexecd
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

