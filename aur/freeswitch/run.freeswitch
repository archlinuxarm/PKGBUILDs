#!/bin/sh -e
. /etc/conf.d/freeswitch
[ -d /run/freeswitch ] || mkdir /run/freeswitch
chown freeswitch /run/freeswitch
ulimit -c unlimited # The maximum size of core files created.
ulimit -d unlimited # The maximum size of a process's data segment.
ulimit -f unlimited # The maximum size of files created by the shell (default option)
ulimit -i unlimited # The maximum number of pending signals
ulimit -n 999999    # The maximum number of open file descriptors.
ulimit -q unlimited # The maximum POSIX message queue size
ulimit -u unlimited # The maximum number of processes available to a single user.
ulimit -v unlimited # The maximum amount of virtual memory available to the process.
ulimit -x unlimited # ???
ulimit -l unlimited # The maximum size that may be locked into memory.
ulimit -s 240      # The maximum stack size
ulimit -a           # All current limits are reported.
echo "Starting Freeswitch with ${FREESWITCH_OPTS}"
exec chpst -u freeswitch:daemon /usr/bin/freeswitch -u freeswitch -g daemon -nf -nc ${FREESWITCH_OPTS} 2>&1

