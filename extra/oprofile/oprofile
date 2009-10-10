#!/bin/bash
# Contributed by basilburn

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/oprofile

PID=`pidof -o %PPID /usr/sbin/oprofiled`
case "$1" in
  start)
    stat_busy "Starting Profiler"
    [ -z "$PID" ] && /usr/bin/opcontrol --init && \
      /usr/bin/opcontrol --start $OPTIONS &>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      echo $PID > /var/run/oprofiled.pid
      add_daemon oprofiled
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Profiler"
    [ ! -z "$PID" ]  && /usr/bin/opcontrol --shutdown &>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm /var/run/oprofiled.pid
      rm_daemon oprofiled
      stat_done
    fi
    ;;
  restart)
    $0 stop
    sleep 2
    $0 start
    ;;
  reset)
    /usr/bin/opcontrol --reset 
    $0 restart
    ;;
 *)
    echo "usage: $0 {start|stop|restart|reset}"  
esac
exit 0
