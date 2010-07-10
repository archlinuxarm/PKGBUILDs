#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

start() {
	stat_busy "Starting DAAP server"
	/usr/sbin/mt-daapd 
	stat_done
}

stop() {
	stat_busy "Shutting down DAAP server"
	killall -INT mt-daapd
	stat_done
}

case "$1" in
  start)
	start
	;;
  stop)
	stop
	;;
  restart)
	stop
	start
	;;
  *)
	echo $"Usage: $0 {start|stop|restart}"
	;;
esac
exit 0
