#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
[ -f /etc/conf.d/samba ] && . /etc/conf.d/samba

[ -z "$SAMBA_DAEMONS" ] && SAMBA_DAEMONS=(smbd nmbd)

case "$1" in
	start)
		rc=0
		stat_busy "Starting Samba Server"
		for d in ${SAMBA_DAEMONS[@]}; do
			PID=`pidof -o %PPID /usr/sbin/$d`
			[ -z "$PID" ] && /usr/sbin/$d -D
			rc=$(($rc+$?))
		done
		if [ $rc -gt 0 ]; then
			stat_fail
		else
			add_daemon samba
			stat_done
		fi
	;;
	stop)
		rc=0
		stat_busy "Stopping Samba Server"
		for d in ${SAMBA_DAEMONS[@]}; do
			PID=`pidof -o %PPID /usr/sbin/$d`
			[ -z "$PID" ] || kill $PID &> /dev/null
			rc=$(($rc+$?))
		done
		if [ $rc -gt 0 ]; then
			stat_fail
		else
			rm /var/run/samba/smbd.pid &>/dev/null
			rm /var/run/samba/nmbd.pid &>/dev/null
			rm /var/run/samba/winbindd.pid &>/dev/null
			rm_daemon samba
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
