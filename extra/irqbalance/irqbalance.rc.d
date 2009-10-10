#!/bin/bash

# source application-specific settings
ONESHOT=0
[ -f /etc/conf.d/irqbalance ] && . /etc/conf.d/irqbalance

if [ "$ONESHOT" -ne 0 ]; then
	ONESHOT_CMD="oneshot"
fi

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/sbin/irqbalance`
case "$1" in
  start)
    stat_busy "Starting IRQ balancing"
    [ -z "$PID" ] && /usr/sbin/irqbalance $ONESHOT_CMD
    if [ $? -gt 0 ]; then
      stat_fail
    else
      if [ "$ONESHOT" -eq 0 ]; then
        add_daemon irqbalance
      fi
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping IRQ balancing"
    [ ! -z "$PID" ] && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon irqbalance
      stat_done
    fi
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
