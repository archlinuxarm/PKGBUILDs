#!/bin/sh
#for instructions or updates go to:
#<A HREF="http://math.ucr.edu:8889/">This script's home page</A>
#email me questions or comments at:
#<A HREF="mailto:insom@math.ucr.edu">insom@math.ucr.edu</A>
#copyright chris ulrich; This software may be used or modified
#in any way so long as this notice remains intact.
#
# WWW server in sh
# Author: Chris Ulrich <chris@tinker.ucr.edu>
#

INDEX=index.html
date=`date`
DOCHOME=/home/insom/web-docs
BINHOME=/home/insom/web-bin
LOGHOME=/home/insom/web-logs
LOGFILE=$LOGHOME/access_log
#verbose=:
verbose=echo
exec 2>> $LOGHOME/error_log

hheader() {
echo "HTTP/1.0 200 OK
Server: WebSH/2.00
Connection: close
Date: $date"
}

header() {
echo "Content-type: $1
"
}

no_url() {
  header "text/plain"
  echo "No such url $1"
}

send() {
  #case "$#" in 2) ;; *) echo eep! | mailx insom@math.ucr.edu  ; exit 3 ;; esac
  if test -f "$DOCHOME/$2"
  then
    header "$1"
    cat "$DOCHOME/$2"
  else
    no_url "$2"
  fi
}

LsToHTML() {
  if test -f "$DOCHOME/$url/.title"
  then
    header "text/html; charset=US-ASCII"
    echo "<pre>"
    cat "$DOCHOME/$url/.title"
    echo "</pre>"
  elif test -f "$DOCHOME/$url/.title.html"
  then
    header "text/html; charset=US-ASCII"
    cat "$DOCHOME/$url/.title.html"
  else
    header "text/html; charset=US-ASCII"
  fi

  case "$url" in 
     /) ;;
     *) url="$url/"
  esac

  while read link
  do
   case $link in
     *.cgi) ;;
     *)
        echo "<A HREF=\"$url$link\">$link</A> <BR>"
     ;;
   esac
  done
}

read method data

$verbose "
$date access from ${TCPREMOTEINFO:=NO-IDENT}@${TCPREMOTEHOST:=$TCPREMOTEIP}
	on local machine $TCPLOCALHOST
	 $method $data " >> $LOGFILE

for hopeurl in $data
do
  url="${url}${url:+ }$second"
  second="$hopeurl"
done

case "$second" in
  *[1-9].*) 
     read inheader
     while 
        case "$inheader" in 
          ?|'') false 
           ;;
          *) 
            read inheader
           ;; 
        esac 
     do
       :
     done
     hheader
  ;;
esac

case "$url" in
  *..*)
    no_url "$url"
    exit 1
   ;;
  *.txt|*.[ch])
    send "text/plain; charset=US-ASCII" "$url"
    ;;
  *.html)
    send "text/html; charset=US-ASCII" "$url"
   ;;
  *.cgi)
    if test -x "$DOCHOME/$url" 
    then 
      read message
      echo "$message" | "$DOCHOME/$url"
    else
      no_url "$url"
    fi
   ;;
  *".cgi?"*)
    oIFS="$IFS"
    echo "$url"  | { 
      IFS='?' read url QUERY_STRING
      if test -x "$DOCHOME/$url" 
      then
        IFS="$oIFS"
        export QUERY_STRING 
        "$DOCHOME/$url" 
      else
        no_url "$url"
      fi
       }
   ;;
  *.[Gg][Ii][Ff])
    send "image/gif" "$url"
   ;;
  *.[Jj][Pp][Gg]|*.[Jj][Pp][Ee][Gg])
    send "image/jpeg" "$url"
   ;;
  *.tbl)
    header "text/html; charset=US-ASCII"
    echo "<pre>"
    test -f "$DOCHOME/$url" && 
      tbl < "$DOCHOME/$url"  | nroff ||
      no_url "$url" 
    echo "</pre>"
   ;;
  *.nroff)
    header "text/html; charset=US-ASCII"
    echo "<pre>"
    test -f "$DOCHOME/$url" && 
      nroff < "$DOCHOME/$url" ||
      no_url "$url" 
    echo "</pre>"
   ;;
  *mp[23])
    if test -f "$DOCHOME/$url" 
    then
      header "application/mpstream"
      echo "+$TCPLOCALIP:${MPSERVPORT:=9001}/$url"
    else 
      no_url "$url" 
    fi
   ;;
  *.[0-9]|*.[0-9][a-z])
    header "text/html; charset=US-ASCII"
    echo "<pre>"
    if test -f "$DOCHOME/$url" 
    then
      #nroff -man  "$DOCHOME/$url" | $BINHOME/man2html
      echo "perl at the moment is broken, so man2html doesn't work.  Sorry."
      echo "</pre>"
    else 
      no_url "$url"
    fi
   ;;
  *.???|*.??)
    send "unknown/data" "$url"
   ;;
  */)
    if test -d "$DOCHOME/$url"
    then 
      ls "$DOCHOME/$url" | LsToHTML
    fi
   ;;
  *)
   if test -f "$DOCHOME/$url"
   then
     read filetype < "$DOCHOME/$url"
     case "$filetype" in
       \#!/*/*|\#!?/*/*)
         header "text/plain; charset=US-ASCII"
         cat "$DOCHOME/$url"
         ;;
       '<!*>')
         header "text/html; charset=US-ASCII"
         cat "$DOCHOME/$url"
        ;;
       *)
         header "text/html; charset=US-ASCII"
         echo "<pre>"
         cat "$DOCHOME/$url"
         echo "</pre>"
        ;;
     esac
   elif test -f "$DOCHOME/$url/$INDEX"
   then
     header "text/html; charset=US-ASCII"
     cat "$DOCHOME/$url/$INDEX"
   elif test -d "$DOCHOME/$url"
   then 
     ls "$DOCHOME/$url" | LsToHTML
   else
     no_url "$url" 
   fi
   ;;
esac
