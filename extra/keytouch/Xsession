#!/bin/sh

for script in /etc/X11/Xsession.d/*; do
  if [ -x $script ]; then
    . $script
  fi
done
unset script
