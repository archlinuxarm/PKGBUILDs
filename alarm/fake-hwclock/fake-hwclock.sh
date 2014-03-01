#!/bin/bash

THISFILE=$0

setclock() {
	echo "Setting clock."
	MYTIME=$(date -r $THISFILE '+%Y-%m-%d %H:%M:%S')
	date --set="$MYTIME" &>/dev/null
}

saveclock() {
	echo "Saving current time."
	touch $THISFILE &>/dev/null
}

case "$1" in
	set)
		setclock
		;;
	save)
		saveclock
		;;
	*)
		echo "Usage: $THISFILE {set|save}"
		exit 1
		;;
esac
