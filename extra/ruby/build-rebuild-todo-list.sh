#!/usr/bin/env bash

if [ -z "${1}" ]; then
  echo "Usage: $0 <old_ruby_version>"
  exit 1
fi

ruby_version_short="${1:0:3}"
ruby_version_long="${ruby_version_short}.0"
todo_list_filename="./ruby_rebuild_todo.txt"

sudo pacman -Fy

echo
echo "Generating ${todo_list_filename} ..."

{
  pacman -Fx ".*/ruby/(${ruby_version_long}|gems/${ruby_version_long}|vendor_ruby/${ruby_version_short}|vendor_ruby/${ruby_version_long})/*" |sed -ne 's,^.*\(core\|extra\|multilib\)/\(.*\) .*$,\2,p'

  sogrep extra libruby.so
  sogrep extra "libruby.so.${ruby_version_short}"
} | sort -u > "${todo_list_filename}"
