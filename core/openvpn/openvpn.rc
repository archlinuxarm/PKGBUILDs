#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

CFGDIR="/etc/openvpn"
STATEDIR="/var/run/openvpn"

case "$1" in
  start)
    stat_busy "Starting OpenVPN ... "
    success=0
    mkdir -p "${STATEDIR}"
    for cfg in "${CFGDIR}"/*.conf; do
      stat_append "$(basename "${cfg}" .conf) "
      /usr/sbin/openvpn --daemon --writepid "${STATEDIR}"/"$(basename "${cfg}" .conf)".pid --cd "${CFGDIR}" --config "${cfg}" || success=$?
    done
    if [ $success -eq 0 ]; then
      add_daemon openvpn
      stat_done
    else
      stat_fail
    fi
    ;;
  stop)
    stat_busy "Stopping OpenVPN ..."
    for pidfile in "${STATEDIR}"/*.pid; do
      stat_append "$(basename "${pidfile}" .pid) "
      kill $(cat "${pidfile}" 2>/dev/null) 2>/dev/null
      rm -f "${pidfile}"
    done
    rm_daemon openvpn
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
