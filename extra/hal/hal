#!/bin/bash

# general config
. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
    start)
        #Check for running dbus, start when not running
    	ck_daemon dbus && /etc/rc.d/dbus start
	if [ -x /etc/rc.d/acpid ]; then
		ck_daemon acpid && /etc/rc.d/acpid start
	fi
        stat_busy "Starting Hardware Abstraction Layer"
        if [ ! -x /var/cache/hald ] ; then
		install -m755 -g 82 -o 82 -d /var/cache/hald
        fi
	if [ ! -x /var/run/hald ]; then
		install -m755 -g 82 -o 82 -d /var/run/hald
	fi
	if [ ! -x /var/run/hald/hald-local ]; then
		install -m755 -g 0 -o 0 -d /var/run/hald/hald-local
	fi
	if [ ! -x /var/run/hald/hald-runner ]; then
		install -m755 -g 0 -o 0 -d /var/run/hald/hald-runner
	fi
	/usr/sbin/hald
	if [ $? -gt 0 ]; then
		stat_fail
	else
		add_daemon hal
		stat_done
	fi
        ;;
    stop)
	stat_busy "Stopping Hardware Abstraction Layer"
	[ -f /var/run/hald.pid ] && kill `cat /var/run/hald.pid` &> /dev/null
	if [ $? -gt 0 ]; then
		stat_fail
	else
		rm_daemon hal
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
	;;
esac
exit 0
