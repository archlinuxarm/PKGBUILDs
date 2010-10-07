#From: "Grigoriy Strokin" <grg@philol.msu.ru>
#Newsgroups: comp.unix.shell
#Subject: BASH: getopt function that parses long-named options
#Date: Mon, 22 Dec 1997 20:35:18 +0300

#Hi, I have written a BASH function named getoptex, that is like bash builtin
#"getopts", but does parse long-named options and optional arguments. It only
#uses builtin bash commands, so it is very fast.  In order to use it in your
#bash scripts, include a command ". getopt.sh" (<dot> getopt.sh) to the file
#containing your script, and that will define functions getopt, getoptex, and
#optlistex (the file getopt.sh with its detailed description is listed
#below).

#*** file getopt.sh ***

#! /bin/bash
#
# getopt.sh:
# functions like getopts but do long-named options parsing
# and support optional arguments
#
# Version 1.0 1997 by Grigoriy Strokin (grg@philol.msu.ru), Public Domain
# Date created:  December 21, 1997
# Date modified: December 21, 1997
#
# IMPORTANT FEATURES
#
# 1) Parses both short and long-named options
# 2) Supports optional arguments
# 3) Only uses bash builtins, thus no calls to external
#    utilities such as expr or sed is done. Therefore,
#    parsing speed is high enough
#
#
# DESCRIPTION
#
# FUNCTION getopt
# Usage: getopt OPTLIST {"$@"|ALTERNATIVE_PARAMETERS}
#
# like getopts, but parse options with both required and optional arguments,
# Options with optional arguments must have "." instead of ":" after them.
# Furthemore, a variable name to place option name cannot be specified
# and is always placed in OPTOPT variable
#
# This function is provided for compatibility with getopts()
# OPTLIST style, and it actually calls getoptex (see bellow)
#
# NOTE that a list of parameters is required and must be either "$@",
# if processing command line arguments, or some alternative parameters.
#
# FUNCTION getoptex
# Usage: getoptex OPTION_LIST {"$@"|ALTERNATIVE_PARAMETERS}
#
# like getopts, but parse long-named options.
#
# Both getopt and getoptex return 0 if an option has been parsed,
# and 1 if all options are already parsed or an error occured
#
# Both getopt and getoptex set or test the following variables:
#
# OPTERR -- tested for whether error messages must be given for invalid
options
#
# OPTOPT -- set to the name of an option parsed,
#           or to "?" if no more options or error
# OPTARG -- set to the option argument, if any;
#           unset if ther is no argument;
#           on error, set to the erroneous option name
#
# OPTIND -- Initialized to 1.
#           Then set to the number of the next parameter to be parsed
#           when getopt or getoptex will be called next time.
#           When all options are parsed, contains a number of
#           the first non-option argument.
#
#
# OPTOFS -- If a parameter number $OPTIND containg an option parsed
#           does not contain any more options, OPTOFS is unset;
#           otherwise, OPTOFS is set to such a number of "?" signs
#           which is equal to the number of options parsed
#
#           You might not set variables OPTIND and OPTOFS yourself
#           unless you want to parse a list of parameters more than once.
#           Otherwise, you whould unset OPTIND (or set it to 1)
#           and unset OPTOFS each time you want to parse a new parameters
list
#
# Option list format is DIFFERENT from one for getopts or getopt.
getopts-style
# option list can be converted to getoptex-style using a function optlistex
# (see bellow)
#
# DESCRIPTION of option list used with getoptex:
# Option names are separated by whitespace. Options consiting of
# more than one character are treated as long-named (--option)
#
# Special characters can appear at the and of option names specifying
# whether an argument is required (default is ";"):
# ";" (default) -- no argument
# ":" -- required argument
# "," -- optional argument
#
# For example, an option list "a b c help version f: file: separator."
# defines the following options:
#    -a, -b, -c, --help, --version -- no argument
#    -f, --file -- argument required
#    --separator -- optional argument
#
# FUNCTION optlistex
# Usage new_style_optlist=`optlistex OLD_STYLE_OPTLIST`
#
# Converts getopts-style option list in a format suitable for use with getoptex
# Namely, it inserts spaces after each option name.
#
#
# HOW TO USE
#
# In order o use in your bash scripts the functions described,
# include a command ". getopt.sh" to the file containing the script,
# which will define functions getopt, getoptex, and optlistex
#
# EXAMPLES
#
# See files 'getopt1' and 'getopt2' that contain sample scripts that use
# getopt and getoptex functions respectively
#
#
# Please send your comments to grg@philol.msu.ru

