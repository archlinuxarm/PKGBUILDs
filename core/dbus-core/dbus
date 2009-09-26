#!/bin/bash

# general config
. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
    start)
        stat_busy "Starting D-BUS system messagebus"
	if [ ! -x /var/run/dbus ] ; then
	    install -m755 -g 81 -o 81 -d /var/run/dbus
	fi
        if [ -x /usr/bin/dbus-uuidgen ] ; then
            /usr/bin/dbus-uuidgen --ensure
        fi

        /usr/bin/dbus-daemon --system
        if [ $? -gt 0 ]; then
            stat_fail
        else
            add_daemon dbus
            stat_done
        fi
        ;;
    stop)
        stat_busy "Stopping D-BUS system messagebus"
	[ -f /var/run/dbus.pid ] && kill `cat /var/run/dbus.pid` >/dev/null 2>&1
        if [ $? -gt 0 ]; then
            stat_fail
        else
	    rm -f /var/run/dbus.pid
            rm_daemon dbus
            stat_done
        fi
        ;;
    restart)
        $0 stop
	sleep 1
        $0 start
        ;;
    reload)
        stat_busy "Reloading D-BUS configuration"
        [ -f /var/run/dbus.pid ] && /usr/bin/dbus-send \
                --system --type=method_call \
                --dest=org.freedesktop.DBus \
                / org.freedesktop.DBus.ReloadConfig
        if [ $? -gt 0 ]; then
            stat_fail
        else
            stat_done
        fi
        ;;
    *)
        echo "usage: $0 {start|stop|restart|reload}"
	;;
esac
exit 0
