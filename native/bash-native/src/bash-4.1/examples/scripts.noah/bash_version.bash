# bash_version.bash --- get major and minor components of bash version number
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1993-01-26
# Last modified: 1993-01-26
# Public domain

# Converted to bash v2 syntax by Chet Ramey

# Commentary:
# Code:

#:docstring bash_version:
# Usage: bash_version {major|minor}
#
# Echo the major or minor number of this version of bash on stdout, or
# just echo $BASH_VERSION if no argument is given. 
#:end docstring:

###;;;autoload
function bash_version ()
{
    local major minor

    case "$1" in 
    major) echo "${BASH_VERSION/.*/}" ;;
    minor) major="${BASH_VERSION/.*/}"
	   minor="${BASH_VERSION#${major}.}"
           echo "${minor%%.*}" ;;
    patchlevel) minor="${BASH_VERSION#*.*.}"
		echo "${minor%(*}" ;;
    version) minor=${BASH_VERSION/#*.*./}
	     echo ${BASH_VERSION/%.$minor/} ;;
    release) echo ${BASH_VERSION%(*} ;;
    build) minor="${BASH_VERSION#*.*.*(}"
	   echo ${minor%)} ;;
    *) echo "${BASH_VERSION}" ;;
    esac
}

provide bash_version

# bash_version.bash ends here
