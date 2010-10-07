#!/bin/sh

# this is a line editor using only /bin/sh, /bin/dd and /bin/rm

# /bin/rm is not really required, but it is nice to clean up temporary files

PATH=
dd=/bin/dd
rm=/bin/rm

# temporary files we might need
tmp=/tmp/silly.$$
ed=/tmp/ed.$$
trap "$rm -f $tmp $tmp.1 $tmp.2 $tmp.3 $tmp.4 $tmp.5 $tmp.6 $ed.a $ed.b $ed.c; exit" 0 1 2 3

# from now on, no more rm - the above trap is enough
unset rm

# we do interesting things with IFS, but better save it...
saveIFS="$IFS"

# in case "echo" is not a shell builtin...

Echo () {
case "$1" in
  -n) shift
      $dd of=$tmp 2>/dev/null <<EOF 
$@
EOF
      IFS="+"
      set `$dd if=$tmp bs=1 of=/dev/null skip=1 2>&1`
      IFS="$saveIFS"
      $dd if=$tmp bs=1 count=$1 2>/dev/null
      ;;
  *)  $dd 2>/dev/null <<EOF 
$@
EOF
      ;;
esac
}

# this is used to generate garbage files

true () {
  return 0
}

false () {
  return 1
}

zero () {
  ( trap 'go=false' 13
    go=true
    while $go
    do
      $dd "if=$0"
      case "$?" in
	0) ;;
	*) go=false ;;
      esac
    done
  ) 2>/dev/null
}

# arithmetic using dd!

# add variable n1 n2 n3...
# assigns n1+n2+n3+... to variable

add () {
  result="$1"
  shift
  $dd if=/dev/null of=$tmp bs=1 2>/dev/null
  for n in "$@"
  do
    case "$n" in
      0) ;;
      *) zero | $dd of=$tmp.1 bs=1 "count=$n" 2>/dev/null
	 ( $dd if=$tmp; $dd if=$tmp.1 ) 2>/dev/null | $dd of=$tmp.2 2>/dev/null
	 $dd if=$tmp.2 of=$tmp 2>/dev/null
	 ;;
    esac
  done
  IFS="+"
  set `$dd if=$tmp bs=1 of=/dev/null 2>&1`
  IFS="$saveIFS"
  eval $result='$1'
}

# subtract variable n1 n2
# subtracts n2 from n1, assigns result to variable

subtract () {
  result="$1"
  zero | $dd of=$tmp bs=1 "count=$2" 2>/dev/null
  IFS="+"
  set `$dd if=$tmp bs=1 of=/dev/null "skip=$3" 2>&1`
  IFS="$saveIFS"
  case "$1" in
    dd*) set 0 ;;
  esac
  eval $result='$1'
}

# multiply variable n1 n2
# variable = n1 * n2

multiply () {
  result="$1"
  zero | $dd "bs=$2" of=$tmp "count=$3" 2>/dev/null
  IFS="+"
  set `$dd if=$tmp bs=1 of=/dev/null 2>&1`
  IFS="$saveIFS"
  eval $result='$1'
}

# divide variable n1 n2
# variable = int( n1 / n2 )

divide () {
  result="$1"
  zero | $dd bs=1 of=$tmp "count=$2" 2>/dev/null
  IFS="+"
  set `$dd if=$tmp "bs=$3" of=/dev/null 2>&1`
  IFS="$saveIFS"
  eval $result='$1'
}

# compare variable n1 n2 sets variable to lt if n1<n2, gt if n1>n2, eq if n1==n2

compare () {
  res="$1"
  n1="$2"
  n2="$3"
  subtract somename "$n1" "$n2"
  case "$somename" in
    0) ;;
    *) eval $res=gt; return;
  esac
  subtract somename "$n2" "$n1"
  case "$somename" in
    0) ;;
    *) eval $res=lt; return;
  esac
  eval $res=eq
}

# lt n1 n2 returns true if n1 < n2

lt () {
  n1="$1"
  n2="$2"
  subtract somename "$n2" "$n1"
  case "$somename" in
    0) return 1 ;;
  esac
  return 0
}

# le n1 n2 returns true if n1 <= n2

le () {
  n1="$1"
  n2="$2"
  subtract somename "$n1" "$n2"
  case "$somename" in
    0) return 0 ;;
  esac
  return 1
}

# gt n1 n2 returns true if n1 > n2

gt () {
  n1="$1"
  n2="$2"
  subtract somename "$n1" "$n2"
  case "$somename" in
    0) return 1 ;;
  esac
  return 0
}

# ge n1 n2 returns true if n1 >= n2

ge () {
  n1="$1"
  n2="$2"
  subtract somename "$n2" "$n1"
  case "$somename" in
    0) return 0 ;;
  esac
  return 1
}

