#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

. /etc/conf.d/uwsgid

case "$1" in
    start)
        stat_busy "Starting uwsgid"
        /usr/bin/uwsgi --daemonize "$UWSGI_DAEMONIZE" --ini "$UWSGI_INI" --pidfile "$UWSGI_PIDFILE" &>/dev/null
        if [ $? -gt 0 ]; then
            stat_fail
        else
            add_daemon uwsgid
            stat_done
        fi
    ;;
    stop)
        stat_busy "Stopping uwsgid"
        kill -QUIT `cat "$UWSGI_PIDFILE"` &>/dev/null
        if [ $? -ne 0 ]; then
            stat_fail
        else
            rm_daemon uwsgid
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
