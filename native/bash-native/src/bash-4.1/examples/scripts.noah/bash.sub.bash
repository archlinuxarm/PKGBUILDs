# bash.sub.bash --- stub for standalone shell scripts using bash library
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-07-13
# Last modified: 1993-09-29
# Public domain

#:docstring bash.sub:
# Standard subroutines for bash scripts wishing to use "require" to load
# libraries.
#
# Usage: In each directory where a bash script that uses this script
# exists, place a copy of this script.  Then, at the top of such scripts,
# put the command
#
#    source ${0%/*}/bash.sub || exit 1
#
# Then you can use `require' to load packages. 
#
#:end docstring:

default_FPATH="~friedman/etc/init/bash/functions/lib"

source "${default_FPATH}/feature"
REQUIRE_FAILURE_FATAL=t

FPATH="${FPATH-${default_FPATH}}"

# bash.sub.bash ends here
