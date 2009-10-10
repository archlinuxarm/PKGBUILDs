#!/bin/bash

[ -f /etc/conf.d/dbmail ] && . /etc/conf.d/dbmail

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
    start)
	for daemon in $DBMAIL_DAEMONS; do
	    stat_busy "Starting DbMail ${daemon}"
	    /usr/sbin/${daemon}
	    if [ $? -gt 0 ]; then
		stat_fail
	    else
		stat_done
	    fi
	done
	add_daemon dbmail
	;;
    stop)
	for daemon in $DBMAIL_DAEMONS; do
	    stat_busy "Stopping DbMail ${daemon}"
	    pid=$(cat /var/run/${daemon}.pid)
	    kill $pid
	    sleep 4
	    stat_done
	done
	rm_daemon dbmail
	;;
    restart)
	$0 stop
	sleep 3
	$0 start
	;;
    *)
	echo "usage: $0 {start|stop|restart}"
	;;
esac

exit 0
