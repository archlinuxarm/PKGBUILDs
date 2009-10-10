export PATH=$PATH:/opt/kde/bin
if [ ! -z $XDG_DATA_DIRS ]; then
  export XDG_DATA_DIRS=$XDG_DATA_DIRS:/opt/kde/share
else
  export XDG_DATA_DIRS=/opt/kde/share
fi
