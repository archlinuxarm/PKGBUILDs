#!/bin/bash

. /usr/lib/pm-utils/functions

suspend_netcfg() {
	netcfg2 all-suspend
}

resume_netcfg() {
	netcfg2 all-resume
}

if [ -x /usr/bin/netcfg2 ]; then
	case "$1" in
		hibernate|suspend)
			suspend_netcfg
			;;
		thaw|resume)
			resume_netcfg
			;;
		*)
			;;
	esac
fi

exit $?
