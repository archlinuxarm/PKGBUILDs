#!/bin/bash

# source application-specific settings
[ -f /etc/conf.d/apache13 ] && . /etc/conf.d/apache13

# general config
. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
  start)
    stat_busy "Starting HTTP Daemon"
    if [ "$HTTPD_USE_SSL" = "yes" -o "$HTTPD_USE_SSL" = "YES" ]; then
      apachectl13 startssl &>/dev/null
    else
      apachectl13 start &>/dev/null
    fi
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon httpd
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping HTTP Daemon"
    apachectl13 stop &>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
    else
      rm_daemon httpd
      stat_done
    fi
    ;;
  restart)
    $0 stop
    sleep 3
    $0 start
    ;;
  reload)
    if [ -f /var/run/apache13/httpd.pid ]; then
      status "Reloading HTTP Configuration" kill -HUP `cat /var/run/apache13/httpd.pid`
    fi
    ;;
  *)
    echo "usage: $0 {start|stop|restart|reload}"  
esac
