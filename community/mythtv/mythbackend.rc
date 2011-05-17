#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

# Default values to use if none are supplied in the config file.

# Running mythbackend as non-root requires you to ensure that audio/video
# devices used for recording have suitable user permissions. One way
# to achieve this is to modify existing or create new udev rules which
# assign these devices to a non-root group with rw permissions and add
# your mythbackend user to that group. Be aware that console.perms can
# also affect device permissions and may need additional configuration.
# Running as non-root may also introduce increased process latency.
#
# User who should start the mythbackend processes
MBE_USER='root'

# Startup options for mythbackend
MBE_OPTIONS=''

# Name of mythbackend log file
LOG_FILE='/var/log/mythbackend.log'

# Logging options for mythbackend (empty means '-v important,general')
LOG_OPTS=''

###############################################################################

CONFIG_FILE=/etc/conf.d/mythbackend
PIDFILE=/var/run/mythbackend.pid

if [[ -f ${CONFIG_FILE} ]]; then
    . ${CONFIG_FILE}
fi

pid="$(cat ${PIDFILE} 2>/dev/null || pidof mythbackend)";

# fix FS#11890 
mbe_user_home="$(getent passwd ${MBE_USER}|cut -d : -f 6)"

case "$1" in
    start)
	stat_busy "Starting MythTV Backend"

	# already running ?
	if [[ "${pid}" -gt  0 ]] && kill -0 "${pid}"; then
	    stat_fail
	    exit 1;
	fi	    
	touch ${PIDFILE} ${LOG_FILE}
	chown "$MBE_USER" ${PIDFILE} ${LOG_FILE}
	if su "$MBE_USER" -c "HOME=${mbe_user_home} mythbackend \
	--daemon \
        --logfile $LOG_FILE $LOG_OPTS \
        --pidfile ${PIDFILE} $MBE_OPTIONS"; 
	then
	    add_daemon mythbackend
	    stat_done
	else
	    stat_fail
	fi
	;;
    
    stop)
	stat_busy "Stopping MythTV Backend"
	if [[ "${pid}" -gt 0 ]] && kill $pid &>/dev/null; then
	    rm_daemon mythbackend
	    stat_done
	    rm ${PIDFILE} 2>/dev/null
	else
	    stat_fail
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
