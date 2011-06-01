#!/bin/bash
daemon_name=dropbear

. /etc/rc.conf
. /etc/rc.d/functions

. /etc/conf.d/$daemon_name

for port in $DROPBEAR_PORT; do
  daemon_args="$daemon_args -p $port"
done

[ ! -z $DROPBEAR_BANNER ] && daemon_args="$daemon_args -b $DROPBEAR_BANNER"
[ ! -z $DROPBEAR_DSSKEY ] && daemon_args="$daemon_args -d $DROPBEAR_DSSKEY"
[ ! -z $DROPBEAR_RSAKEY ] && daemon_args="$daemon_args -r $DROPBEAR_RSAKEY"
[ ! -z $DROPBEAR_EXTRA_ARGS ] && daemon_args="$daemon_args $DROPBEAR_EXTRA_ARGS"
[ -z $DROPBEAR_PIDFILE ] && DROPBEAR_PIDFILE="/var/run/$daemon_name.pid"
daemon_args="$daemon_args -P $DROPBEAR_PIDFILE"

get_pid() {
  PID=''
  if [ -r $DROPBEAR_PIDFILE -a -f $DROPBEAR_PIDFILE -a -w $DROPBEAR_PIDFILE ]; then
    if kill -0 "`< $DROPBEAR_PIDFILE`" &>/dev/null; then # kill -0 == "exit code indicates if a signal may be sent"
      PID="`< $DROPBEAR_PIDFILE`"
    else # may not send signals to dropbear, because it's probably not running => remove pidfile
      rm -f $DROPBEAR_PIDFILE
    fi
  fi
}

case "$1" in
  start)
    stat_busy "Starting $daemon_name"
    get_pid
    if [ -z "$PID" ]; then
      printhl "Checking for hostkeys"
      if [ ! -z $DROPBEAR_DSSKEY ]; then
        [ ! -f $DROPBEAR_DSSKEY ] && dropbearkey -t dss -f $DROPBEAR_DSSKEY
      fi;
      if [ ! -z $DROPBEAR_RSAKEY ]; then
        [ ! -f $DROPBEAR_RSAKEY ] && dropbearkey -t rsa -f $DROPBEAR_RSAKEY
      fi;

      $daemon_name $daemon_args # Make it Go Joe!
      if [ $? -gt 0 ]; then
        stat_die
      else
        add_daemon $daemon_name
        stat_done
      fi
    else
      stat_die
    fi
    ;;

  stop)
    stat_busy "Stopping $daemon_name"

    get_pid
    [ ! -z "$PID" ] && kill $PID &> /dev/null # Be dead (please), I say!
    if [ $? -gt 0 ]; then
      stat_die
    else
      rm_daemon $daemon_name
      stat_done
    fi
    ;;

  restart)
    $0 stop
    sleep 3
    $0 start
    ;;

  fingerprint)
    stat_busy "Fingerprinting $daemon_name hostkeys"
    if [ ! -z $DROPBEAR_DSSKEY ]; then
      printhl "DSS/DSA Key $(dropbearkey -y -f $DROPBEAR_DSSKEY | grep Fingerprint)"
    fi;
    if [ ! -z $DROPBEAR_RSAKEY ]; then
      printhl "RSA Key $(dropbearkey -y -f $DROPBEAR_RSAKEY | grep Fingerprint)"
    fi;
  ;;

  *)
    echo "usage: $0 {start|stop|restart|fingerprint}"
esac
exit 0
