#!/bin/bash
LMCTL_ARGS=
[ -f /etc/conf.d/lmctl ] && . /etc/conf.d/lmctl

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
  start)
    stat_busy "Setting up Logitech Mouse"
    /usr/bin/lmctl ${LMCTL_ARGS} &>/dev/null
    if [ $? -eq 0 ] ; then
      stat_fail
    else
      stat_done
      add_daemon lmctl
    fi
    ;;
  *)
    echo "usage: $0 {start}"  
esac
