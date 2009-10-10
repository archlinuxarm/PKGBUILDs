CI_DAEMONS=
[ -f /etc/conf.d/courier-imap ] && . /etc/conf.d/courier-imap

. /etc/rc.conf
. /etc/rc.d/functions

case "$1" in
  start)
    if [ ! -f /var/run/daemons/authdaemond ]; then
      echo "ERROR: authdaemond is not running"
      stat_fail
      exit 1
    fi
    for daemon in $CI_DAEMONS; do
      stat_busy "Starting Courier ${daemon}"
      /usr/lib/courier-imap/${daemon}.rc start
      if [ $? -gt 0 ]; then
        stat_fail
      else
        add_daemon $daemon
        stat_done
      fi
    done
    ;;
  stop)
    for daemon in $CI_DAEMONS; do
      stat_busy "Stopping Courier ${daemon}"
      /usr/lib/courier-imap/$daemon.rc stop > /dev/null
      if [ $? -gt 0 ]; then
        stat_fail
      else
        rm_daemon $daemon
        stat_done
      fi
    done
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
~                                                                                                                                                         
~         
