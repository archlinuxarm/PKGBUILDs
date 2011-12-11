#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/mediatomb

MT_OPTIONS="-p $MT_PORT -u $MT_USER -g $MT_GROUP -P $MT_PIDFILE \
            -l $MT_LOGFILE -m $MT_HOME -f $MT_CFGDIR $MT_OPTIONS"

case "$1" in
    start)
        stat_busy "Starting Mediatomb UPnP Media Server"

        chown "$MT_USER:$MT_GROUP" "$MT_HOME"

        if ! pidof -o %PPID /usr/bin/mediatomb &> /dev/null; then
            rm -f "$MT_PIDFILE"
        fi

        PID="$(cat "$MT_PIDFILE" 2> /dev/null)"

        if [ -z "$PID" ] && /usr/bin/mediatomb -d $MT_OPTIONS; then
            add_daemon mediatomb
            stat_done
        else
            stat_fail
        fi
        ;;

    stop)
        stat_busy "Stopping Mediatomb UPnP Media Server"

        PID="$(cat "$MT_PIDFILE" 2> /dev/null)"

        if [ -n "$PID" ] && kill "$PID" &> /dev/null; then
            rm -f "$MT_PIDFILE"
            rm_daemon mediatomb
            stat_done
        else
            stat_fail
        fi
        ;;

    restart)
        "$0" stop
        sleep 1
        "$0" start
        ;;

    *)
        echo "usage: $0 {start|stop|restart}"
        ;;
esac
exit 0
