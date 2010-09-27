#! /bin/bash
#
#From: kaz@cafe.net (Kaz Kylheku)
#Newsgroups: comp.unix.shell
#Subject: Funky little bash script
#Message-ID: <6mspb9$ft2@espresso.cafe.net>
#Date: Thu, 25 Jun 1998 06:11:39 GMT

#Here is something I wrote a few years ago when I was bored one day.
#Warning: this contains control characters.

# Line input routine for GNU Bourne-Again Shell
# plus terminal-control primitives.
#
# by Kaz Kylheku
# June 1996, Vancouver, Canada


#
# Function to disable canonical input processing.
# Terminal modes are saved into variable "savetty"
#
#

function raw
{
	savetty=$(stty -g)
	stty -icanon -isig -echo -echok -echonl inlcr
}

#
# Function to restore terminal settings from savetty variable
#

function restore
{
	stty $savetty
}

#
# Set terminal MIN and TIME values.
# If the input argument is a zero, set up terminal to wait for
# a keystroke indefinitely. If the argument is non-zero, set up
# an absolute timeout of that many tenths of a second. The inter-keystroke
# timer facility of the terminal driver is not exploited.
#

function settimeout
# $1 = tenths of a second
{
	if [ "$1" = "0" ] ; then
		min=1
		timeout=0
	else
		min=0
		timeout="$1"
	fi

	stty min $min time $timeout

	unset min timeout
}

#
# Input a single key using 'dd' and echo it to standard output.
# Launching an external program to get a single keystroke is a bit
# of a pig, but it's the best you can do! Maybe we could convince the
# GNU guys to make 'dd' a bash builtin.
#

function getkey
{
	eval $1="\"\$(dd bs=1 count=1 2> /dev/null)\""
}

#
# Input a line of text gracefully.
# The first argument is the name of a variable where the input line is
# to be stored. If this variable is not empty, its contents are printed
# and treated as though the user had entered them.
# The second argument gives the maximum length of the input line; if it
# is zero, the input is unlimited (bad idea).
# ^W is used to delete words
# ^R redraws the line at any time by backspacing over it and reprinting it
# ^U backspaces to the beginning
# ^H or ^? (backspace or del) delete a single character
# ^M (enter) terminates the input
# all other control keys are ignored and cause a beep when pressed
# 
#


function getline
{
	settimeout 0			# No keystroke timeout.
	save_IFS="$IFS"			# Save word delimiter and set it to
	IFS=""				# to null so ${#line} works correctly.
	eval line=\${$1}		# Fetch line contents
	echo -n "$line"			# and print the existing line.
	while [ 1 ] ; do
		getkey key		# fetch a single keystroke
		case "$key" in
		 |  )				# BS or DEL
			if [ ${#line} != 0 ] ; then	# if line not empty
				echo -n " "		# print destructive BS
				line="${line%%?}"	# chop last character
			else				# else if line empty
				echo -n 		# beep the terminal
			fi
			;;
		 )					# kill to line beg
			while [ ${#line} != 0 ] ; do	# while line not empty
				echo -n " "		# print BS, space, BS
				line="${line%?}"	# shorten line by 1
			done
			;;
		 )					# redraw line
			linesave="$line"		# save the contents
			while [ ${#line} != 0 ] ; do	# kill to line beg
				echo -n " "
				line="${line%?}"
			done
			echo -n "$linesave"		# reprint, restore
			line="$linesave"
			unset linesave			# forget temp var
			;;
		 )
			while [ "${line% }" != "$line" ] && [ ${#line} != 0 ] ; do
				echo -n " "
				line="${line%?}"
			done
			while [ "${line% }" = "$line" ] && [ ${#line} != 0 ] ; do
				echo -n " "
				line="${line%?}"
			done
			;;
		 |  |  |  |  |  |  |  |  |  |  |  )
			echo -n 	# ignore various control characters
			;;		# with an annoying beep
		 |  |  |  |  |  |  |  |  |  |  |  |  )
			echo -n 
			;;
		'	' |  |  |  |  |  )
			echo -n 
			;;
		'' )			# Break out of loop on carriage return.
			echo		# Send a newline to the terminal.
			break		# (Also triggered by NUL char!).
			;;
		* )		# Append character to the end of the line.
				# If length is restricted, and the line is too
				# long, then beep...

			if [ "$2" != 0 ] && [ $(( ${#line} >= $2 )) = 1 ] ; then
				echo -n 
			else				# Otherwise add
				line="$line$key"	# the character.
				echo -n "$key"		# And echo it.
			fi
			;;
		esac
	done
	eval $1=\"\$line\"
	IFS="$save_IFS"
	unset line save_IFS
}

# uncomment the lines below to create a standalone test program
#
echo "Line input demo for the GNU Bourne-Again Shell."
echo "Hacked by Kaz Kylheku"
echo
echo "Use ^H/Backspace/Del to erase, ^W to kill words, ^U to kill the"
echo "whole line of input, ^R to redraw the line."
echo "Pass an argument to this program to prime the buffer contents"
raw
echo -n "go: "
if [ ${#1} != 0 ] ; then
	LINE=$1
fi
getline LINE 50
restore

echo "<$LINE>"
