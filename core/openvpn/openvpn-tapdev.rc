#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

. /etc/conf.d/openvpn-tapdev

case "$1" in
  start)
    stat_busy "Creating tap devices for OpenVPN ... "
    success=0
    for tapdev in ${TAPDEVS}; do
      stat_append "${tapdev} "
      /usr/sbin/openvpn --mktun --dev-type tap --dev ${tapdev} >/dev/null 2>&1 || success=$?
    done
    if [ $success -eq 0 ]; then
      add_daemon openvpn-tapdev
      stat_done
    else
      stat_fail
    fi
    ;;
  stop)
    stat_busy "Destroying tap devices for OpenVPN ..."
    for tapdev in ${TAPDEVS}; do
      stat_append "${tapdev} "
      /usr/sbin/openvpn --rmtun --dev-type tap --dev ${tapdev} >/dev/null 2>&1 || success=$?
    done
    rm_daemon openvpn-tapdev
    stat_done
    ;;
  restart)
    $0 stop
    sleep 1
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"  
esac
exit 0
