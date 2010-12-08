#!/bin/sh
# Based on the Debian init.d script from the original hd-idle tarball.
# Modified for Arch Linux by Tilman Blumenbach <tilman@ax86.net>.

DAEMON=/usr/sbin/hd-idle
HD_IDLE_OPTS="-i 600"
START_HD_IDLE=false

[ -r /etc/conf.d/hd-idle ] && . /etc/conf.d/hd-idle

if [ "$START_HD_IDLE" != "true" ] ; then
  exit 0
fi

# See if the daemon is there
test -x $DAEMON || exit 0

. /etc/rc.conf
. /etc/rc.d/functions

APP_PID=`pidof -s "${DAEMON}"`

case "$1" in
	start)
		stat_busy "Starting the hd-idle daemon"

		[ -z "$APP_PID" ] && $DAEMON $HD_IDLE_OPTS
		if [ $? -eq 0 ]; then
			stat_done
			add_daemon hd-idle
		else
			stat_fail
		fi
		;;

	stop)
		stat_busy "Stopping the hd-idle daemon"

		[ -n "$APP_PID" ] && kill "$APP_PID"
		if [ $? -eq 0 ]; then
			stat_done
			rm_daemon hd-idle
		else
			stat_fail
		fi
		;;

	restart|force-reload)
		$0 stop && sleep 2 && $0 start
		;;

	*)
		echo "Usage: /etc/init.d/hd-idle start/stop/restart/force-reload"
		exit 1
		;;
esac
