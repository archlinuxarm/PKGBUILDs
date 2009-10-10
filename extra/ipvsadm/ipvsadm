#!/bin/bash

# source application-specific settings
[ -f /etc/conf.d/ipvsadm ] && . /etc/conf.d/ipvsadm

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
	start)
		if [ ! -f $IPVSADM_CONF ]; then
			echo "Cannot load ipvsadm rules: $IPVSADM_CONF is missing!" >&2
			exit 1
		fi
		status "Clearing current IPVS table" /sbin/ipvsadm -C
		stat_busy "Applying IPVS configuration"
		/sbin/ipvsadm-restore <$IPVSADM_CONF
		if [ $? -gt 0 ]; then
			stat_fail
		else
			add_daemon ipvsadm
			stat_done
		fi
		;;
	stop)
		status "Clearing current IPVS table" /sbin/ipvsadm -C
		rm_daemon ipvsadm
		;;
	restart)
		# no need to stop, start will clear the table for us
		$0 start
		;;
	save)
		stat_busy "Saving IPVS configuration"
		/sbin/ipvsadm-save -n >$IPVSADM_CONF 2>/dev/null
		if [ $? -gt 0 ]; then
			stat_fail
		else
			stat_done
		fi
		;;
	*)
		echo "usage: $0 {start|stop|restart|save}"
esac
exit 0
