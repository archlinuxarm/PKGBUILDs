# y_or_n_p.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-06-18
# Last modified: 1993-03-01
# Public domain

# Conversion to bash v2 syntax done by Chet Ramey

# Commentary:
# Code:

#:docstring y_or_n_p:
# Usage: y_or_n_p QUERY
#
# Print QUERY on stderr, then read stdin for a y-or-n response.  Actually,
# user may type anything they like, but first character must be a `y', `n',
# `q', or `!', otherwise the question is repeated until such an answer is
# obtained.  
#
# If user typed `y', y_or_n_p returns 0.
#
# If user typed `n', y_or_n_p returns 1.
#
# If user typed `!', y_or_n_p returns 2.  This is an indication to the
#  caller that no more queries should be made.  Assume `y' for all the rest. 
#
# If user typed `q', y_or_n_p returns 3.  This is an indication to the
#  caller that no more queries should be made.  Assume `n' for all the rest.
#
#:end docstring:

###;;;autoload
function y_or_n_p ()
{
 local ans

    [ ! -t 0 ] && return 1

    while read -p "$*" -e ans ; do
	case "${ans}" in
        y* | Y* ) return 0 ;;
        n* | N* ) return 1 ;;
        \! )      return 2 ;;
        q* | Q* ) return 3 ;;
        *) echo "Please answer one of \`y', \`n', \`q', or \`"\!"'" 1>&2 ;;
	esac
   done
}

#:docstring yes_or_no_p:
# Usage: yes_or_no_p QUERY
#
# Like y_or_n_p, but require a full `yes', `no', `yes!', or `quit' response. 
#:end docstring:

###;;;autoload
function yes_or_no_p ()
{
    local ans

    [ ! -t 0 ] && return 3

    while read -p "$*" -e ans; do
        ans="$(echo ${ans} | tr '[A-Z]' '[a-z]')"

        case "${ans}" in
        yes )   return 0 ;;
        no )    return 1 ;;
        yes\! ) return 2 ;;
        quit )  return 3 ;;
        *) echo "Please answer \`yes', \`no', \`yes"\!"', or \`quit'" 1>&2 ;;
       esac
   done
}

provide y_or_n_p

# y_or_n_p.bash ends here
