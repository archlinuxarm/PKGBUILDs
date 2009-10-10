#!/bin/bash

# source application-specific settings
SQUID_ARGS=
[ -f /etc/conf.d/squid ] && . /etc/conf.d/squid

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/squid`
case "$1" in

  start)
    if [ ! -f /var/cache/squid/swap.state ]; then
      stat_busy "Creating squid's swap directories"
      /usr/sbin/squid -z
    fi
    stat_busy "Starting squid"
    [ -z "$PID" ] && /usr/sbin/squid ${SQUID_ARGS}
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon squid
      stat_done
    fi
    ;;

  stop)
    stat_busy "Stopping squid"
    [ ! -z "$PID" ]  && /usr/sbin/squid -k shutdown &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      # wait for squid to shutdown so we can safely do a restart
      while [ ! -z "`pidof -o %PPID /usr/sbin/squid`" ]; do
        stat_append "."
        sleep 3
      done
      rm_daemon squid
      stat_done
    fi
    ;;

  restart)
    $0 stop
    sleep 5
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"  
esac
exit 0