function getoptex()
{
  let $# || return 1
  local optlist="${1#;}"
  let OPTIND || OPTIND=1
  [ $OPTIND -lt $# ] || return 1
  shift $OPTIND
  if [ "$1" != "-" ] && [ "$1" != "${1#-}" ]
  then OPTIND=$[OPTIND+1]; if [ "$1" != "--" ]
  then
    local o
    o="-${1#-$OPTOFS}"
    for opt in ${optlist#;}
    do
      OPTOPT="${opt%[;.:]}"
      unset OPTARG
      local opttype="${opt##*[^;:.]}"
      [ -z "$opttype" ] && opttype=";"
      if [ ${#OPTOPT} -gt 1 ]
      then # long-named option
        case $o in
          "--$OPTOPT")
            if [ "$opttype" != ":" ]; then return 0; fi
            OPTARG="$2"
            if [ -z "$OPTARG" ];
            then # error: must have an agrument
              let OPTERR && echo "$0: error: $OPTOPT must have an argument" >&2
              OPTARG="$OPTOPT";
              OPTOPT="?"
              return 1;
            fi
            OPTIND=$[OPTIND+1] # skip option's argument
            return 0
          ;;
          "--$OPTOPT="*)
            if [ "$opttype" = ";" ];
            then  # error: must not have arguments
              let OPTERR && echo "$0: error: $OPTOPT must not have arguments" >&2
              OPTARG="$OPTOPT"
              OPTOPT="?"
              return 1
            fi
            OPTARG=${o#"--$OPTOPT="}
            return 0
          ;;
        esac
      else # short-named option
        case "$o" in
          "-$OPTOPT")
            unset OPTOFS
            [ "$opttype" != ":" ] && return 0
            OPTARG="$2"
            if [ -z "$OPTARG" ]
            then
              echo "$0: error: -$OPTOPT must have an argument" >&2
              OPTARG="$OPTOPT"
              OPTOPT="?"
              return 1
            fi
            OPTIND=$[OPTIND+1] # skip option's argument
            return 0
          ;;
          "-$OPTOPT"*)
            if [ $opttype = ";" ]
            then # an option with no argument is in a chain of options
              OPTOFS="$OPTOFS?" # move to the next option in the chain
              OPTIND=$[OPTIND-1] # the chain still has other options
              return 0
            else
              unset OPTOFS
              OPTARG="${o#-$OPTOPT}"
              return 0
            fi
          ;;
        esac
      fi
    done
    echo "$0: error: invalid option: $o"
  fi; fi
  OPTOPT="?"
  unset OPTARG
  return 1
}
function optlistex
{
  local l="$1"
  local m # mask
  local r # to store result
  while [ ${#m} -lt $[${#l}-1] ]; do m="$m?"; done # create a "???..." mask
  while [ -n "$l" ]
  do
    r="${r:+"$r "}${l%$m}" # append the first character of $l to $r
    l="${l#?}" # cut the first charecter from $l
    m="${m#?}"  # cut one "?" sign from m
    if [ -n "${l%%[^:.;]*}" ]
    then # a special character (";", ".", or ":") was found
      r="$r${l%$m}" # append it to $r
      l="${l#?}" # cut the special character from l
      m="${m#?}"  # cut one more "?" sign
    fi
  done
  echo $r
}
function getopt()
{
  local optlist=`optlistex "$1"`
  shift
  getoptex "$optlist" "$@"
  return $?
}

#**************************************
#     cut here
#**************************************
#*** (end of getopt.sh) ***


#*** file getopt1 ***

#! /bin/bash
# getopt1:
# Sample script using the function getopt
#
# Type something like "getopt1 -ab -d 10 -e20 text1 text2"
# on the command line to see how it works
#
# See getopt.sh for more information
#. getopt.sh
#echo Using getopt to parse arguments:
#while getopt "abcd:e." "$@"
#do
#  echo "Option <$OPTOPT> ${OPTARG:+has an arg <$OPTARG>}"
#done
#shift $[OPTIND-1]
#for arg in "$@"
#do
#  echo "Non option argument <$arg>"
#done
#
#**************************************
#        cut here
#**************************************
#*** (end of getopt1) ***
#
#
#*** file getopt2 ***
#
#! /bin/bash
# getopt2:
# Sample script using the function getoptex
#
# Type something like "getopt2 -ab -d 10 -e20 --opt1 --opt4=100 text1 text2"
# to see how it works
#
# See getopt.sh for more information
. getopt.sh
#echo Using getoptex to parse arguments:
#while getoptex "a; b; c; d: e. opt1 opt2 opt3 opt4: opt5." "$@"
#do
#  echo "Option <$OPTOPT> ${OPTARG:+has an arg <$OPTARG>}"
#done
#shift $[OPTIND-1]
#for arg in "$@"
#do
#  echo "Non option argument <$arg>"
#done
#
#**************************************
#         cut here
#**************************************
#*** (end of getopt2) ***

