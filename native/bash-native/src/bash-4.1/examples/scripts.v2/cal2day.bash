#!/bin/bash
# cal2day - "parse" appropriate calendar output to match date number
#	    with day name.
#
# usage: cal2day month day [year]
#
# ORIGINAL *TAG:33239 3:Dec 9 1997:0755:sh.d/cal2day:
#
# Obtained from usenet
#
# Converted to bash v2 syntax by Chet Ramey <chet@po.cwru.edu>

#1 PARSE OPTIONS
while getopts :dls _inst
do	case $_inst in
	(d)	format='%1d%.0s\n' ;;		# 0, 1, ..., 7
	(l)	format='%0.s%-s\n' ;;		# Sunday, Monday, ..., Saturday
	(s)	format='%0.s%-.3s\n' ;;		# Sun, Mon, ..., Sat
	esac
done
shift $((OPTIND-1))

#2 PARAMETER VALUES
((!$#)) && set -- $(date '+%m %d')
: ${format:='%0.s%-.3s\n'}
: ${1:?missing month parameter [1-12]}
: ${2:?missing day parameter [1-31]}

#3 CALCULATE DAY-OF-WEEK FROM DATE
cal $1 ${3:-$(date +%Y)} | gawk -FX '
BEGIN	{ day="Sunday   Monday   Tuesday  WednesdayThursday Friday   Saturday"
	  sub(/^0/, "", daynum)
	  dayre="(^| )" daynum "( |$)"
	}
#NR==2	{ print length($0) }
NR==1 || NR==2 \
	{ next }
dayre	{ if (match($0, dayre))
	  {	#print RSTART, RLENGTH, substr($0, RSTART, RLENGTH)
		if (daynum<=9 || RSTART==1) RSTART-=1
		exit
	  }
	}
END	{ # 20/21 char width assumed
	  printf format, RSTART/3, substr(day, RSTART*3+1, 9)
	}
' daynum=$2 format=$format -

exit 0
