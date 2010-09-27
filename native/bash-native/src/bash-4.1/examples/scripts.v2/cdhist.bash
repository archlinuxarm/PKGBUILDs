#! /bin/bash
#
# cdhist - cd replacement with a directory stack like pushd/popd
#
# usage: cd [-l] [-n] [-] [dir]
#
# options:
#	-l	print the cd directory stack, one entry per line
#	-	equivalent to $OLDPWD
#	-n	cd to nth directory in cd directory stack
#	-s	cd to first directory in stack matching (substring) `s'
#
# arguments:
#	dir	cd to dir and push dir onto the cd directory stack
#
# If the new directory is a directory in the stack and the options selected
# it (-n, -s), the new working directory is printed
#
# If the variable CDHISTFILE is set, the cd directory stack is loaded from
# and written to $CDHISTFILE every time `cd' is executed.
#
# Note: I got this off the net somewhere; I don't know the original author
#
# Chet Ramey
# chet@po.cwru.edu	

_cd_print()
{
	echo -e "$@"
}

cd()
{
	typeset -i cdlen i
	typeset t
	
	if [ $# -eq 0 ]
	then
		set -- $HOME
	fi
	
	if [ "$CDHISTFILE" ] && [ -r "$CDHISTFILE" ] # if directory history exists
	then
		typeset CDHIST
		i=-1
		while read -r t			# read directory history file
		do
			CDHIST[i=i+1]=$t
		done <$CDHISTFILE
	fi
	
	if [ "${CDHIST[0]}" != "$PWD" ] && [ -n "$PWD" ]
	then
		_cdins				# insert $PWD into cd history
	fi
	
	cdlen=${#CDHIST[*]}			# number of elements in history
	
	case "$@" in
	-)					# cd to new dir
		if [ "$OLDPWD" = "" ] && ((cdlen>1))
		then
			'_cdprint' ${CDHIST[1]}
			builtin cd ${CDHIST[1]}
			pwd
		else
			builtin cd "$@"
			# pwd
		fi
		;;
	-l)					# _cdprint directory list
		((i=cdlen))
		while (((i=i-1)>=0))
		do
			num=$i
			'_cdprint' "$num ${CDHIST[i]}"
		done
		return
		;;
	-[0-9]|-[0-9][0-9])			# cd to dir in list
		if (((i=${1#-})<cdlen))
		then
			'_cdprint' ${CDHIST[i]}
			builtin cd ${CDHIST[i]}
			pwd
		else
			builtin cd $@
			# pwd
		fi
		;;
	-*)					# cd to matched dir in list
		t=${1#-}
		i=1
		while ((i<cdlen))
		do
			case ${CDHIST[i]} in
			*$t*)
				'_cdprint' ${CDHIST[i]}
				builtin cd ${CDHIST[i]}
				pwd
				break
				;;
			esac
			((i=i+1))
		done
		if ((i>=cdlen))
		then
			builtin cd $@
			# pwd
		fi
		;;
	*)					# cd to new dir
		builtin cd $@
		# pwd
		;;
	esac

	_cdins					# insert $PWD into cd history
	
	if [ "$CDHISTFILE" ]
	then
		cdlen=${#CDHIST[*]}		# number of elements in history

		i=0
		while ((i<cdlen))
		do
			echo ${CDHIST[i]}	# update directory history
			((i=i+1))
		done >$CDHISTFILE
	fi
}
	
_cdins()					# insert $PWD into cd history
{						# meant to be called only by cd
	typeset -i i

	i=0

	while (( i < ${#CDHIST[*]} ))		# see if dir is already in list
	do
		if [ "${CDHIST[$i]}" = "$PWD" ]
		then
			break
		fi
		((i=i+1))
	done

	if (( i>22 ))				# limit max size of list
	then
		i=22
	fi

	while (((i=i-1)>=0))			# bump old dirs in list
	do
		CDHIST[i+1]=${CDHIST[i]}
	done

	CDHIST[0]=$PWD				# insert new directory in list
}
	
# examples
shopt -s expand_aliases

# go to known place before doing anything
cd /

echo CDHIST: "${CDHIST[@]}"
for dir in /tmp /bin - -2 -l
do
	cd $dir
	echo CDHIST: "${CDHIST[@]}"
	echo PWD: $PWD

done

exit 0
