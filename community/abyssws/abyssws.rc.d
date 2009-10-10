#!/bin/bash

[ -f /etc/conf.d/abyssws ] && . /etc/conf.d/abyssws

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
  start)
    stat_busy "Starting Abyss Web Server X1"
      /opt/abyssws/abyssws -d $OPTIONS
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon abyssws
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Abyss Web Server X1"
    /opt/abyssws/abyssws --stop
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon abyssws
      stat_done
    fi
    ;;
  restart)
    stat_busy "Restarting Abyss Web Server X1"
    /opt/abyssws/abyssws --restart
    if [ $? -gt 0 ]; then
      stat_fail
    else
      stat_done
    fi
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"
esac
exit 0
