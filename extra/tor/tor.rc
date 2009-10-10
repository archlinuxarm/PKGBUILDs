#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/bin/tor`
case "$1" in
	start)
		stat_busy "Starting Tor Daemon"
		[ -z "$PID" ] && /usr/bin/tor &>/dev/null
		if [ $? -gt 0 ]; then
			stat_fail
		else
			add_daemon tor
			stat_done
		fi
		;;
	stop)
		stat_busy "Stopping Tor Daemon"
		[ ! -z "$PID" ] && kill $PID &> /dev/null
		if [ $? -gt 0 ]; then
			stat_fail
		else
			rm_daemon tor
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
# vim: ft=sh ts=2 sw=2
