# coprocess.bash
#
# vi:set sts=2 sw=2 ai:
#

coprocess_pid=

#
# coprocess - Start, control, and end coprocesses.
#
function coprocess ()
{
  while (( $# > 0 )) ; do
    case "$1" in
      #
      # coprocess close
      #
      c|cl|clo|clos|close)
	shift
	exec 61>&- 62<&-
	coprocess_pid=
	if [ "$1" = "-SIGPIPE" ] ; then
	  # Only print message in an interactive shell
	  case "$-" in
	    *i*)
	      echo 'SIGPIPE' >&2
	      ;;
	  esac
	  return 1
	fi
	return 0
	;;

      #
      # coprocess open
      #
      o|op|ope|open)
	shift
	local fifo="/var/tmp/coprocess.$$.$RANDOM"

	local cmd="/bin/bash"
	if (( $# > 0 )) ; then
	  cmd="$@"
	fi

	mkfifo "$fifo.in" || return $?
	mkfifo "$fifo.out" || {
	  ret=$?
	  rm -f "$fifo.in"
	  return $?
	}

	( "$@" <$fifo.in >$fifo.out ; rm -f "$fifo.in" "$fifo.out" ) &
	coprocess_pid=$!
	exec 61>$fifo.in 62<$fifo.out
	return 0
	;;

      #
      # coprocess print - write to the coprocess
      #
      p|pr|pri|prin|print)
	shift
	local old_trap=$(trap -p SIGPIPE)
	trap 'coprocess close -SIGPIPE' SIGPIPE
	if [ $# -eq 1 ] && [ "$1" = "--stdin" ] ; then
	  cat >&61
	else
	  echo "$@" >&61
	fi
	local ret=$?
	eval "$old_trap"
	return $ret
	;;

      #
      # coprocess read - read from the coprocess
      #
      r|re|rea|read)
	shift
	local old_trap=$(trap -p SIGPIPE)
	trap '_coprocess_close -SIGPIPE' SIGPIPE
	builtin read "$@" <&62
	local ret=$?
	eval "$old_trap"
	return $ret
	;;

      s|st|sta|stat|statu|status)
	if [ -z "$coprocess_pid" ] ; then
	  echo 'no active coprocess'
	  return 1
	else
	  echo "  coprocess is active [$coprocess_pid]"
	  return 0
	fi
	;;

      *)
	coprocess print "$@"
	return $?
	;;
    esac
    shift
  done
  coprocess status
  return $?
}
