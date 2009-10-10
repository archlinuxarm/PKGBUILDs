#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

get_pid() {
	pidof /usr/bin/jabberd
}

case "$1" in
  start)
    stat_busy "Starting Jabber daemon"

    [ -f /var/run/jabberd/jabber.pid ] && rm -f /var/run/jabberd/jabber.pid
    PID=`get_pid`
    if [ -z "$PID" ]; then
       /usr/bin/jabberd -c /etc/jabberd/jabber.xml -B >/dev/null 2>/dev/null
      if [ $? -gt 0 ]; then
        stat_fail
        exit 1
      else
        sleep 1 # wait on children
        echo `get_pid` > /var/run/jabberd/jabberd.pid
        add_daemon jabberd
  	sleep 1
        stat_done
      fi
    else
      stat_fail
      exit 1
    fi
    ;;

  stop)
    stat_busy "Stopping Jabber daemon"
    PID=`get_pid`
    [ ! -z "$PID" ] && kill $PID &> /dev/null
    sleep 2
    if [ $? -gt 0 ]; then
      stat_fail
      exit 1
    else
      rm -f /var/run/jabberd/jabberd.pid &> /dev/null
      rm_daemon jabberd
      stat_done
    fi
    ;;

  restart)
    $0 stop
    sleep 3
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"  
esac
exit 0
