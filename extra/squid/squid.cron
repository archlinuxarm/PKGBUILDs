#!/bin/bash

PID=`pidof -o %PPID /usr/sbin/squid`
[ -n "$PID" ] && /usr/sbin/squid -k rotate
