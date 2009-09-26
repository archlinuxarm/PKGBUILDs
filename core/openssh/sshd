#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions
. /etc/conf.d/sshd

PID=`cat /var/run/sshd.pid 2>/dev/null`
case "$1" in
  start)
    stat_busy "Starting Secure Shell Daemon"
    [ -f /etc/ssh/ssh_host_key ] || { /usr/bin/ssh-keygen -t rsa1 -N "" -f /etc/ssh/ssh_host_key >/dev/null; }
    [ -f /etc/ssh/ssh_host_rsa_key ] || { /usr/bin/ssh-keygen -t rsa -N "" -f /etc/ssh/ssh_host_rsa_key >/dev/null; }
    [ -f /etc/ssh/ssh_host_dsa_key ] || { /usr/bin/ssh-keygen -t dsa -N "" -f /etc/ssh/ssh_host_dsa_key >/dev/null; }
    [ -z "$PID" ] && /usr/sbin/sshd $SSHD_ARGS
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon sshd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Secure Shell Daemon"
    [ ! -z "$PID" ]  && kill $PID &> /dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon sshd
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
