#!/bin/bash

daemon_name=cherokee

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
  start)
    stat_busy "Starting $daemon_name daemon"
    if [ ! -f /var/run/$daemon_name.pid ] && $daemon_name -d &>/dev/null; then
      add_daemon $daemon_name
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;

  stop)
    stat_busy "Stopping $daemon_name daemon"
    [ -f /var/run/$daemon_name.pid ] && read PID </var/run/$daemon_name.pid
    if kill $PID &>/dev/null; then
      rm_daemon $daemon_name
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;

  reload)
    stat_busy "Reloading $daemon_name daemon"
    [ -f /var/run/$daemon_name.pid ] && read PID </var/run/$daemon_name.pid
    if kill -HUP $PID &>/dev/null; then
      add_daemon $daemon_name
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;

  restart)
    stat_busy "Restarting $daemon_name daemon"
    [ -f /var/run/$daemon_name.pid ] && read PID </var/run/$daemon_name.pid
    if kill -USR1 $PID &>/dev/null; then
      add_daemon $daemon_name
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;

  status)
    stat_busy "Checking $daemon_name status";
    ck_status $daemon_name
    ;;

  *)
    echo "usage: $0 {start|stop|reload|restart|status}"
esac

exit 0
