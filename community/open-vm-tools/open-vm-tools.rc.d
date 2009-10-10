#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

# source application-specific settings
[ -f /etc/conf.d/open-vm-tools ] && . /etc/conf.d/open-vm-tools

PID=`pidof -o %PPID /usr/bin/vmtoolsd`
case "$1" in
  start)
    stat_busy "Starting Open Virtual Machine Tools"

    if [ "$VM_DRAG_AND_DROP" == "yes" ]; then
	VMBLOCK=`grep -q -w vmblock /proc/modules`
	[ -z "$VMBLOCK" ] && modprobe vmblock
	if [ $? -gt 0 ]; then
    	    stat_fail
	    exit 1
	fi
	
	DND_TMPDIR="/tmp/VMwareDnD"
	if [ ! -d "$DND_TMPDIR" ]; then
	    mkdir $DND_TMPDIR
	    chown 1777 $DND_TMPDIR
	fi
	
	mount -t vmblock none /proc/fs/vmblock/mountPoint
	if [ $? -gt 0 ]; then
    	    stat_fail
	    exit 1
	fi
    fi
    
    VMHGFS=`grep -q -w vmhgfs /proc/modules`
    [ -z "$VMHGFS" ] && modprobe vmhgfs
    if [ $? -gt 0 ]; then
        stat_fail
        exit 1
    fi
    
    VMMEMCTL=`grep -q -w vmmemctl /proc/modules`
    [ -z "$VMMEMCTL" ] && modprobe vmmemctl
    if [ $? -gt 0 ]; then
        stat_fail
        exit 1
    fi

    VMSYNC=`grep -q -w vmsync /proc/modules`
    [ -z "$VMSYNC" ] && modprobe vmsync
    if [ $? -gt 0 ]; then
        stat_fail
        exit 1
    fi
    
    [ -z "$PID" ] && /usr/bin/vmtoolsd --background $PIDFILE
    if [ $? -gt 0 ]; then
      stat_fail
    else
      add_daemon open-vm-tools
      stat_done
    fi
    ;;
  stop)
    stat_busy "Stopping Open Virtual Machine Tools"
    [ ! -z "$PID" ]  && kill $PID &>/dev/null
    if [ $? -gt 0 ]; then
      stat_fail
      exit 1
    fi

    VMSYNC=`grep -q -w vmsync /proc/modules`
    [ -z "$VMSYNC" ] && rmmod vmsync
    if [ $? -gt 0 ]; then
      stat_fail
      exit 2
    fi
    
    VMMEMCTL=`grep -q -w vmmemctl /proc/modules`
    [ -z "$VMMEMCTL" ] && rmmod vmmemctl
    if [ $? -gt 0 ]; then
      stat_fail
      exit 3
    fi

    VMHGFS=`grep -q -w vmhgfs /proc/modules`
    [ -z "$VMHGFS" ] && rmmod vmhgfs
    if [ $? -gt 0 ]; then
      stat_fail
      exit 4
    fi

    if [ "$VM_DRAG_AND_DROP" == "yes" ]; then
	MOUNTPOINT=`grep -q -w "none /proc/fs/vmblock/mountPoint vmblock" /proc/modules`
	[ -z "$MOUNTPOINT" ] && umount /proc/fs/vmblock/mountPoint
        if [ $? -gt 0 ]; then
	  stat_fail
    	  exit 5
	fi
	
	DND_TMPDIR="/tmp/VMwareDnD"
	rm -r $DND_TMPDIR
	
	VMBLOCK=`grep -q -w vmblock /proc/modules`
	[ -z "$VMBLOCK" ] && rmmod vmblock
        if [ $? -gt 0 ]; then
	  stat_fail
    	  exit 6
	fi
    fi
        
    rm_daemon open-vm-tools
    stat_done
    ;;
  restart)
    $0 stop
    $0 start
    ;;
  *)
    echo "usage: $0 {start|stop|restart}"  
esac
exit 0
