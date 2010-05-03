#! /bin/sh
# Creates cdsymlinks in /dev
# for Archlinux by Tobias Powalowski <tpowa@archlinux.org>

# check on cd/dvd drives and if persistant rules file is used
if [ -d /dev/cd -a ! -e /etc/udev/rules.d/75-persistent-cd.rules ]; then
	# remove existing device files
	rm /dev/cdrom*
	rm /dev/cdrw*
	rm /dev/dvd*
	rm /dev/dvdrw*
	# start creating symlinks
	for i in /dev/cd/cdrom-*; do
		if [ -h $i ]; then
		[ "$CD_NUMBER" = "" ] && ln -s $i /dev/cdrom
		[ "$CD_NUMBER" = "" ] && CD_NUMBER="-1"
		! [ "$CD_NUMBER" = "" ] && CD_NUMBER="$((CD_NUMBER+1))" && ln -s $i /dev/cdrom$CD_NUMBER
		fi
	done
	
	for i in /dev/cd/cdrw-*; do
		if [ -h $i ]; then
		[ "$CDRW_NUMBER" = "" ] && ln -s $i /dev/cdrw
		[ "$CDRW_NUMBER" = "" ] && CDRW_NUMBER="-1"
		! [ "$CDRW_NUMBER" = "" ] && CDRW_NUMBER="$((CDRW_NUMBER+1))" && ln -s $i /dev/cdrw$CDRW_NUMBER
		fi
	done

	for i in /dev/cd/dvd-*; do
		if [ -h $i ]; then
		[ "$DVD_NUMBER" = "" ] && ln -s $i /dev/dvd
		[ "$DVD_NUMBER" = "" ] && DVD_NUMBER="-1"
		! [ "$DVD_NUMBER" = "" ] && DVD_NUMBER="$((DVD_NUMBER+1))" && ln -s $i /dev/dvd$DVD_NUMBER
		fi
	done

	for i in /dev/cd/dvdrw-*; do
		if [ -h $i ]; then
		[ "$DVDRW_NUMBER" = "" ] && ln -s $i /dev/dvdrw
		[ "$DVDRW_NUMBER" = "" ] && DVDRW_NUMBER="-1"
		! [ "$DVDRW_NUMBER" = "" ] && DVDRW_NUMBER="$((DVDRW_NUMBER+1))" && ln -s $i /dev/dvdrw$DVDRW_NUMBER
		fi
	done
fi