# useful functions for the line editor

# open a file - copy it to the buffers

open () {
  file="$1"
  set `$dd "if=$file" of=/dev/null 2>&1`
  case "$1" in
    dd*) return 1
  esac
  # copy the first line to $ed.c
  go=true
  len=0
  while $go
  do
    case "`$dd "if=$file" bs=1 skip=$len count=1 2>/dev/null`" in
      ?*) go=true ;;
      *) go=false ;;
    esac
    add len 1 $len
  done
  # now $len is the length of the first line (including newline)
  $dd "if=$file" bs=1 count=$len of=$ed.c 2>/dev/null
  $dd "if=$file" bs=1 skip=$len of=$ed.b 2>/dev/null
  $dd if=/dev/null of=$ed.a 2>/dev/null
  lineno=1
}

# save a file - copy the buffers to the file

save () {
  # make a backup copy of the original
  $dd "if=$1" "of=$1.bak" 2>/dev/null
  # and save
  ( $dd if=$ed.a; $dd if=$ed.c; $dd if=$ed.b ) > "$1" 2>/dev/null
}

# replace n1 n2 bla replaces n2 chars of current line, starting n1-th

replace () {
  $dd if=$ed.c of=$tmp.1 bs=1 "count=$1" 2>/dev/null
  ( $dd if=$ed.c "skip=$1" bs=1 | $dd of=$tmp.2 bs=1 "skip=$2" ) 2>/dev/null
  shift
  shift
  ( $dd if=$tmp.1; Echo -n "$@"; $dd if=$tmp.2 ) > $tmp.3 2>/dev/null
  $dd if=$tmp.3 of=$ed.c 2>/dev/null
}

# rstring n s bla
# replace the n-th occurence of s with bla

rstring () {
  n="$1"
  shift;
  # first we have to find it - this is fun!
  # we have $tmp.4 => text before string, $tmp.5 => text after
  $dd if=/dev/null of=$tmp.4 2>/dev/null
  $dd if=$ed.c of=$tmp.5 2>/dev/null
  string="$1"
  shift
  $dd of=$tmp.6 2>/dev/null <<EOF
$@
EOF
  while :
  do
    case "`$dd if=$tmp.5 2>/dev/null`" in
      $string*)
	  if lt $n 2
	  then
	    # now we want to replace the string
	    Echo -n "$@" > $tmp.2
	    Echo -n "$string" > $tmp.1
	    IFS="+"
	    set `$dd bs=1 if=$tmp.1 of=/dev/null 2>&1`
	    IFS="$saveIFS"
	    slen=$1
	    IFS="+"
	    ( $dd if=$tmp.4; $dd if=$tmp.2; $dd if=$tmp.5 bs=1 skip=$slen ) \
		  2>/dev/null > $tmp
	    $dd if=$tmp of=$ed.c 2>/dev/null
	    return 0
	  else
	    subtract n $n 1
	    ( $dd if=$tmp.4; $dd if=$tmp.5 bs=1 count=1 ) > $tmp 2>/dev/null
	    $dd if=$tmp of=$tmp.4 2>/dev/null
	    # and remove it from $tmp.5
	    $dd if=$tmp.5 of=$tmp bs=1 skip=1 2>/dev/null
	    $dd if=$tmp of=$tmp.5 2>/dev/null
	  fi
	  ;;
      ?*) # add one more byte...
	  ( $dd if=$tmp.4; $dd if=$tmp.5 bs=1 count=1 ) > $tmp 2>/dev/null
	  $dd if=$tmp of=$tmp.4 2>/dev/null
	  # and remove it from $tmp.5
	  $dd if=$tmp.5 of=$tmp bs=1 skip=1 2>/dev/null
	  $dd if=$tmp of=$tmp.5 2>/dev/null
	  ;;
      *)  # not found
	  return 1
	  ;;
    esac
  done
}

# skip to next line
next () {
  add l $lineno 1
  ( $dd if=$ed.a; $dd if=$ed.c ) 2>/dev/null > $tmp.3
  $dd if=$ed.b of=$tmp.4 2>/dev/null
  open $tmp.4
  $dd if=$tmp.3 of=$ed.a 2>/dev/null
  lineno=$l
}

# delete current line
delete () {
  l=$lineno
  $dd if=$ed.a 2>/dev/null > $tmp.1
  $dd if=$ed.b of=$tmp.2 2>/dev/null
  open $tmp.2
  $dd if=$tmp.1 of=$ed.a 2>/dev/null
  lineno=$l
}

