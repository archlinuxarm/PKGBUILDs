# shcat.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-07-17
# Last modified: 1993-09-29
# Public domain

# Conversion to bash v2 syntax done by Chet Ramey

# Commentary:
# Code:

#:docstring shcat:
# Usage: shcat {file1} {file2} {...}
#
# Like `cat', only this is all inline bash. 
#:end docstring:

###;;;autoload
function shcat ()
{
 local IFS=""
 local line
 local file
 local exitstat=0
 
    if [ $# -eq 0 ]; then
       while read -r line; do
          echo "${line}"
       done
       return 0
    else
       for file in "$@" ; do
          if [ -r "${file}" ]; then
               while read -r line; do
                  echo "${line}"
               done < "${file}"
          else
             # This will cause the error to be printed on stderr
             < "${file}"
             exitstat=1
          fi
       done
       return ${exitstat}
    fi
}

provide shcat

# shcat.bash ends here
