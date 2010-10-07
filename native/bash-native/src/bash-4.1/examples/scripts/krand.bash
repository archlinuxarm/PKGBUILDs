# Originally
#
# From: bsh20858@news.fhda.edu (Brian S Hiles)
# Newsgroups: comp.unix.shell
# Subject: Re: getting random numbers
# Date: 23 Jan 1997 23:27:30 GMT
# Message-ID: <5c8s52$eif@tiptoe.fhda.edu>

# @(#) krand  Produces a random number within integer limits
# "krand" Korn shell script generates a random number in a
# specified range with an optionally specified ``seed'' value.
# Author: Peter Turnbull, May 1993
# Modified by: Becca Thomas, January 1994

# changed the optional third argument to a -s option, converted to
# bash v2 syntax -- chet@po.cwru.edu

PROGNAME=${0##*/}
USAGE="usage: $PROGNAME [-s seed] lower-limit upper-limit"

Seed=$$			# Initialize random-number seed value with PID

usage()
{
	echo ${PROGNAME}: "$USAGE" >&2
}

errexit()
{
	echo ${PROGNAME}: "$@" >&2
	exit 1
}

# Process command-line arguments:
while getopts "s:" opt; do
	case "$opt" in
	s) Seed=$OPTARG ;;
	*) usage ; exit 2;;
	esac
done

shift $(($OPTIND - 1))

case $# in
	2) Lower=$1; Upper=$2 ;;
	*) usage ; exit 2;;
esac

# Check that specified values are integers:
expr "$Lower" + 0 >/dev/null 2>&1
[ $? -eq 2 ] && { errexit "lower ($Lower) not an integer"; }
expr "$Upper" + 0 >/dev/null 2>&1
[ $? -eq 2 ] && { errexit "upper ($Upper) not an integer"; }
expr "$Seed" + 0 >/dev/null 2>&1
[ $? -eq 2 ] && { errexit "seed ($Seed) not an integer"; }

# Check that values are in the correct range:
if (( "$Lower" < 0 )) || [ ${#Lower} -gt 5 ]; then
	errexit "lower limit ($Lower) less than zero"
fi
if (( "$Upper" > 32767 )) || [ ${#Upper} -gt 5 ]; then
	errexit "upper limit ($Upper) greater than 32767"
fi
if (( "$Seed" < 0 )) || (( "$Seed" > 32767 )) || [ ${#Seed} -gt 5 ]; then
	errexit "seed value ($Seed) out of range (0 to 32767)"
fi
(( "$Upper" <= "$Lower" )) && errexit "upper limit ($Upper) <= lower limit ($Lower)"

# Seed the random-number generator:
RANDOM=$Seed
# Compute value, scaled within range:
let rand="$RANDOM % ($Upper - $Lower + 1) + $Lower"
# Report result:
echo $rand
