trap _notify CHLD
NOTIFY_ALL=false
unset NOTIFY_LIST
unalias false

false()
{
	return 1
}

_notify ()
{
	local i j
	local newlist=

	if $NOTIFY_ALL
	then
		return		# let bash take care of this itself
	elif [ -z "$NOTIFY_LIST" ]; then
		return
	else
		set -- $NOTIFY_LIST
		for i in "$@"
		do
			j=$(jobs -n %$i)
			if [ -n "$j" ]; then
				echo "$j"
				jobs -n %$i >/dev/null
			else
				newlist="newlist $i"
			fi
		done
		NOTIFY_LIST="$newlist"
	fi
}

notify ()
{
	local i j

	if [ $# -eq 0 ]; then
		NOTIFY_ALL=:
		set -b
		return
	else
		for i in "$@"
		do
			# turn a valid job spec into a job number
			j=$(jobs $i)
			case "$j" in
			[*)	j=${j%%]*}
				j=${j#[}
				NOTIFY_LIST="$NOTIFY_LIST $j"
				;;
			esac
		done
	fi
}
