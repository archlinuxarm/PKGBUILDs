#!/bin/bash
# Implement blacklisting for udev-loaded modules

[ $# -ne 1 ] && exit 1

. /etc/rc.conf

# grab modules from rc.conf
BLACKLIST="${MOD_BLACKLIST[@]}"
MODPROBE="/sbin/modprobe"
LOGGER="/usr/bin/logger"
RESOLVEALIAS="${MODPROBE} --resolve-alias"
USEBLACKLIST="--use-blacklist"

if [ -f /proc/cmdline ]; then 
    for cmd in $(cat /proc/cmdline); do
        case $cmd in
            disablemodules=*) eval $cmd ;;
            load_modules=off) exit ;;
        esac
    done
    #parse cmdline entries of the form "disablemodules=x,y,z"
    if [ -n "$disablemodules" ]; then
        BLACKLIST="$BLACKLIST $(echo $disablemodules | sed 's|,| |g')"
    fi
fi

#MODULES entries in rc.conf that begin with ! are blacklisted
for mod in ${MODULES[@]}; do
    if [ "${mod}" != "${mod#!}" ]; then
        BLACKLIST="$BLACKLIST ${mod#!}"
    fi
done

if [ "$MOD_AUTOLOAD" = "yes" -o "$MOD_AUTOLOAD" = "YES" ]; then
  if [ -n "${BLACKLIST}" ]; then
    # If an alias name is on the blacklist, load no modules for this device
    if echo "${BLACKLIST}" | /bin/grep -q -e " $1 " -e "^$1 " -e " $1\$"; then
      $LOGGER -p info -t "$(basename $0)" "Not loading module alias '$1' because it is blacklisted"
      exit
    fi
    #sanitize the blacklist
    BLACKLIST="$(echo "$BLACKLIST" | sed -e 's|-|_|g')"
    # Try to find all modules for the alias
    mods=$($RESOLVEALIAS $1)
    # If no modules could be found, try if the alias name is a module name
    # In that case, omit the --use-blacklist parameter to imitate normal modprobe behaviour
    [ -z "${mods}" ] && $MODPROBE -qni $1 && mods="$1" && USEBLACKLIST=""
    [ -z "${mods}" ] && $LOGGER -p local0.debug -t "$(basename $0)" "'$1' is not a valid module or alias name"
    for mod in ${mods}; do
      # Find the module and all its dependencies
      deps="$($MODPROBE -i --show-depends ${mod})"
      [ $? -ne 0 ] && continue

      #sanitize the module names
      deps="$(echo "$deps" | sed \
              -e "s#^insmod /lib.*/\(.*\)\.ko.*#\1#g" \
              -e 's|-|_|g')"

      # If the module or any of its dependencies is blacklisted, don't load it
      for dep in $deps; do
        if echo "${BLACKLIST}" | /bin/grep -q -e " ${dep} " -e "^${dep} " -e " ${dep}\$"; then
          if [ "${dep}" = "${mod}" ]; then
            $LOGGER -p local0.info -t "$(basename $0)" "Not loading module '${mod}' for alias '$1' because it is blacklisted"
          else
            $LOGGER -p local0.info -t "$(basename $0)" "Not loading module '${mod}' for alias '$1' because its dependency '${dep}' is blacklisted"
          fi
          continue 2
        fi
      done
      # modprobe usually uses the "blacklist" statements from modprobe.conf only to blacklist all aliases
      # of a module, but not the module itself. We use --use-blacklist here so that modprobe also blacklists
      # module names if we resolved alias names manually above
      $MODPROBE $USEBLACKLIST ${mod}
    done
  else
    $MODPROBE $USEBLACKLIST $1
  fi
fi
# vim: set et ts=4:
