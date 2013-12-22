#!/bin/bash

MYSELF=$0

setclock() {
	echo "Setting clock."
	MYTIME=$(date -r $MYSELF '+%Y-%m-%d %r')
	date --set="$MYTIME" &>/dev/null
}

saveclock() {
	echo "Saving current time."
	touch $MYSELF &>/dev/null
}

case "$1" in
	set)
		setclock
		;;
	save)
		saveclock
		;;
	*)
		echo "Usage: $MYSELF {set|save}"
		exit 1
		;;
esac
