#!/bin/bash
#	ash -- "Adventure shell"
#	last edit:	86/04/21	D A Gwyn
#	SCCS ID:	@(#)ash.sh	1.4

OPATH=$PATH

ask()
{
	echo -n "$@" '[y/n] '
	read ans

	case "$ans" in
	y*|Y*)
		return 0
		;;
	*)
		return 1
		;;
	esac
}
	
CAT=${PAGER:-more}

ash_inst()
{
	cat <<- EOF

	                Instructions for the Adventure shell

	Welcome to the Adventure shell!  In this exploration of the UNIX file
	system, I will act as your eyes and hands.  As you move around, I will
	describe whatever is visible and will carry out your commands.  The
	general form of a command is
	        Verb Object Extra_stuff.
	Most commands pay no attention to the "Extra_stuff", and many do not
	need an "Object".  A typical command is
	        get all
	which picks up all files in the current "room" (directory).  You can
	find out what you are carrying by typing the command
	        inventory
	The command "help" results in a full description of all commands that I
	understand.  To quit the Adventure shell, type
	        quit

	There are UNIX monsters lurking in the background.  These are also
	known as "commands with arguments".

	Good luck!
	EOF
}

ash_help()
{
echo "I understand the following commands (synonyms in parentheses):"
echo ""

echo "change OBJECT to NEW_NAME       changes the name of the object"
echo "clone OBJECT as NEW_NAME        duplicates the object"
echo "drop OBJECTS                    leaves the objects in the room"
echo "enter (go) PASSAGE              takes the labeled passage"
echo "examine OBJECTS                 describes the objects in detail"
echo "feed OBJECT to MONSTER          stuffs the object into a UNIX monster"
echo "get (take) OBJECTS              picks up the specified objects"
echo "gripe (bug)                     report a problem with the Adventure shell"
echo "help                            prints this summary"
echo "inventory (i)                   tells what you are carrying"
echo "kill (destroy) OBJECTS          destroys the objects"
echo "look (l)                        describes the room, including hidden objects"
echo "open (read) OBJECT              shows the contents of an object"
echo "quit (exit)                     leaves the Adventure shell"
echo "resurrect OBJECTS               attempts to restore dead objects"
echo "steal OBJECT from MONSTER       obtains the object from a UNIX monster"
echo "throw OBJECT at daemon          feeds the object to the printer daemon"
echo "up                              takes the overhead passage"
echo "wake MONSTER                    awakens a UNIX monster"
echo "where (w)                       tells you where you are"
echo "xyzzy                           moves you to your home"
}
	
MAINT=chet@ins.cwru.edu

PATH=/usr/ucb:/bin:/usr/bin:/usr/local/bin:.
export PATH

trap 'echo Ouch!' 2 3
#trap '' 18			# disable Berkeley job control

