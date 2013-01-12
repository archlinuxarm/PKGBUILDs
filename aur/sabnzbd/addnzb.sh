#!/bin/sh

. /etc/conf.d/sabnzbd 
curl -s -F apikey="$API_KEY" -F mode="addfile" -F name=@"$1" $URL/sabnzbd/api &> /dev/null
