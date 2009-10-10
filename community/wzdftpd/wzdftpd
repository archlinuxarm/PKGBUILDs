#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/wzdftpd`
case "$1" in
  start)
    stat_busy "Starting WzdFTPD Server"
    [ -z "$PID" ] && /usr/sbin/wzdftpd
    if [ $? -gt 0 ]; then
      stat_fail
    else
      echo $PID > /var/run/wzdftpd.pid
      add_daemon wzdftpd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping WzdFTPD Server"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm /var/run/wzdftpd.pid
      rm_daemon wzdftpd
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
