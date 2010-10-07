#!/bin/bash
#
# scrollbar - display scrolling text
#
# usage: scrollbar args
#
# A cute hack originally from Heiner Steven <hs@bintec.de>
#
# converted from ksh syntax to bash v2 syntax by Chet Ramey

WIDTH=${COLUMNS:-80}
WMINUS=$(( $WIDTH - 1 ))

[ $# -lt 1 ] && set -- TESTING

# use the bash-2.02 printf builtin
Text=$(printf "%-${WIDTH}s" "$*")
Text=${Text// /_}

while :
do
	printf "%-.${WIDTH}s\r" "$Text"
	LastC=${Text:${WMINUS}:1}
	Text="$LastC""${Text%?}"
done
