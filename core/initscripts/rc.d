#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

usage() {
	local name=${0##*/}
	cat >&2 << EOF
usage: $name action daemon ...

e.g: $name list
     $name help
     $name start sshd gpm
EOF
	exit 1
}

(( $# < 1 )) && usage

declare -i ret=0
case $1 in
	help)
		usage
		;;
	list)
		cd /etc/rc.d/
		for d in *; do
			have_daemon "$d" || continue
			# print running / stopped satus
			if ! ck_daemon "$d"; then
				printf "${C_OTHER}[${C_DONE}STARTED${C_OTHER}]"
			else
				printf "${C_OTHER}[${C_FAIL}STOPPED${C_OTHER}]"
			fi
			# print auto / manual status
			if ! ck_autostart "$d"; then
				printf "${C_OTHER}[${C_DONE}AUTO${C_OTHER}]"
			else
				printf "${C_OTHER}[${C_FAIL}    ${C_OTHER}]"
			fi
			printf " ${C_CLEAR}$d\n"
		done
		;;
	*)
		action=$1
		shift
		# set same environment variables as init
		runlevel=$(/sbin/runlevel)
		ENV="PATH='/bin:/usr/bin:/sbin:/usr/sbin'"
		ENV+=" PREVLEVEL='${runlevel:0:1}'"
		ENV+=" RUNLEVEL='${runlevel:2:1}'"
		ENV+=" CONSOLE='${CONSOLE:-/dev/console}'"
		for i; do
			[[ -x "/etc/rc.d/$i" ]] && cd / && eval /usr/bin/env -i $ENV "/etc/rc.d/$i" "$action"
			(( ret += !! $? ))  # clamp exit value to 0/1
		done
esac

exit $ret

# vim: set ts=2 sw=2 noet:
