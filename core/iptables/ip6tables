#!/bin/bash

# source application-specific settings
[ -f /etc/conf.d/iptables ] && . /etc/conf.d/iptables

# Set defaults if settings are missing
[ -z "$IP6TABLES" ] && IP6TABLES=/usr/sbin/ip6tables
[ -z "$IP6TABLES_CONF" ] && IP6TABLES_CONF=/etc/iptables/ip6tables.rules

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
	start)
		if [ ! -f $IP6TABLES_CONF ]; then
			echo "Cannot load iptables rules: $IP6TABLES_CONF is missing!" >&2
			exit 1
		fi
		stat_busy "Starting IP6 Tables"
		if [ "$IPTABLES_FORWARD" = "1" ]; then
                        echo 1 >/proc/sys/net/ipv6/conf/default/forwarding
                        echo 1 >/proc/sys/net/ipv6/conf/all/forwarding
		fi
		if ck_daemon ip6tables; then
			/usr/sbin/ip6tables-restore < $IP6TABLES_CONF
			if [ $? -gt 0 ]; then
				stat_fail
			else
				add_daemon ip6tables
				stat_done
			fi
		else
			stat_fail
		fi
		;;
	stop)
		stat_busy "Stopping IP6 Tables"
                echo 0 >/proc/sys/net/ipv6/conf/all/forwarding
                echo 0 >/proc/sys/net/ipv6/conf/default/forwarding
		if ! ck_daemon ip6tables; then
			fail=0
			for table in $(cat /proc/net/ip6_tables_names); do
				$IP6TABLES -t $table -F &>/dev/null && \
					$IP6TABLES -t $table -X &>/dev/null && \
					$IP6TABLES -t $table -Z &>/dev/null
				[ $? -gt 0 ] && fail=1
			done
			if [ $fail -gt 0 ]; then
				stat_fail
			else
				rm_daemon ip6tables
				# reset policies
				for table in filter mangle raw; do
					if grep -qw $table /proc/net/ip6_tables_names; then
						$IP6TABLES -t $table -P OUTPUT ACCEPT
					fi
				done
				for table in filter mangle; do
					if grep -qw $table /proc/net/ip6_tables_names; then
						$IP6TABLES -t $table -P INPUT ACCEPT
						$IP6TABLES -t $table -P FORWARD ACCEPT
					fi
				done
				for table in mangle raw; do
					if grep -qw $table /proc/net/ip6_tables_names; then
						$IP6TABLES -t $table -P PREROUTING ACCEPT
					fi
				done
				for table in mangle; do
					if grep -qw $table /proc/net/ip6_tables_names; then
						$IP6TABLES -t $table -P POSTROUTING ACCEPT
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
		stat_busy "Saving IP6 Tables"
		/usr/sbin/ip6tables-save >$IP6TABLES_CONF
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
