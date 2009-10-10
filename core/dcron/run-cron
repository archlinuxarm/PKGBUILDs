#!/bin/sh


if [ -z $1 ]; then
   echo "Usage: $0 crondir"
   exit 1
fi

for cron in $1/* ; do
  if [ -x $cron ]; then
     $cron
  fi
done
unset cron
