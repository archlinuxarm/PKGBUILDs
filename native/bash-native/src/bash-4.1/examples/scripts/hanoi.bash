# Towers of Hanoi in bash
#
# cribbed from the ksh93 book, example from exercises on page 85
#
# Chet Ramey
# chet@po.cwru.edu

hanoi() # n from to spare
{
    typeset -i nm1=$1-1
    ((nm1>0)) && hanoi $nm1 $2 $4 $3
    echo "Move disc $2 to $3"
    ((nm1>0)) && hanoi $nm1 $4 $3 $2
}

case $1 in
[1-9])
      hanoi $1 1 2 3;;
*)    echo "${0##*/}: Argument must be from 1 to 9"
      exit 1;;
esac
