#!/bin/sh

xmessage -buttons reboot,halt "$@" ""

case "$?" in
	101)
		/sbin/reboot
		;;
	102)
		/sbin/halt
		;;
esac