#ash_lk(){ echo " $1 " | fgrep " $2 " >&- 2>&-; }
ash_lk(){ echo " $1 " | fgrep -q " $2 " >/dev/null 2>&1 ; }
ash_pr(){ echo $* | tr ' ' '\012' | pr -5 -t -w75 -l$[ ( $# + 4 ) / 5 ]; }
ash_rm(){ echo " $1 " | sed -e "s/ $2 / /" -e 's/^ //' -e 's/ $//'; }

# enable history, bang history expansion, and emacs editing
set -o history
set -o histexpand
set -o emacs

cd
LIM=.limbo			# $HOME/$LIM contains "destroyed" objects
mkdir $LIM || {
	echo "ash: cannot mkdir $LIM: exiting"
	exit 1
}
KNAP=.knapsack			# $HOME/$KNAP contains objects being "carried"
if [ ! -d $KNAP ]
then	mkdir $KNAP >/dev/null 2>&1
	if [ $? = 0 ]
	then	echo 'You found a discarded empty knapsack.'
	else	echo 'You have no knapsack to carry things in.'
		exit 1
	fi
else	echo 'One moment while I peek in your old knapsack...'
fi

kn=`echo \`ls -a $KNAP | sed -e '/^\.$/d' -e '/^\.\.$/d'\``

if ask 'Welcome to the Adventure shell!  Do you need instructions?'
then
	ash_inst
	echo -n 'Type a newline to continue: '
	read
fi

wiz=false
cha=false
prev=$LIM
while :
do	room=`pwd`
	if [ $room != $prev ]
	then	if [ $room = $HOME ]
		then	echo 'You are in your own home.'
		else	echo "You have entered $room."
		fi
		exs=
		obs=
		hexs=
		hobs=
		f=false
		for i in `ls -a`
		do	case $i in
			.|..)	;;
			.*)	if [ -f $i ]
				then	hobs="$hobs $i"
				elif [ -d $i ]
				then	hexs="$hexs $i"
				else	f=true
				fi
				;;
			*)	if [ -f $i ]
				then	obs="$obs $i"
				elif [ -d $i ]
				then	exs="$exs $i"
				else	f=true
				fi
				;;
			esac
		done
		if [ "$obs" ]
		then	echo 'This room contains:'
			ash_pr $obs
		else	echo 'The room looks empty.'
		fi
		if [ "$exs" ]
		then	echo 'There are exits labeled:'
			ash_pr $exs
			echo 'as well as a passage overhead.'
		else	echo 'There is a passage overhead.'
		fi
		if sh -c $f
		then	echo 'There are shadowy figures in the corner.'
		fi
		prev=$room
	fi

	read -e -p '-advsh> ' verb obj x	# prompt is '-advsh> '
	if [ $? != 0 ]
	then	verb=quit		# EOF
	fi

	case $verb in
	change)		if [ "$obj" ]
			then	if ash_lk "$obs $hobs" "$obj"
				then	set -- $x
					case "$1" in
					to)	if [ "$2" ]
						then	if [ -f $2 ]
							then	echo "You must destroy $2 first."
								set --
							fi
							if [ "$2" ]
							then	if mv $obj $2 # >&- 2>&-
								then	echo "The $obj shimmers and turns into $2."
									obs=`ash_rm "$2 $obs" "$obj"`
								else	echo "There is a cloud of smoke but the $obj is unchanged."
								fi
							fi
						else	echo 'To what?'
						fi
						;;
					*)	echo "Change $obj to what?"
						;;
					esac
				else	if ash_lk "$kn" "$obj"
					then	echo 'You must drop it first.'
					else	echo "I see no $obj here."
					fi
				fi
			else	echo 'Change what?'
			fi
			;;
	clone)		if [ "$obj" ]
			then	if ash_lk "$obs $hobs" "$obj"
				then	if [ ! -r $obj ]
					then	echo "The $obj does not wish to be cloned."
					else	set -- $x
						case "$1" in
						as)	if [ "$2" ]
							then	if [ -f $2 ]
								then	echo "You must destroy $2 first."
								else	if cp $obj $2 # >&- 2>&-
									then	echo "Poof!  When the smoke clears, you see the new $2."
										obs="$obs $2"
									else	echo 'You hear a dull thud but no clone appears.'
									fi
								fi
							else	echo 'As what?'
							fi
							;;
						*)	echo "Clone $obj as what?"
							;;
						esac
					fi
				else	if ash_lk "$kn" "$obj"
					then	echo 'You must drop it first.'
					else	echo "I see no $obj here."
					fi
				fi
			else	echo 'Clone what?'
			fi
			;;
	drop)		if [ "$obj" ]
			then	for it in $obj $x
				do	if ash_lk "$kn" "$it"
					then	if [ -w $it ]
						then	echo "You must destroy $it first."
						else	if mv $HOME/$KNAP/$it $it # >&- 2>&-
							then	echo "$it: dropped."
								kn=`ash_rm "$kn" "$it"`
								obs=`echo $it $obs`
							else	echo "The $it is caught in your knapsack."
							fi
						fi
					else	echo "You're not carrying the $it!"
					fi
				done
			else	echo 'Drop what?'
			fi
			;;
	enter|go)	if [ "$obj" ]
			then	if [ $obj != up ]
				then	if ash_lk "$exs $hexs" "$obj"
					then	if [ -x $obj ]
						then	if cd $obj
							then	echo 'You squeeze through the passage.'
							else	echo "You can't go that direction."
							fi
						else	echo 'An invisible force blocks your way.'
						fi
					else	echo 'I see no such passage.'
					fi
				else	if cd ..
					then	echo 'You struggle upwards.'
					else	echo "You can't reach that high."
					fi
				fi
			else	echo 'Which passage?'
			fi
			;;
	examine)	if [ "$obj" ]
			then	if [ $obj = all ]
				then	$obj=`echo $obs $exs`
					x=
				fi
				for it in $obj $x
				do	if ash_lk "$obs $hobs $exs $hexs" "$it"
					then	echo "Upon close inspection of the $it, you see:"
						ls -ld $it 2>/dev/null
						if [ $? != 0 ]
						then	echo "-- when you look directly at the $it, it vanishes."
						fi
					else	if ash_lk "$kn" "$it"
						then	echo 'You must drop it first.'
						else	echo "I see no $it here."
						fi
					fi
				done
			else	echo 'Examine what?'
			fi
			;;
	feed)		if [ "$obj" ]
			then	if ash_lk "$obs $hobs" "$obj"
				then	set -- $x
					case "$1" in
					to)	if [ "$2" ]
						then	shift
							if PATH=$OPATH $* <$obj 2>/dev/null
							then	echo "The $1 monster devours your $obj."
								if rm -f $obj # >&- 2>&-
								then	obs=`ash_rm "$obs" "$obj"`
								else	echo 'But he spits it back up.'
								fi
							else	echo "The $1 monster holds his nose in disdain."
							fi
						else	echo 'To what?'
						fi
						;;
					*)	echo "Feed $obj to what?"
						;;
					esac
				else	if ash_lk "$kn" "$obj"
					then	echo 'You must drop it first.'
					else	echo "I see no $obj here."
					fi
				fi
			else	echo 'Feed what?'
			fi
			;;
	get|take)	if [ "$obj" ]
			then	if [ $obj = all ]
				then	obj="$obs"
					x=
				fi
				for it in $obj $x
				do	if ash_lk "$obs $hobs" "$it"
					then	if ash_lk "$kn" "$it"
						then	echo 'You already have one.'
						else	if mv $it $HOME/$KNAP/$it # >&- 2>&-
							then	echo "$it: taken."
								kn="$it $kn"
								obs=`ash_rm "$obs" "$it"`
							else	echo "The $it is too heavy."
							fi
						fi
					else	echo "I see no $it here."
					fi
				done
			else	echo 'Get what?'
			fi
			;;
	gripe|bug)	echo 'Please describe the problem and your situation at the time it failed.\nEnd the bug report with a line containing just a Ctrl-D.'
			cat | mail $MAINT -s 'ash bug'
			echo 'Thank you!'
			;;
	help)		ash_help
			;;
	inventory|i)	if [ "$kn" ]
			then	echo 'Your knapsack contains:'
				ash_pr $kn
			else	echo 'You are poverty-stricken.'
			fi
			;;
	kill|destroy)	if [ "$obj" ]
			then	if [ $obj = all ]
				then	x=
					if ask "Do you really want to attempt to $verb them all?"
					then	obj=`echo $obs`
					else	echo 'Chicken!'
						obj=
					fi
				fi
				for it in $obj $x
				do	if ash_lk "$obs $hobs" "$it"
					then	if mv $it $HOME/$LIM # <&- >&- 2>&-
						then	if [ $verb = kill ]
							then	echo "The $it cannot defend himself; he dies."
							else	echo "You have destroyed the $it; it vanishes."
							fi
							obs=`ash_rm "$obs" "$it"`
						else	if [ $verb = kill ]
							then	echo "Your feeble blows are no match for the $it."
							else	echo "The $it is indestructible."
							fi
						fi
					else	if ash_lk "$kn" "$it"
						then	echo "You must drop the $it first."
							found=false
						else	echo "I see no $it here."
						fi
					fi
				done
			else	echo 'Kill what?'
			fi
			;;
	look|l)		obs=`echo $obs $hobs`
			hobs=
			if [ "$obs" ]
			then	echo 'The room contains:'
				ash_pr $obs
			else	echo 'The room is empty.'
			fi
			exs=`echo $exs $hexs`
			hexs=
			if [ "$exs" ]
			then	echo 'There are exits plainly labeled:'
				ash_pr $exs
				echo 'and a passage directly overhead.'
			else	echo 'The only exit is directly overhead.'
			fi
			;;
	magic)		if [ "$obj" = mode ]
			then	if sh -c $cha
				then	echo 'You had your chance and you blew it.'
				else	if ask 'Are you a wizard?'
					then	echo -n 'Prove it!  Say the magic word: '
						read obj
						if [ "$obj" = armadillo ]
						then	echo 'Yes, master!!'
							wiz=true
						else	echo "Homie says: I don't think so"
							cha=true
						fi
					else	echo "I didn't think so."
					fi
				fi
			else	echo 'Nice try.'
			fi
			;;
	open|read)	if [ "$obj" ]
			then	if ash_lk "$obs $hobs" "$obj"
				then	if [ -r $obj ]
					then	if [ -s $obj ]
						then	echo "Opening the $obj reveals:"
							$CAT < $obj
							if [ $? != 0 ]
							then	echo '-- oops, you lost the contents!'
							fi
						else	echo "There is nothing inside the $obj."
						fi
					else	echo "You do not have the proper tools to open the $obj."
					fi
				else	if ash_lk "$kn" "$obj"
					then	echo 'You must drop it first.'
						found=false
					else	echo "I see no $obj here."
					fi
				fi
			else	echo 'Open what?'
			fi
			;;
	quit|exit)	if ask 'Do you really want to quit now?'
			then	if [ "$kn" ]
				then	echo 'The contents of your knapsack will still be there next time.'
				fi
				rm -rf $HOME/$LIM
				echo 'See you later!'
				exit 0
			fi
			;;
	resurrect)	if [ "$obj" ]
			then	for it in $obj $x
				do	if ash_lk "$obs $hobs" "$it"
					then	echo "The $it is already alive and well."
					else	if mv $HOME/$LIM/$it $it # <&- >&- 2>&-
						then	echo "The $it staggers to his feet."
							obs=`echo $it $obs`
						else	echo "There are sparks but no $it appears."
						fi
					fi
				done
			else	echo 'Resurrect what?'
			fi
			;;
	steal)		if [ "$obj" ]
			then	if ash_lk "$obs $hobs" "$obj"
				then	echo 'There is already one here.'
				else	set -- $x
					case "$1" in
					from)	if [ "$2" ]
						then	shift
							if PATH=$OPATH $* >$obj 2>/dev/null
							then	echo "The $1 monster drops the $obj."
								obs=`echo $obj $obs`
							else	echo "The $1 monster runs away as you approach."
								rm -f $obj # >&- 2>&-
							fi
						else	echo 'From what?'
						fi
						;;
					*)	echo "Steal $obj from what?"
						;;
					esac
				fi
			else	echo 'Steal what?'
			fi
			;;
	throw)		if [ "$obj" ]
			then	if ash_lk "$obs $hobs" "$obj"
				then	set -- $x
					case "$1" in
					at)	case "$2" in
						daemon)	if sh -c "lpr -r $obj"
							then	echo "The daemon catches the $obj, turns it into paper,\nand leaves it in the basket."
								obs=`ash_rm "$obs" "$obj"`
							else	echo "The daemon is nowhere to be found."
							fi
							;;
						*)	echo 'At what?'
							;;
						esac
						;;
					*)	echo "Throw $obj at what?"
						;;
					esac
				else	if ash_lk "$kn" "$obj"
					then	echo 'It is in your knapsack.'
						found=false
					else	echo "I see no $obj here."
					fi
				fi
			else	echo 'Throw what?'
			fi
			;;
	u|up)		if cd ..
			then	echo 'You pull yourself up a level.'
			else	echo "You can't reach that high."
			fi
			;;
	wake)		if [ "$obj" ]
			then	echo "You awaken the $obj monster:"
				PATH=$OPATH $obj $x
				echo 'The monster slithers back into the darkness.'
			else	echo 'Wake what?'
			fi
			;;
	w|where)	echo "You are in $room."
			;;
	xyzzy)		if cd
			then	echo 'A strange feeling comes over you.'
			else	echo 'Your spell fizzles out.'
			fi
			;;
	*)		if [ "$verb" ]
			then	if sh -c $wiz
				then	PATH=$OPATH $verb $obj $x
				else	echo "I don't know how to \"$verb\"."
					echo 'Type "help" for assistance.'
				fi
			else	echo 'Say something!'
			fi
			;;
	esac
done
