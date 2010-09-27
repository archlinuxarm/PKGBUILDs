# require.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-07-08
# Last modified: 1993-09-29
# Public domain

# Commentary:

# These functions provide an interface based on the lisp implementation for
# loading libraries when they are needed and eliminating redundant loading.
# The basic idea is that each "package" (or set of routines, even if it is
# only one function) registers itself with a symbol that marks a "feature"
# as being "provided".  If later you "require" a given feature, you save
# yourself the trouble of explicitly loading it again.
# 
# At the bottom of each package, put a "provide foobar", so when another
# package has a "require foobar", it gets loaded and registered as a
# "feature" that won't need to get loaded again.  (See warning below for
# reasons why provide should be put at the end.)
#
# The list of provided features are kept in the `FEATURES' variable, which
# is not exported.  Care should be taken not to munge this in the shell.
# The search path comes from a colon-separated `FPATH' variable.  It has no
# default value and must be set by the user.
#
# Require uses `fpath_search', which works by scanning all of FPATH for a
# file named the same as the required symbol but with a `.bash' appended to
# the name.  If that is found, it is loaded.  If it is not, FPATH is
# searched again for a file name the same as the feature (i.e. without any
# extension).  Fpath_search may be useful for doing library filename
# lookups in other functions (such as a `load' or `autoload' function).
#
# Warning: Because require ultimately uses the builtin `source' command to
# read in files, it has no way of undoing the commands contained in the
# file if there is an error or if no provide statement appeared (this
# differs from the lisp implementation of require, which normally undoes
# most of the forms that were loaded if the require fails).  Therefore, to
# minize the number of problems caused by requiring a faulty package (such
# as syntax errors in the source file) it is better to put the provide at
# the end of the file, rather than at the beginning.

# Code:

# Exporting this variable would cause considerable lossage, since none of
# the functions are exported (or at least, they're not guaranteed to be)
export -n FEATURES

#:docstring :
# Null function.  Provided only so that one can put page breaks in source
# files without any ill effects.
#:end docstring:
#
# (\\014 == C-l)
eval "function $(echo -e \\014) () { : }"


#:docstring featurep:
# Usage: featurep argument
#
# Returns 0 (true) if argument is a provided feature.  Returns 1 (false)
# otherwise. 
#:end docstring:

###;;;autoload
function featurep ()
{
    local feature="$1"

    case " ${FEATURES} " in
       *" ${feature} "* ) return 0 ;;
    esac

    return 1
}


#:docstring provide:
# Usage: provide symbol ...
#
# Register a list of symbols as provided features
#:end docstring:

###;;;autoload
function provide ()
{
    local feature

    for feature in "$@" ; do
       if ! featurep "${feature}" ; then
          FEATURES="${FEATURES} ${feature}"
       fi
    done

    return 0
}


#:docstring require:
# Usage: require feature {file}
#
# Load FEATURE if it is not already provided.  Note that require does not
# call `provide' to register features.  The loaded file must do that
# itself.  If the package does not explicitly do a `provide' after being
# loaded, require will complain about the feature not being provided on
# stderr.
#
# Optional argument FILE means to try to load FEATURE from FILE.  If no
# file argument is given, require searches through FPATH (see fpath_search)
# for the appropriate file.
#
# If the variable REQUIRE_FAILURE_FATAL is set, require will cause the
# current shell invocation to exit, rather than merely return.  This may be
# useful for a shell script that vitally depends on a package. 
#
#:end docstring:

###;;;autoload
function require ()
{
 local feature="$1"
 local path="$2"
 local file
 
   if ! featurep "${feature}" ; then
      file=$(fpath_search "${feature}" "${path}") && source "${file}"

      if ! featurep "${feature}" ; then
         echo "require: ${feature}: feature was not provided." 1>&2
         if [ "${REQUIRE_FAILURE_FATAL+set}" = "set" ]; then
            exit 1
         fi
         return 1
      fi
   fi

   return 0
}

#:docstring fpath_search:
# Usage: fpath_search filename {path ...}
#
# Search $FPATH for `filename' or, if `path' (a list) is specified, search
# those directories instead of $FPATH.  First the path is searched for an
# occurrence of `filename.bash, then a second search is made for just
# `filename'.
#:end docstring:

###;;;autoload
function fpath_search ()
{
 local name="$1"
 local path="$2"
 local suffix=".bash"
 local file

    if [ -z "${path}" ]; then path="${FPATH}"; fi

    for file in "${name}${suffix}" "${name}" ; do
       set -- $(IFS=':'
                 set -- ${path}
                 for p in "$@" ; do
                    echo -n "${p:-.} "
                 done)

       while [ $# -ne 0 ]; do
          test -f "${1}/${file}" && { file="${1}/${file}"; break 2 }
          shift
       done
    done

    if [ $# -eq 0 ]; then
       echo "fpath_search: ${name}: file not found in fpath" 1>&2
       return 1
    fi

    echo "${file}"
    return 0
}

provide require

# require.bash ends here
