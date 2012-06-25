#!/bin/bash
#
# Start/stop the Bluetooth daemons
#

. /etc/rc.conf
. /etc/rc.d/functions

DAEMON_NAME="bluetoothd"
HIDD_NAME="hidd"
RFCOMM_NAME="rfcomm"
PAND_NAME="pand"
DUND_NAME="dund"

DAEMON_EXEC="/usr/sbin/bluetoothd"
HIDD_EXEC="/usr/bin/hidd"
RFCOMM_EXEC="/usr/bin/rfcomm"
PAND_EXEC="/usr/bin/pand"
DUND_EXEC="/usr/bin/dund"

DAEMON_ENABLE="true"
HIDD_ENABLE="false"
RFCOMM_ENABLE="false"
PAND_ENABLE="false"
DUND_ENABLE="false"

RFCOMM_CONFIG="/etc/bluetooth/rfcomm.conf"

HIDD_OPTIONS=""
PAND_OPTIONS=""
DUND_OPTIONS=""

[ -f /etc/conf.d/bluetooth ] && . /etc/conf.d/bluetooth

case "$1" in
  start)
    stat_busy "Starting bluetooth subsystem:"
    if [ "$DAEMON_ENABLE" = "true" -a -x "$DAEMON_EXEC" ] ; then
      stat_append " $DAEMON_NAME"
      $DAEMON_EXEC
      sleep 1
    fi
    if [ "$HIDD_ENABLE" = "true" -a -x "$HIDD_EXEC" ]; then
      stat_append " $HIDD_NAME"
      $HIDD_EXEC $HIDD_OPTIONS
    fi
    if [ "$RFCOMM_ENABLE" = "true" -a -x "$RFCOMM_EXEC" -a -f "$RFCOMM_CONFIG" ]; then
      stat_append " $RFCOMM_NAME"
      $RFCOMM_EXEC -f $RFCOMM_CONFIG bind all
    fi
    if [ "$PAND_ENABLE" = "true" -a -x "$PAND_EXEC" -a -n "$PAND_OPTIONS" ]; then
      stat_append " $PAND_NAME"
      $PAND_EXEC $PAND_OPTIONS
    fi
    if [ "$DUND_ENABLE" = "true" -a -x "$DUND_EXEC" -a -n "$DUND_OPTIONS" ]; then
      stat_append " $DUND_NAME"
      $DUND_EXEC $DUND_OPTIONS
    fi
    add_daemon bluetooth
    stat_done
    ;;
  stop)
    stat_busy "Stopping bluetooth subsystem:"

    stat_append " $DUND_NAME"
    killall $DUND_NAME >/dev/null 2>&1

    stat_append " $PAND_NAME"
    killall $PAND_NAME >/dev/null 2>&1

    if [ -x "$RFCOMM_EXEC" ]; then
      stat_append " $RFCOMM_NAME"
      $RFCOMM_EXEC release all >/dev/null 2>&1
    fi

    stat_append " $HIDD_NAME"
    killall $HIDD_NAME >/dev/null 2>&1

    stat_append " $DAEMON_NAME"
    killall $DAEMON_NAME >/dev/null 2>&1

    rm_daemon bluetooth
    stat_done
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
