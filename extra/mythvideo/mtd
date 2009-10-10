#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/bin/mtd`
case "$1" in
  start)
    stat_busy "Starting Myth Transcoding Daemon"
    [ -z "$PID" ] && /usr/bin/mtd -d
    if [ $? -gt 0 ]; then
      stat_fail
    else
      echo $PID > /var/run/mtd.pid
      add_daemon mythtranscode
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Myth Transcoding Daemon"
    [ ! -z "$PID" ]  && kill $PID &>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon mtd 
      stat_done
    fi
    ;;
  restart)
    $0 stop
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"
esac
exit 0
