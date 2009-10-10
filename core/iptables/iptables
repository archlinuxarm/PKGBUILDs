#!/bin/bash

# source application-specific settings
[ -f /etc/conf.d/iptables ] && . /etc/conf.d/iptables

# Set defaults if settings are missing
[ -z "$IPTABLES" ] && IPTABLES=/usr/sbin/iptables
[ -z "$IPTABLES_CONF" ] && IPTABLES_CONF=/etc/iptables/iptables.rules

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
	start)
		if [ ! -f $IPTABLES_CONF ]; then
			echo "Cannot load iptables rules: $IPTABLES_CONF is missing!" >&2
			exit 1
		fi
		stat_busy "Starting IP Tables"
		if [ "$IPTABLES_FORWARD" = "1" ]; then
			echo 1 >/proc/sys/net/ipv4/ip_forward
		fi
		if ck_daemon iptables; then
			/usr/sbin/iptables-restore < $IPTABLES_CONF
			if [ $? -gt 0 ]; then
				stat_fail
			else
				add_daemon iptables
				stat_done
			fi
		else
			stat_fail
		fi
		;;
	stop)
		stat_busy "Stopping IP Tables"
		echo 0 >/proc/sys/net/ipv4/ip_forward
		if ! ck_daemon iptables; then
			fail=0
			for table in $(cat /proc/net/ip_tables_names); do
				$IPTABLES -t $table -F &>/dev/null && \
					$IPTABLES -t $table -X &>/dev/null && \
					$IPTABLES -t $table -Z &>/dev/null
				[ $? -gt 0 ] && fail=1
			done
			if [ $fail -gt 0 ]; then
				stat_fail
			else
				rm_daemon iptables
				# reset policies
				for table in filter nat mangle raw; do
					if grep -qw $table /proc/net/ip_tables_names; then
						$IPTABLES -t $table -P OUTPUT ACCEPT
					fi
				done
				for table in filter mangle; do
					if grep -qw $table /proc/net/ip_tables_names; then
						$IPTABLES -t $table -P INPUT ACCEPT
						$IPTABLES -t $table -P FORWARD ACCEPT
					fi
				done
				for table in nat mangle raw; do
					if grep -qw $table /proc/net/ip_tables_names; then
						$IPTABLES -t $table -P PREROUTING ACCEPT
					fi
				done
				for table in nat mangle; do
					if grep -qw $table /proc/net/ip_tables_names; then
						$IPTABLES -t $table -P POSTROUTING ACCEPT
					fi
				done
				stat_done
			fi
		else
			stat_fail
		fi
		;;
	restart)
		$0 stop
		sleep 2
		$0 start
		;;
	save)
		stat_busy "Saving IP Tables"
		/usr/sbin/iptables-save >$IPTABLES_CONF
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
