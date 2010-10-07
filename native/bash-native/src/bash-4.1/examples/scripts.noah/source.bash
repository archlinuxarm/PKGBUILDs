# source.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-05-17
# Last modified: 1993-09-29
# Public domain

# Commentary:
# Code:

#:docstring source:
# Usage: source file ...
#
# Source forces file arguments to be considered in the current directory
# only, unless there is an absolute path starting with `/'.  I think it's
# bad that the builtin "source" searches PATH, because PATH normally
# contains directories with binary files that aren't useful for bash to
# read and most people don't put "." first in their path.
#
# This "source" is capable of reading more than one file at a time.  Return
# value is number of failed source attempts.
#:end docstring:

# This function is not hygienic, but there's not much we can do about
# variable name conflicts here.

###;;;autoload
function source ()
{
    local -i _source_failure_count=0
    local _source_file
 
    for _source_file ; do
       # Check first part of each filename.  If it's not `/', `./', or
       # `../' then prepend "./" to the path to force the builtin `source'
       # not to go searching through PATH to find the file.
       case "${_source_file}" in
          /*|./*|../* ) ;;
          * ) _source_file="./${_source_file}" ;;
       esac  

       builtin source "${_source_file}" || 
         _source_failure_count="_source_failure_count + 1"

    done

    return ${_source_failure_count}
}

#:docstring .:
# See "source"
#:end docstring:

# So that `.' will call function definition of `source' instead of builtin

###;;;autoload
function . ()
{
    source "$@"
}

provide source

# source.bash ends here
