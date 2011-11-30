#!/bin/bash

daemon_name=httpd

. /etc/rc.conf
. /etc/rc.d/functions

APACHECTL=/usr/sbin/apachectl

case "$1" in
  start)
    stat_busy "Starting Apache Web Server"
    [ ! -d /var/run/httpd ] && install -d /var/run/httpd
    if $APACHECTL start >/dev/null ; then
      add_daemon $daemon_name
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;

  stop)
    stat_busy "Stopping Apache Web Server"
    if $APACHECTL stop >/dev/null ; then
      rm_daemon $daemon_name
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;

  reload)
    stat_busy "Reloading Apache Web Server"
    if $APACHECTL graceful >/dev/null ; then
      add_daemon $daemon_name
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;

  restart)
    stat_busy "Restarting Apache Web Server"
    if $APACHECTL restart >/dev/null ; then
      add_daemon $daemon_name
      stat_done
    else
      stat_fail
      exit 1
    fi
    ;;

  status)
    stat_busy "Checking Apache Web Server status";
    ck_status $daemon_name
    ;;

  *)
    echo "usage: $0 {start|stop|reload|restart|status}"
esac

exit 0
