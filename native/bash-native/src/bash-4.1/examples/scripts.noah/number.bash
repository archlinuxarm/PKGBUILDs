# number.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1993-02-22
# Last modified: 1993-04-01
# Public domain

# Conversion to bash v2 syntax done by Chet Ramey

# Commentary:
# Code:

#:docstring number:
# Usage: number [number]
#
# Converts decimal integers to english notation.  Spaces and commas are
# optional.  Numbers 67 digits and larger will overflow this script.
#
# E.g: number 99,000,000,000,000,454
#      => ninety-nine quadrillion four hundred fifty-four
#
#:end docstring:

function number ()
{
 local result
 local val1
 local val2
 local val3
 local d1
 local d2
 local d3

   case "$*" in
      *[!0-9,.]* ) 
         echo "number: invalid character in argument." 1>&2
         return 1 
        ;;
      *.* ) 
         echo "number: fractions not supported (yet)." 1>&2
         return 1 
        ;;
   esac

   result=''

   eval set - "`echo ${1+\"$@\"} | sed -n -e '
      s/[, ]//g;s/^00*/0/g;s/\(.\)\(.\)\(.\)$/\"\1 \2 \3\"/;
      :l
      /[0-9][0-9][0-9]/{
         s/\([^\" ][^\" ]*\)\([^\" ]\)\([^\" ]\)\([^\" ]\)/\1\"\2 \3 \4\"/g;
         t l
      }
      /^[0-9][0-9][0-9]/s/\([^\" ]\)\([^\" ]\)\([^\" ]\)/\"\1 \2 \3\"/;
      /^[0-9][0-9]/s/\([^\" ]\)\([^\" ]\)/\"\1 \2\"/;
      /^[0-9]/s/^\([^\" ][^\" ]*\)/\"\1\"/g;s/\"\"/\" \"/g;p;'`"

   while test $# -ne 0 ; do
      eval `set - $1; 
            d3='' d2='' d1=''
            case $# in
               1 ) d1=$1 ;;
               2 ) d2=$1 d1=$2 ;;
               3 ) d3=$1 d2=$2 d1=$3 ;;
            esac
            echo "d3=\"${d3}\" d2=\"${d2}\" d1=\"${d1}\""`

      val1='' val2='' val3=''

      case "${d3}" in
         '1' ) val3='one'   ;;
         '2' ) val3='two'   ;;
         '3' ) val3='three' ;;
         '4' ) val3='four'  ;;
         '5' ) val3='five'  ;;
         '6' ) val3='six'   ;;
         '7' ) val3='seven' ;;
         '8' ) val3='eight' ;;
         '9' ) val3='nine'  ;;
      esac

      case "${d2}" in
         '1' ) val2='teen'    ;;
         '2' ) val2='twenty'  ;;
         '3' ) val2='thirty'  ;;
         '4' ) val2='forty'   ;;
         '5' ) val2='fifty'   ;;
         '6' ) val2='sixty'   ;;
         '7' ) val2='seventy' ;;
         '8' ) val2='eighty'  ;;
         '9' ) val2='ninety'  ;;
      esac

      case "${val2}" in
         'teen')
            val2=''
            case "${d1}" in
               '0') val1='ten'       ;;
               '1') val1='eleven'    ;;
               '2') val1='twelve'    ;;
               '3') val1='thirteen'  ;;
               '4') val1='fourteen'  ;;
               '5') val1='fifteen'   ;;
               '6') val1='sixteen'   ;;
               '7') val1='seventeen' ;;
               '8') val1='eighteen'  ;;
               '9') val1='nineteen'  ;;
            esac
           ;;
         0 ) : ;;
         * )
            if test ".${val2}" != '.' && test ".${d1}" != '.0' ; then
               val2="${val2}-"
            fi
            case "${d1}" in
               '0') val2="${val2} " ;;
               '1') val1='one'    ;;
               '2') val1='two'    ;;
               '3') val1='three'  ;;
               '4') val1='four'   ;;
               '5') val1='five'   ;;
               '6') val1='six'    ;;
               '7') val1='seven'  ;;
               '8') val1='eight'  ;;
               '9') val1='nine'   ;;
            esac
           ;;
      esac

      if test ".${val3}" != '.' ; then
         result="${result}${val3} hundred "
      fi

      if test ".${val2}" != '.' ; then
         result="${result}${val2}"
      fi

      if test ".${val1}" != '.' ; then
         result="${result}${val1} "
      fi

      if test ".${d1}${d2}${d3}" != '.000' ; then
         case $# in
             0 | 1 ) ;;
             2 ) result="${result}thousand " ;;
             3 ) result="${result}million " ;;
             4 ) result="${result}billion " ;;
             5 ) result="${result}trillion " ;;
             6 ) result="${result}quadrillion " ;;
             7 ) result="${result}quintillion " ;;
             8 ) result="${result}sextillion " ;;
             9 ) result="${result}septillion " ;;
            10 ) result="${result}octillion " ;;
            11 ) result="${result}nonillion " ;;
            12 ) result="${result}decillion " ;;
            13 ) result="${result}undecillion " ;;
            14 ) result="${result}duodecillion " ;;
            15 ) result="${result}tredecillion " ;;
            16 ) result="${result}quattuordecillion " ;;
            17 ) result="${result}quindecillion " ;;
            18 ) result="${result}sexdecillion " ;;
            19 ) result="${result}septendecillion " ;;
            20 ) result="${result}octodecillion " ;;
            21 ) result="${result}novemdecillion " ;;
            22 ) result="${result}vigintillion " ;;
            * )
               echo "Error: number too large (66 digits max)." 1>&2
               return 1
              ;;
         esac
      fi

      shift
   done

   set - ${result}
   case "$*" in
      '') set - 'zero' ;;
   esac

   echo ${1+"$@"}
}

provide number

# number.bash ends here
