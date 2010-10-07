# mktmp.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1993-02-03
# Last modified: 1993-02-03
# Public domain

# Conversion to bash v2 syntax done by Chet Ramey

# Commentary:
# Code:

#:docstring mktmp:
# Usage: mktmp [template] {createp}
#
# Generate a unique filename from TEMPLATE by appending a random number to
# the end. 
#
# If optional 2nd arg CREATEP is non-null, file will be created atomically
# before returning.  This is to avoid the race condition that in between
# the time that the temporary name is returned and the caller uses it,
# someone else creates the file. 
#:end docstring:

###;;;autoload
function mktmp ()
{
    local template="$1"
    local tmpfile="${template}${RANDOM}"
    local createp="$2"
    local noclobber_status

    case "$-" in
    *C*)	noclobber_status=set;;
    esac

    if [ "${createp:+set}" = "set" ]; then
       # Version which creates file atomically through noclobber test.   
       set -o noclobber
       (> "${tmpfile}") 2> /dev/null
       while [ $? -ne 0 ] ; do
          # Detect whether file really exists or creation lost because of
          # some other permissions problem.  If the latter, we don't want
          # to loop forever.
          if [ ! -e "${tmpfile}" ]; then
             # Trying to create file again creates stderr message.
             echo -n "mktmp: " 1>&2
             > "${tmpfile}"
             return 1
          fi
          tmpfile="${template}${RANDOM}"
          (> "${tmpfile}") 2> /dev/null
       done
       test "${noclobber_status}" != "set" && set +o noclobber
    else
       # Doesn't create file, so it introduces race condition for caller. 
       while [ -e "${tmpfile}" ]; do
          tmpfile="${template}${RANDOM}"
       done
    fi

    echo "${tmpfile}"
}

provide mktmp

# mktmp.bash ends here
