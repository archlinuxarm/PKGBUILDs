#!/bin/bash

# source application-specific settings
[[ -f /etc/conf.d/squid ]] && . /etc/conf.d/squid

. /etc/rc.conf
. /etc/rc.d/functions

pidfile=/run/squid.pid
{ read -r PID </run/squid.pid; } 2>/dev/null
if [[ $pid && ! /proc/$pid/exe -ef /usr/sbin/squid ]]; then
  rm /run/squid.pid
fi

case $1 in
  start)
    stat_busy "Starting squid"
    if [[ $PID ]] || ! squid $SQUID_ARGS; then
      stat_fail
    else
      add_daemon squid
      stat_done
    fi
    ;;

  stop)
    stat_busy "Stopping squid"
    if [[ -z $PID ]] || ! squid -k shutdown &>/dev/null; then
      stat_fail
    else
      # squid takes forever to shutdown all its listening FDs
      while [[ /proc/$PID/exe -ef /usr/sbin/squid ]]; do
        stat_append "."
        sleep 3
      done
      rm_daemon squid
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
