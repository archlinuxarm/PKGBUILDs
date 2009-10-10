#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

get_pid() {
	pidof -o %PPID /usr/sbin/bitlbee
}

case "$1" in
  start)
    stat_busy "Starting Bitlbee"

    PID=$(get_pid)
    if [ -z "$PID" ]; then
      [ -f /var/run/bitlbee/bitlbee.pid ] && rm -f /var/run/bitlbee/bitlbee.pid
      su -s /bin/sh -c "/usr/sbin/bitlbee -F" "bitlbee"
      if [ $? -gt 0 ]; then
        stat_fail
        exit 1
      else
        add_daemon bitlbee
        stat_done
      fi
    else
      stat_fail
      exit 1
    fi
    ;;

  stop)
    stat_busy "Stopping Bitlbee"
    PID=$(get_pid)
    # KILL
    [ ! -z "$PID" ] && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
      exit 1
    else
      rm -f /var/run/bitlbee/bitlbee.pid &> /dev/null
      rm_daemon bitlbee
      stat_done
    fi
    ;;

  restart)
    $0 stop
    sleep 3
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"  
esac
exit 0
