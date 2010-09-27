#Newsgroups: comp.unix.shell
#From: gwc@root.co.uk (Geoff Clare)
#Subject: Re: Determining permissions on a file
#Message-ID: <Dr79nw.DtL@root.co.uk>
#Date: Fri, 10 May 1996 17:23:56 GMT

#Here's a bit of Korn shell that converts the symbolic permissions produced
#by "ls -l" into octal, using only shell builtins.  How to create a script
#combining this with an "ls -l" is left as an exercise...
#
#
# Converted to Bash v2 syntax by Chet Ramey <chet@po.cwru.edu>
#
# usage: showperm modestring
#
# example: showperm '-rwsr-x--x'
#

[ -z "$1" ] && {
	echo "showperm: usage: showperm modestring" >&2
	exit 2
}

tmode="$1"

typeset -i omode sbits
typeset pmode

# check for set-uid, etc. bits
sbits=0
case $tmode in
???[sS]*)       (( sbits += 8#4000 )) ;; # set-uid
??????[sSl]*)   (( sbits += 8#2000 )) ;; # set-gid or mand. lock
?????????[tT]*) (( sbits += 8#1000 )) ;; # sticky
esac

omode=0
while :
do
	tmode=${tmode#?}
	case $tmode in
	"")       break ;;
	[-STl]*)  (( omode *= 2 )) ;;
	[rwxst]*) (( omode = omode*2 + 1 )) ;;
	*)	  echo "$0: first letter of \"$tmode\" is unrecognized" >&2
	          (( omode *= 2 ))
	          ;;
	esac
done

(( omode += sbits ))

printf "0%o\n" $omode
