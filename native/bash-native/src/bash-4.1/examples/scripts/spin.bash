#!/bin/bash
#
# spin.bash -- provide a `spinning wheel' to show progress
#
# Chet Ramey
# chet@po.cwru.edu
#
bs=$'\b'
 
chars="|${bs} \\${bs} -${bs} /${bs}"
 
# Infinite loop for demo. purposes
while :
do
    for letter in $chars
    do
        echo -n ${letter}
    done
done

exit 0
