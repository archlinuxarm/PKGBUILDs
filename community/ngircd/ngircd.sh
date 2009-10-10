#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
	start)
		stat_busy "Starting ngircd IRC daemon"
		/usr/sbin/ngircd
		if [ $? -gt 0 ]; then
			stat_fail
		else
			stat_done
			add_daemon ngircd
		fi
		;;
	stop)
		stat_busy "Stopping ngircd IRC daemon"
		PID=`pidof -o %PPID /usr/sbin/ngircd`
		kill $PID > /dev/null
		if [ $? -gt 0 ]; then
			stat_fail
		else
			rm_daemon ngircd
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
exit 0
