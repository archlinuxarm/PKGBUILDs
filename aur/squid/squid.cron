#!/bin/sh

# exit without error if no pidfile exists
{ read pid </run/squid.pid; } 2>/dev/null || exit 0

# make sure found PID really is a squid process
if [ /proc/$pid/exec -ef /usr/sbin/squid ]; then
  /usr/sbin/squid -k rotate
fi
