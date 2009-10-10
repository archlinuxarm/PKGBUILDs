#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/memcached

PIDFILE='/var/run/memcached.pid'

getpid() {
  local pid
  pid=$(cat $PIDFILE 2>/dev/null)
  # if the process is no longer valid, don't return it
  if [ -n "$pid" ]; then
    if ! ps -p $pid >/dev/null; then
      rm -f $PIDFILE
      pid=""
    fi
  fi
  echo $pid
}

PID="$(getpid)"

case "$1" in
  start)
    stat_busy "Starting memcached"
    # memcached is retarded and doesn't write to the pidfile
    # before it drops permissions
    if [ -n "$PID" ]; then
      stat_fail
    elif [ -z "$MEMCACHED_USER"  ]; then
      echo "MEMCACHED_USER must be defined in /etc/conf.d/memcached"
      stat_fail
    else
      touch $PIDFILE && chown $MEMCACHED_USER $PIDFILE
      /usr/bin/memcached -d -P $PIDFILE -u $MEMCACHED_USER $MEMCACHED_ARGS
      if [ $? -gt 0 ]; then
        stat_fail
      else
        add_daemon memcached
        stat_done
      fi
    fi
    ;;
  stop)
    stat_busy "Stopping memcached"
    [ ! -z "$PID" ] && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm -f $PIDFILE
      rm_daemon memcached
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
