# vi:set sts=2 sw=2 ai:
#
# coshell.bash - Control shell coprocesses (see coprocess.bash).
#

function coshell ()
{
  while (( $# > 0 )) ; do
    case "$1" in
      #
      # coshell open
      #
      o|op|ope|open)
	shift
	coprocess open "$@"
	local ret=$?

	# This should eat any ssh error messages or what not.
	coshell eval : >/dev/null 2>&1
	return $ret
	;;

      #
      # coshell close
      #
      c|cl|clo|close)
	shift
	coprocess close "$@"
	return $?
	;;

      #
      # coshell eval
      #
      e|ev|eva|eval)
	shift
	local cookie=$RANDOM
	if (( $# == 0 )) ; then
	  echo "coshell eval: no argumentsl" >&2
	  return 1
	fi
	if [ x$coprocess_pid = x ] ; then
	  echo "coshell eval: no active coshell" >&2
	  return 1
	fi

	coprocess print "$@" 
	coprocess print "coprocess_rc=\$?"
	coprocess print "printf 'coprocess-$cookie----\n%d\n' \$coprocess_rc"
	if [ x$coprocess_pid = x ] ; then
	  return 0
	fi

	local ol
	while coprocess read ol ; do
	  case "$ol" in
	    *coprocess-$cookie----*)
	      ol="${ol%coprocess-$cookie----}"
	      echo -n "$ol"
	      break
	      ;;
	  esac
	  echo "$ol"
	done
	coprocess read ol
	return $ol
	;;

      #
      # coshell sendfile
      #
      s|se|sen|send|sendf|sendfi|sendfil|sendfile)
	shift
	if (( $# != 2 )) ; then
	  echo "coshell sendfile: syntax is 'coshell sendfile SRC TARGET'" >&2
	  return 1
	fi
	if [ x$coprocess_pid = x ] ; then
	  echo "coshell sendfile: no active coshell" >&2
	  return 1
	fi

	local target=$2
	if coshell test -d "$target" ; then
	  target="$target/${1##*/}" 
	fi

	coprocess print "uudecode <<END_OF_FILE"
	uuencode -m "$target" <$1 |coprocess print --stdin
	coshell eval "END_OF_FILE"
	return $?
	;;

      #
      # coshell getfile
      #
      g|ge|get|getf|getfi|getfil|getfile)
	shift
	if (( $# != 2 )) ; then
	  echo "coshell getfile: syntax is 'coshell getfile SRC TARGET'" >&2
	  return 1
	fi
	if [ x$coprocess_pid = x ] ; then
	  echo "coshell getfile: no active coshell" >&2
	  return 1
	fi

	local target=$2
	if test -d "$target" ; then
	  target="$target/${1##*/}" 
	fi

	coshell eval uuencode -m "$target" "<" "$1" |uudecode
	return $?
	;;

      *)
	coshell eval "$@"
	return $?
	;;
    esac
    shift
  done
  coprocess status
  return $?
}

