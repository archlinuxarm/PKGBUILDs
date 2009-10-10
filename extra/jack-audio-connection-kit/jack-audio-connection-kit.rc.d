#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

# source application-specific settings
[ -f /etc/conf.d/jack-audio-connection-kit ] && . /etc/conf.d/jack-audio-connection-kit

PID=`pidof -o %PPID /usr/bin/jackd`

case "$1" in
  start)
    stat_busy "Starting Jack-audio-connection-kit Daemon"
    [ -z "$PID" ] && /usr/bin/jackd $SERVER_PARAMS -d $DRIVER_PARAMS &> /dev/null &
    if [ ! -z "$PID" -o $? -gt 0 ]; then
      stat_fail
    else
      add_daemon jack-audio-connection-kit
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Jack-audio-connection-kit Daemon"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon jack-audio-connection-kit
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
