# aref.bash --- pseudo-array manipulating routines
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created 1992-07-01
# Last modified: 1993-02-03
# Public domain

# Conversion to bash v2 syntax done by Chet Ramey

# Commentary:
# Code:

#:docstring aref:
# Usage: aref NAME INDEX
#
# In array NAME, access element INDEX (0-origin)
#:end docstring:

###;;;autoload
function aref ()
{
    local name="$1"
    local index="$2"

    set -- ${!name}
    [ $index -ge 1 ] && shift $index
    echo $1
}

#:docstring string_aref:
# Usage: aref STRING INDEX
#
# Echo the INDEXth character in STRING (0-origin) on stdout. 
#:end docstring:

###;;;autoload
function string_aref ()
{
  local stuff=${1:$2}
  echo ${stuff:0:1}
}

provide aref

# aref.bash ends here
