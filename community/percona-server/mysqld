#!/bin/bash

# general config
. /etc/rc.conf
. /etc/rc.d/functions

getPID() {
   echo $(pgrep -u mysql mysqld 2>/dev/null);
}

case "$1" in
  start)
    stat_busy "Starting Percona Server"
    [ ! -d /var/run/mysqld ] && install -d -g mysql -o mysql /var/run/mysqld &>/dev/null
    if [ -z "$(getPID)" ]; then
       /usr/bin/mysqld_safe --user=mysql &>/dev/null &
      if [ $? -gt 0 ]; then
        stat_fail
        exit 1
      else
        timeo=30
        while [ $timeo -gt 0 ]; do
          response=`/usr/bin/mysqladmin -uUNKNOWN_USER ping 2>&1` && break
          echo "$response" | grep -q "mysqld is alive" && break
          sleep 1
          let timeo=${timeo}-1
        done
        if [ $timeo -eq 0 ]; then
          stat_fail
          exit 1
        else
          echo $(getPID) > /var/run/mysqld/mysqld.pid
          add_daemon mysqld
          stat_done
        fi
      fi
    else
      stat_fail
      exit 1
    fi
    ;;

  stop)
    stat_busy "Stopping Percona Server"
    if [ ! -z "$(getPID)" ]; then
      timeo=30
      kill $(getPID) &> /dev/null
      if [ $? -gt 0 ]; then
        stat_fail
        exit 1
      fi
      while [ ! -z "$(getPID)" -a $timeo -gt 0 ]; do
        sleep 1
        let timeo=${timeo}-1
      done
      if [ -z "$(getPID)" ]; then
        rm -f /var/run/mysqld/mysqld.pid &>/dev/null
        rm_daemon mysqld
        stat_done
      else
        stat_fail
        exit 1
      fi
    else
      stat_fail
      exit 1
    fi
    ;;

  restart)
    $0 stop
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"
esac
exit 0
