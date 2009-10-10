#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

PID=`pidof -o %PPID /usr/bin/timidity`
case "$1" in
   start)
     stat_busy "Starting Timidity++ ALSA Daemon"
     [ -z "$PID" ] && /usr/bin/timidity -iAD > /dev/null
     if [ $? -gt 0 ]; then
       stat_fail
     else
       echo $PID > /var/run/timidity.pid
       add_daemon timidity++
       stat_done
     fi
     ;;
   stop)
     stat_busy "Stopping Timidity++ ALSA Daemon"
     [ ! -z "$PID" ]  && kill $PID &> /dev/null
     if [ $? -gt 0 ]; then
       stat_fail
     else
       rm /var/run/timidity.pid
       rm_daemon timidity++
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
