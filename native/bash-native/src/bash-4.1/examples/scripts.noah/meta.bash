# meta.bash --- meta key frobnications
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-06-28
# Last modified: 1993-01-26
# Public domain

# Commentary:
# Code:

#:docstring meta:
# Usage: meta [on|off]
# 
# An argument of "on" will make bash use the 8th bit of any input from
# a terminal as a "meta" bit, i.e bash will be able to use a real meta
# key.
#
# An argument of "off" causes bash to disregard the 8th bit, which is
# assumed to be used for parity instead.
#:end docstring:

function meta ()
{
      case "$1" in
         on) bind 'set input-meta On'
	     bind 'set output-meta on'
	     bind 'set convert-meta off' ;;
         off) bind 'set input-meta Off'
	      bind 'set output-meta off'
	      bind 'set convert-meta on' ;;
         *) echo "Usage: meta [on|off]" 1>&2 ; return 1 ;;
      esac
      return 0
}

provide meta

# meta.bash ends here
