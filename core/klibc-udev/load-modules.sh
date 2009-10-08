#! /bin/sh
# Implement blacklisting for udev-loaded modules
#   Includes module checking
# - Aaron Griffin & Tobias Powalowski for Archlinux
[ $# -ne 1 ] && exit 1

MODPROBE="/sbin/modprobe"
RESOLVEALIAS="/bin/resolve-modalias"
USEBLACKLIST="--use-blacklist"
REPLACE="/bin/replace"
MODDEPS="/bin/moddeps"

if [ -f /proc/cmdline ]; then 
  for cmd in $(cat /proc/cmdline); do
    case $cmd in
      disablemodules=*) eval $cmd ;;
      load_modules=off) exit ;;
    esac
  done
  #parse cmdline entries of the form "disablemodules=x,y,z"
  if [ -n "${disablemodules}" ]; then
    BLACKLIST="$(${REPLACE} ${disablemodules} ',')"
  fi
fi

# sanitize the module names
BLACKLIST="$(${REPLACE} "${BLACKLIST}" '-' '_')"

if [ -n "${BLACKLIST}" ] ; then
  # Try to find all modules for the alias
  mods="$($RESOLVEALIAS /lib/modules/$(uname -r)/modules.alias $1)"
  # If no modules could be found, try if the alias name is a module name
  # In that case, omit the --use-blacklist parameter to imitate normal modprobe behaviour
  [ -z "${mods}" ] && $MODPROBE -qni $1 && mods="$1" && USEBLACKLIST=""
  [ -z "${mods}" ] && exit
  for mod in ${mods}; do
    deps="$(${MODDEPS} ${mod})"
    [ $? -ne 0 ] && continue
    # If the module or any of its dependencies is blacklisted, don't load it
    for dep in $deps; do
      for blackmod in ${BLACKLIST}; do
        [ "${blackmod}" = "${dep}" ] && continue 3
      done
    done
    $MODPROBE $USEBLACKLIST ${mod}
  done
else
  $MODPROBE $1
fi

# vim: set et ts=4:
