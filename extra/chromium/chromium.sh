#!/bin/bash

# Allow users to override command-line options
# Based on Gentoo's chromium package
if [[ -f /etc/chromium/default ]]; then
	. /etc/chromium/default
fi
# Source additional configuration files
for file in /etc/chromium/*; do
	# Don't source /etc/chromium/default again
	[[ $file == /etc/chromium/default ]] && continue

	if [[ -f $file ]]; then
		. $file
	fi
done

# Prefer user defined CHROMIUM_USER_FLAGS (from env) over system
# default CHROMIUM_FLAGS (from /etc/chromium/default)
CHROMIUM_FLAGS=${CHROMIUM_USER_FLAGS:-$CHROMIUM_FLAGS}

export CHROME_WRAPPER=$(readlink -f "$0")
export CHROME_DESKTOP=chromium.desktop

exec /usr/lib/chromium/chromium $CHROMIUM_FLAGS "$@"
