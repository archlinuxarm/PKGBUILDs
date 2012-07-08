#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/syslog-ng

checkconfig() {
  if ! syslog-ng -s "${SYSLOG_NG_CHECKOPTS[@]}"; then
    stat_fail
    exit 1
  fi
}

pidfile=/run/syslog-ng.pid
if [[ -r $pidfile ]]; then
  read -r PID < "$pidfile"
  if [[ $PID && ! -d /proc/$PID ]]; then
    # stale pidfile
    unset PID
    rm -f "$pidfile"
  fi
fi

case $1 in
  start)
    stat_busy "Starting Syslog-NG"
    checkconfig
    if [[ -z $PID ]] && /usr/sbin/syslog-ng "${SYSLOG_NG_OPTS[@]}"; then
      add_daemon syslog-ng
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;
  stop)
    stat_busy "Stopping Syslog-NG"
    if [[ $PID ]] && kill $PID &>/dev/null; then
      rm_daemon syslog-ng
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;
  reload)
    stat_busy "Reloading Syslog-NG configuration and re-opening log files"
    if [[ -z $PID ]]; then
      stat_fail
    else
      checkconfig
      if kill -HUP $PID &>/dev/null; then
        stat_done
      else
        stat_fail
        exit 1
      fi
    fi
    ;;
  restart)
    $0 stop
    sleep 1
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart|reload}"
esac
