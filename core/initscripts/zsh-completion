#compdef rc.d

_rc.d () {
	local curcontext="$curcontext" state line
	typeset -A opt_args

	 _arguments "1: :->action" "*: :->service"

	case $state in
		action)
			_arguments "1:action:(list help start stop restart)"
			;;
		service)
			local action="$words[2]"
			curcontext="${curcontext%:*:*}:rc.d-${action}:"

			case $action in
				list|help)
					_arguments "*: :"
					;;
				start)
					_arguments "*: :($(comm -23 <(echo /etc/rc.d/*(N-*:t)|tr ' ' '\n') <(echo /run/daemons/*(N:t)|tr ' ' '\n')))"
					;;
				stop|restart|reload)
					_arguments "*: :(/run/daemons/*(N:t))"
					;;
				*)
					_arguments "*: :(/etc/rc.d/*(N-*:t))"
					;;
			esac
		;;
  esac
}

_rc.d "$@"

# vim: set ts=2 sw=2 ft=sh noet:
