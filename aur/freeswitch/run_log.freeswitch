#!/bin/sh -e
if [ $0 != "./run" ];then
  echo "This script meant to be linked as ./run in a service/log directory only!"
  exit 1
fi
logdir=$(basename $(pwd))
if [ "$logdir" != "log" ];then
  echo "This script meant to be run from a service/log directory only!"
  exit 1
fi
if [ -w /var/log ];then
  if [ -f ./conf ];then
    source ./conf
  fi
  user_group=${USERGROUP:-daemon:adm}
  service=$(basename $(dirname $(pwd)))
  [ -d "/var/log/$service" ] || mkdir -p "/var/log/$service"
  [ -L ./main ] || [ -d ./main ] || ln -s "/var/log/$service" ./main
  [ -L ./current ] || ln -s main/current
  usergroup=$(stat -c "%U:%G" "/var/log/$service")
  if [ "$usergroup" != "$user_group" ];then
    chown -R $user_group "/var/log/$service"
  fi
  echo Logging as $user_group to /var/log/$service
  exec chpst -u $user_group svlogd -t ./main
else
  echo Logging in $PWD
  exec svlogd -t ./
fi