# insert before current line (without changing current)
insert () {
  ( $dd if=$ed.a; Echo "$@" ) 2>/dev/null > $tmp.1
  $dd if=$tmp.1 of=$ed.a 2>/dev/null
  add lineno $lineno 1
}

# previous line
prev () {
  case "$lineno" in
    1) ;;
    *) subtract lineno $lineno 1
       # read last line of $ed.a
       IFS='+'
       set `$dd if=$ed.a of=/dev/null bs=1 2>&1`
       IFS="$saveIFS"
       size=$1
       # empty?
       case "$size" in
	 0) return ;;
       esac
       subtract size $size 1
       # skip final newline
       case "$size" in
	 0) ;;
	 *) subtract size1 $size 1
	    case "`$dd if=$ed.a bs=1 skip=$size count=1 2>/dev/null`" in
	      ?*) ;;
	      *) size=$size1 ;;
	    esac
	    ;;
       esac
       go=true
       while $go
       do
	 case "$size" in
	   0) go=false ;;
	   *) case "`$dd if=$ed.a bs=1 skip=$size count=1 2>/dev/null`" in
	        ?*)  go=true; subtract size $size 1 ;;
	        *)   go=false; add size $size 1 ;;
	      esac
	      ;;
	 esac
       done
       # now $size is the size of the first n-1 lines
       # add $ed.c to $ed.b
       ( $dd if=$ed.c; $dd if=$ed.b ) 2>/dev/null > $tmp.5
       $dd if=$tmp.5 of=$ed.b 2>/dev/null
       # move line to ed.c
       case "$size" in
	 0) $dd if=$ed.a of=$ed.c 2>/dev/null
	    $dd if=/dev/null of=$tmp.5 2>/dev/null
	    ;;
	 *) $dd if=$ed.a of=$ed.c bs=1 skip=$size 2>/dev/null
	    $dd if=$ed.a of=$tmp.5 bs=1 count=$size 2>/dev/null
	    ;;
       esac
       # move rest to ed.a
       $dd if=$tmp.5 of=$ed.a 2>/dev/null
    ;;
  esac
}

# goes to a given line
goto () {
  rl="$1"
  compare bla "$rl" $lineno
  case "$bla" in
    eq) return
	;;
    gt) while gt "$rl" $lineno
	do
	  next
	done
	;;
    lt) while lt "$rl" $lineno
	do
	  prev
	done
	;;
  esac
}

lineout () {
  Echo -n "$lineno: "
  $dd if=$ed.c 2>/dev/null
}

state=closed
name=
autoprint=true

while true
do
  Echo -n '> '
  read cmd arg
  case "$cmd:$state" in
    open:open) Echo "There is a file open already" ;;
    open:*) if open "$arg"
	    then state=open; name="$arg"; $autoprint
	    else Echo "Cannot open $arg"
	    fi
	    ;;
    new:open) Echo "There is a file open already" ;;
    new:*)  open "$arg"
	    state=open
	    name="$arg"
	    $autoprint
	    ;;
    close:changed) Echo "Use 'discard' or 'save'" ;;
    close:closed) Echo "Closed already" ;;
    close:*) state=closed ;;
    save:closed) Echo "There isn't a file to save" ;;
    save:*) case "$arg" in
	      ?*) save "$arg" ;;
	      *) save "$name" ;;
	    esac
	    state=open
	    ;;
    discard:changed) Echo "Your problem!"; state=closed ;;
    discard:*) state=closed ;;
    print:closed) Echo "No current file" ;;
    print:*) lineout ;;
    goto:closed) Echo "No current file" ;;
    goto:*) goto "$arg"; $autoprint ;;
    next:closed) Echo "No current file" ;;
    next:*) next; $autoprint ;;
    prev:closed) Echo "No current file" ;;
    prev:*) prev; $autoprint ;;
    name:closed) Echo "No current file" ;;
    name:*) name="$arg" ;;
    replace:closed) Echo "No current file" ;;
    replace:*) if rstring 1 $arg
	       then state=changed; $autoprint
	       else Echo "Not found"
	       fi
	       ;;
    nreplace:closed) Echo "No current file" ;;
    nreplace:*) if rstring $arg
		then state=changed; $autoprint
		else Echo "Not found"
		fi
		;;
    delete:closed) Echo "No current file" ;;
    delete:*) delete; state=changed; $autoprint ;;
    insert:closed) Echo "No current file" ;;
    insert:*) insert "$arg"; prev; state=changed; $autoprint ;;
    quit:changed) Echo "Use 'save' or 'discard'" ;;
    quit:*) Echo "bye"; exit;;
    autoprint:*) autoprint="lineout" ;;
    noprint:*) autoprint="" ;;
    :*) ;;
    *) Echo "Command not understood" ;;
  esac
done

