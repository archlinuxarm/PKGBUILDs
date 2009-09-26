#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
  start)
    stat_busy "Starting Music Player Daemon"
    /usr/bin/mpd /etc/mpd.conf &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon mpd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Music Player Daemon"
    /usr/bin/mpd --kill /etc/mpd.conf &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon mpd
      stat_done
    fi
    ;;
  create-db)
    stat_busy "Creating mpd's database ..."
	logpath="/var/log/mpd/mpd.db-creation"
    /usr/bin/mpd --create-db /etc/mpd.conf > $logpath \
        && stat_busy "Output written to $logpath"
              stat_done
    ;;
  restart)
    $0 stop
    sleep 1
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart|create-db}"
esac
exit 0
