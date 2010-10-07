#!/bin/bash
#
# vtree - make a tree printout of the specified directory, with disk usage
#	  in 1k blocks
#
# usage: vtree [-a] [dir]
#
# Original posted to Usenet sometime in February, 1996
# I believe that the original author is Brian S. Hiles <bsh29256@atc.fhda.edu>
#
usage()
{
	echo "vtree: usage: vtree [-a] [dir]" >&2
}

while getopts a opt
do
	case "$opt" in
	a)	andfiles=-a ;;
	*)	usage ; exit 2 ;;
	esac
done

shift $((OPTIND - 1))

export BLOCKSIZE=1k	# 4.4 BSD systems need this

[ $# -eq 0 ] && set .

while [ $# -gt 0 ]
do
	cd "$1" || { shift; [ $# -ge 1 ] && echo >&2; continue; }
	echo -n "$PWD"

	du $andfiles | sort -k 2f | sed \
		-e 's/\([^	]*\)	\(.*\)/\2  (\1)/' \
		-e "s#^$1##" \
		-e 's#[^/]*/\([^/]*\)$#|____\1#' \
		-e 's#[^/]*/#|    #g'
	
	[ $# -gt 1 ] && echo 
	shift
done
