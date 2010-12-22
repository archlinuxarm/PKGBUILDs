#!/bin/bash

. /etc/conf.d/sabnzbd

# SABnzbd must be able to read the file, so we copy it to a directory where it
# certainly has rights.
nzbname=`basename "$1"`
TEMP_NZB="/var/tmp/$nzbname"

cp "$1" $TEMP_NZB
curl -f "$SABNZBD_PROTOCOL://$SABNZBD_USPW$SABNZBD_IP:$SABNZBD_PORT/sabnzbd/api?mode=addlocalfile&name=$TEMP_NZB&pp=1&priority=-1&apikey=$SABNZBD_KEY"
rm $TEMP_NZB
exit 0
