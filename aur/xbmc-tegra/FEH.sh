#!/bin/bash

RETVAL=0

if [[ -z $(glxinfo | grep "direct rendering.*Yes" | uniq) ]]; then
  echo "XBMC needs hardware accelerated OpenGL rendering."
  echo "Install an appropriate graphics driver."
  echo 
  echo "Please consult XBMC Wiki for supported hardware"
  echo "http://xbmc.org/wiki/?title=Supported_hardware"
  echo
  RETVAL=1
fi

if [[ -z $(xdpyinfo | grep "depth of root.*24" | uniq) ]]; then
  echo "XBMC cannot run unless the"
  echo "screen color depth is at least 24 bit."
  echo
  echo "Please reconfigure your screen."
  RETVAL=1
fi

exit ${RETVAL}
