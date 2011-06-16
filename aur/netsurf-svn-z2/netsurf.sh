#!/bin/sh
killall mouse-emul > /dev/null 2>&1
mouse-emul -d /dev/input/event0&
NETSURFRES=/usr/share/netsurf/res /usr/bin/nsfb $*
killall mouse-emul > /dev/null 2>&1

