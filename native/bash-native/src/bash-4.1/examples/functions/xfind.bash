#! /bin/bash
#From: kaz@cafe.net (Kaz Kylheku)
#Newsgroups: comp.unix.shell
#Subject: Why not roll your own @#$% find! (was: splitting directory off from filename)
#Message-ID: <6n1117$tp1@espresso.cafe.net>
#Date: Fri, 26 Jun 1998 20:47:34 GMT

# $1 = dirname, $2 = pattern, optional $3 = action
xfind()
{
	local x
	local dir="$1"

	# descend into specified directory

	builtin cd -L "$1" || {
		echo "${FUNCNAME}: cannot change dir to $1" >&2
		return 1
	}

	#
	# default action is to print the filename
	#
	if [ -n "$3" ]; then
		action="$3"
	else
		action='printf -- "%s\n"'
	fi

	# process ordinary files that match pattern

	for x in $2 ; do 
		if [ -f "$x" ] ; then
			eval "$action" "$x"
		fi
	done

	# now descend into subdirectories, avoiding symbolic links
	# and directories that start with a period.

	for x in * ; do
		if [ -d "$x" ] && [ ! -L "$x" ] ; then 
			$FUNCNAME "$x" "$2" "$action"
		fi
	done

	# finally, pop back up

	builtin cd -L ..
}

#xfind "$@"
