#! /bin/bash
#
# center - center a group of lines
#
# tabs in the lines might cause this to look a little bit off
#
#

width=${COLUMNS:-80}

if [[ $# == 0 ]]
then
	set -- /dev/stdin
fi

for file
do
	while read -r
	do
		printf "%*s\n" $(( (width+${#REPLY})/2 )) "$REPLY"
	done < $file
done

exit 0
