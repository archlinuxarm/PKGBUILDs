#!/bin/bash
set -a # export all variables

. /etc/conf.d/sabnzbd

# SABnzbd must be able to read the file, so we copy it to a directory where it
# certainly has rights.
nzbname=$(basename "$1")
TEMP_NZB="/var/tmp/$nzbname"

curl -s $(python2 -c 'import urlparse,urllib,sys; print sys.argv[1] if urlparse.urlparse(sys.argv[1]).scheme else urlparse.urlparse(urllib.pathname2url(sys.argv[1]),"file").geturl()' "$1") -o "$TEMP_NZB"
curl -f $(python2 -c 'import urllib,os; print os.environ["SABNZBD_PROTOCOL"]+"://"+urllib.quote(os.environ["SABNZBD_USPW"]+os.environ["SABNZBD_IP"])+":"+os.environ["SABNZBD_PORT"]+"/sabnzbd/api?mode=addlocalfile&name="+urllib.quote(os.environ["TEMP_NZB"])+"&apikey="+urllib.quote(os.environ["NZB_KEY"])')
rm "$TEMP_NZB"
exit 0
