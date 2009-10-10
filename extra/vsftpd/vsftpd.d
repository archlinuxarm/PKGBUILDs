#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/vsftpd`
case "$1" in
	start)
		stat_busy "Starting vsftpd FTP Daemon"
		if [ -z "$PID" ]; then 
			/usr/sbin/vsftpd &
			PID=`pidof -o %PPID /usr/sbin/vsftpd`
			if [ -z $PID ]; then
				stat_fail
			else
				add_daemon vsftpd
				stat_done
			fi
		else
			stat_fail
		fi
	;;
	stop)
		stat_busy "Stopping vsftpd FTP Daemon"
		[ ! -z "$PID" ]  && kill $PID &> /dev/null
		if [ $? -gt 0 ]; then
			stat_fail
		else
			rm_daemon vsftpd
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
