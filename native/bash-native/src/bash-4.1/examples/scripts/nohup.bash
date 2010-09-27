#
# BASH VERSION OF nohup COMMAND
#
ctype()
{
	path=$(builtin type -p $cmd | sed 1q)
	if [ -n "$path" ]; then
		echo "$path"
		return 0
	else
		case "$cmd" in
		*/*)	[ -x "$cmd ] && { echo "$cmd" ; return 0; } ;;
		  *)	case "$(builtin type -t $cmd)" in
			"")	return 1;;
			*)	echo "$cmd" ; return 0;;
			esac ;;
		esac
	fi
	return 1
}

trap '' HUP		# ignore hangup
command=$(ctype "$1")
oldmask=$(umask)
umask u=rw,og=		# default mode for nohup.out
exec 0< /dev/null	# disconnect input
if [ -t 1 ]; then	# redirect output if necessary
	if [ -w . ]; then
		echo 'Sending output to nohup.out'
		exec >> nohup.out
	else	echo "Sending output to $HOME/nohup.out"
		exec >> $HOME/nohup.out
	fi
fi

umask "$oldmask"

# direct unit 2 to a file
if [ -t 2 ]; then
	exec 2>&1
fi

# run the command
case $command in
*/*)	exec "$@"
	;;
time)	eval "$@"
	;;
*)	"$@"
	;;
esac
