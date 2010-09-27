# 1-Feb-86 09:37:35-MST,30567;000000000001
# Return-Path: <unix-sources-request@BRL.ARPA>
# Received: from BRL-TGR.ARPA by SIMTEL20.ARPA with TCP; Sat 1 Feb 86 09:36:16-MST
# Received: from usenet by TGR.BRL.ARPA id a002623; 1 Feb 86 9:33 EST
# From: chris <chris@globetek.uucp>
# Newsgroups: net.sources
# Subject: Improved Bcsh (Bourne Shell Cshell-Emulator)
# Message-ID: <219@globetek.UUCP>
# Date: 30 Jan 86 17:34:26 GMT
# To:       unix-sources@BRL-TGR.ARPA
#
# This is a new, improved version of my Bourne shell cshell-emulator.
# The code has been cleaned up quite a bit, and a couple of new features
# added (now supports 'noclobber' and 'iclobber' variables).  A bug with
# 'eval' that caused "illegal I/O" error messages on vanilla V7 shells has
# also been fixed.

# I have posted the program in its entirety because a context diff of the
# old and new versions was longer than the new version...

# --Chris
#	Bcsh -- A Simple Cshell-Like Command Pre-Processor For The Bourne Shell
#
#	"Copyright (c) Chris Robertson, December 1985"
#
#	This software may be used for any purpose provided the original
#	copyright notice and this notice are affixed thereto.  No warranties of
#	any kind whatsoever are provided with this software, and it is hereby
#	understood that the author is not liable for any damagages arising
#	from the use of this software.
#
#	Features Which the Cshell Does Not Have:
#	----------------------------------------
#
#	+  command history persists across bcsh sessions
# 	+  global last-command editing via 'g^string1^string2^' syntax
#	+  edit any command via $EDITOR or $VISUAL editors
#	+  history file name, .bcshrc file name, alias file name, and number
#	   of commands saved on termination can be set by environment variables
#	+  prompt may evaluate commands, such as `pwd`, `date`, etc.
#	+  the whole text of interactive 'for' and 'while' loops and 'if'
#	   statements goes into the history list and may be re-run or edited
#	+  multiple copies of commands and requests to see command history
#	   are not added to the history list
#	+  the history mechanism actually stores all commands entered in a
#	   current session, not just $history of them.  This means that you
#	   can increase $history on the fly and at once have a larger history.
#
#
#	Synonyms:
#	---------
#
#	logout, exit, bye	write out history file and exit
#	h, history		show current history list
#	
#	
#	Aliases:
#	--------
#
#	alias NAME CMND		create an alias called NAME to run CMND
#	unalias NAME		remove the alias NAME
#
#	There are no 'current-session only' aliases -- all alias and unalias
#	commands are permanent, and stored in the $aliasfile.
#
#	If an alias contains positional variables -- $1, $2, $*, etc. -- any
#	arguments following the alias name are considered to be values for
#	those variables, and the alias is turned into a command of the form
#	'set - arguments;alias'.  Otherwise, a simple substitution is performed
#	for the alias and the rest of the command preserved.  The cshell
#	convention of using '\!:n' in an alias to get bits of the current
#	command is mercifully abandoned.
#
#	Quotes are not necessary around the commands comprising an alias;
#	in fact, any enclosing quotes are stripped when the alias is added
#	to the file.
#
#	A couple of typical aliases might be:
#
#		goto	cd $1;pwd
#		l	ls -F
#
#	Note that aliasing something to "commands;logout" will not work -- if
#	you want something to happen routinely on logout put it in the file
#	specified by $logoutfile, default = $HOME/.blogout.
#
#
#	Command Substitutions:
#	----------------------
#
#	!!			substitute last command from history list
#	!!:N			substitute Nth element of last command from
#				history list -- 0 = command name, 1 = 1st arg
# 	!!:$			substitute last element of last command from
#				history list
# 	!!:*			substitute all arguments to last command
#				from history list
#	!NUMBER			substitute command NUMBER from the history list
#	!NUMBER:N		as above, but substitute Nth element, where
#				0 = command name, 1 = 1st arg, etc.
# 	!NUMBER:$		as above, but substitute last element
# 	!NUMBER:*		as above, but substitute all arguments
#	!-NUMBER		substitute the command NUMBER lines from the
#				end of the history list; 1 = last command
#	!-NUMBER:N		as above, but substitute Nth element, where
#				0 = command name, 1 = 1st arg, etc.
# 	!-NUMBER:$		as above, but substitute last element
# 	!-NUMBER:*		as above, but substitute all arguments
#	!?STRING		substitute most-recent command from history list
#				containing STRING -- STRING must be enclosed in
#				braces if followed by any other characters
#	!?STRING:N		as above, but substitute Nth element, where
#				0 = command name, 1 = 1st arg, etc.
# 	!?STRING:$		as above, but substitute last element	
# 	!?STRING:*		as above, but substitute all arguments
#
#
#	Command Editing:
#	----------------
#
#	CMND~e			edit CMND using $EDITOR, where CMND may be found
#				using a history substitution
#	CMND~v			edit CMND using $VISUAL, where CMND may be found
#				using a history substitution
# "	^string1^string2^	substitute string2 for string1 in last command"
#				command and run it
# "	g^string1^string2^	globally substitute string2 for string1 in  "
#				last command and run it
# 	!NUMBER:s/string1/string2/
#				substitute string2 for string1 in
#				command NUMBER and run it
# 	!NUMBER:gs/string1/string2/
#				globally substitute string2 for string1 in
#				command NUMBER and run it
# 	!?STRING:s/string1/string2/
#				substitute string2 for string1 in last command
#				containing STRING and run it
# 	!?STRING:gs/string1/string2/
#				globally substitute string2 for string1 in last
#				command containing STRING and run it
#	
#	Any command which ends in the string ":p" is treated as a normal
#	command until all substitutions have been completed.  The trailing
#	":p" is then stripped, and the command is simply echoed and added to
#	the history list instead of being executed.
#
#	None of the other colon extensions of the cshell are supported.
#
#
#	Shell Environment Variables:
#	----------------------------
#
#	EDITOR		editor used by ~e command, default = "ed"
#	VISUAL		editor used by ~v command, default = "vi"
#	MAIL		your system mailbox
#	PAGER		paging program used by history command, default = "more"
#	PS1		primary prompt
#	PS2		secondary prompt
#	history		number of commands in history list, default = 22
#	histfile	file history list is saved in, default = $HOME/.bhistory
#	savehist	number of commands remembered from last bcsh session
#	aliasfile	file of aliased commands, default = $HOME/.baliases
#	logoutfile	file of commands to be executed before termination
#	inc_cmdno	yes/no -- keep track of command numbers or not
#	noclobber	if set, existing files are not overwritten by '>'
#	iclobber	if both noclobber and iclobber are set, the user is
#			prompted for confirmation before existing files are
#			overwritten by '>'
#
#	Note:	if you are setting either noclobber or iclobber mid-session,
#		set them to 'yes'
#
#
#	Regular Shell Variables:
#	------------------------
#
#	Shell variables may be set via Bourne or cshell syntax, e.g., both
#	"set foo=bar" and "foo=bar" set a variable called "foo" with the value
#	"bar".  However, all variables are automatically set as environment
#	variables, so there is no need to export them.  Conversely, there
#	are NO local variables.  Sorry, folks.
#
#	A cshell-style "setenv" command is turned into a regular "set" command.
#
#
#	The Prompt:
#	----------
#
#	You may, if you wish, have a command executed in your prompt.  If
#	the variable PS1 contains a dollar sign or a backquote, it is
#	evaluated and the result used as the prompt, provided the evaluation
#	did not produce a "not found" error message.  The two special cases
#	of PS1 consisting solely of "$" or "$ " are handled correctly.  For
#	example, to have the prompt contain the current directory followed
#	by a space, enter:
#
#		PS1=\'echo "`pwd` "\'
#
#	You need the backslashed single quotes to prevent the command being
#	evaluated by the variable-setting mechanism and the shell before it
#	is assigned to PS1.
#
#	To include the command number in your prompt, enter the command:
#
#		PS1=\'echo "$cmdno "\'
#
#
#	Shell Control-Flow Syntax:
#	--------------------------
#
#	'While', 'for', 'case', and 'if' commands entered in Bourne shell
#	syntax are executed as normal.
#
#	A valiant attempt is made to convert 'foreach' loops into 'for' loops,
#	cshell-syntax 'while' loops into Bourne shell syntax, and 'switch'
#	statements into 'case' statements.  I cannot guarantee to always get it
#	right.  If you forget the 'do' in a 'while' or 'for' loop, or finish
#	them with 'end' instead of 'done', this will be corrected.
#
#	Note that cshell-to-Bourne control flow conversions do not take place
#	if control is nested -- e.g., a 'foreach' inside a 'while' will fail.
#
#	The simple-case cshell "if (condition) command" is turned into Bourne
#	syntax.  Other 'if' statements are left alone apart from making the
#	'then' a separate statement, because constructing a valid interactive
#	cshell 'if' statement is essentially an exercise in frustration anyway.
#	The cshell and Bourne shell have sufficiently different ideas about
#	conditions that if is probably best to resign yourself to learning
#	the Bourne shell conventions.
#
#	Note that since most of the testing built-ins of the cshell are
#	not available in the Bourne shell, a complex condition in a 'while'
#	loop or an 'if' statement will probably fail.
#	
#
#	Bugs, Caveats, etc.:
#	--------------------
#
#	This is not a super-speedy program.  Be patient, especially on startup.
#
#	To the best of my knowledge this program should work on ANY Bourne
#	shell -- note that if your shell does not understand 'echo -n' you
#	will have to re-set the values of '$n' and '$c'.
#
#	This program may run out of stack space on a 16-bit machine where
#	/bin/sh is not split-space.
#
#	Mail checking is done every 10 commands if $MAIL is set in your
#	environment.  For anything fancier, you will have to hack the code.
#
#	Because commands are stuffed in a file before sh is invoked on them,
#	error messages from failed commands are ugly.
#
#	Failed history substitutions either give nothing at all, or a
#	"not found" style of error message.
#
#	A command history is kept whether you want it or not.  This may be
#	perceived as a bug or a feature, depending on which side of bed you
#	got out on.
#
#	If you want a real backslash in a command, you will have to type two
# 	of them  because the shell swallows the first backslash in the initial
# 	command pickup.  This means that to include a non-history '!' in a
#	command you need '\\!' -- a real wart, especially for net mail,
#	but unavoidable.
#
#	Commands containing an '@' will break all sorts of things.
#
#	Very complex history substitutions may fail.
#
#	File names containing numbers may break numeric history sustitutions.
#
#	Commands containing bizzare sequences of characters may conflict
#	with internal kludges.
#
#	Aliasing something to "commands;logout" will not work -- if you
#	want something to happen routinely on logout, put it in the file
#	specified by $logoutfile, default = $HOME/.blogout.
#
#	Please send all bug reports to ihnp4!utzoo!globetek!chris.
#	Flames will be posted to net.general with 'Reply-to' set to your
# '	path...  :-)							'
#
#
#
#		************* VERY IMPORTANT NOTICE *************
#
# If your shell supports # comments, then REPLACE all the colon 'comments'
# with # comments.  If it does not, then REMOVE all the 'comment' lines from the
# working copy of the file, as it will run MUCH faster -- the shell evaluates
# lines starting with a colon but does not actually execute them, so you will
# save the read-and-evaluate time by removing them.

case "`echo -n foo`" in
	-n*)
		n=
		c="\c"
		;;
	foo)
		n=-n
		c=
		;;
	*)
		echo "Your 'echo' command is broken."
		exit 1
		;;
esac
history=${history-22}
savehist=${savehist-22}
histfile=${histfile-$HOME/.bhistory}
logoutfile=${logoutfile-$HOME/.blogout}
EDITOR=${EDITOR-ed}
VISUAL=${VISUAL-vi}
PAGER=${PAGER-more}

aliasfile=${aliasfile-$HOME/.baliases}

# the alias file may contain 1 blank line, so a test -s will not work

case "`cat $aliasfile 2> /dev/null`" in
	"")
		doalias=no
		;;
	*)
		doalias=yes
		;;
esac

if test -s "${sourcefile-$HOME/.bcshrc}"
	then
	. ${sourcefile-$HOME/.bcshrc}
fi

if test -s "$histfile"
	then
	cmdno="`set - \`wc -l $histfile\`;echo $1`"
	cmdno="`expr \"$cmdno\" + 1`"
	lastcmd="`sed -n '$p' $histfile`"
	copy=false
	ohist=$histfile
	while test ! -w "$histfile"
		do
		echo "Cannot write to history file '$histfile'."
		echo $n "Please enter a new history filename: $c"
		read histfile
		copy=true
	done
	if $copy
		then
		cp $ohist $histfile
	fi
else
	cat /dev/null > $histfile
	cmdno=1
	lastcmd=
fi

# keep track of command number as the default

inc_cmdno=${inc_cmdo-yes}

# default prompts -- PS1 and PS2 may be SET but EMPTY, so '${PS1-% }' syntax
# is not used here

case "$PS1" in
	"")					
		PS1="% "
		;;				
esac
case "$PS2" in
	"")					
		PS2="> "
		;;				
esac

export histfile savehist history aliasfile EDITOR VISUAL PAGER cmdno PS1 PS2

case "$MAIL" in
	"")
		;;
	*)
		if [ -f $MAIL ]; then
			mailsize=`set - \`wc -c $MAIL\`;echo $1`
		else
			mailsize=0
		fi
		;;
esac

trap ':' 2
trap exit 3
trap "tail -n $savehist $histfile>/tmp/hist$$;uniq /tmp/hist$$ > $histfile;\
rm -f /tmp/*$$;exit 0" 15

getcmd=yes
mailcheck=
exclaim=
echoit=
mailprompt=

while :
do

	run=yes
	case "$mailprompt" in
		"")
			;;
		*)
			echo "$mailprompt"
			;;
	esac
	case "$getcmd" in
	yes)
		: guess if the prompt should be evaluated or not
		case "$PS1" in
		\$|\$\ )
			echo $n "$PS1$c"
				;;
		*\`*|*\$*)
			tmp="`(eval $PS1) 2>&1`"
			case "$tmp" in
			*not\ found)			
				echo $n "$PS1$c"
				;;			
			*)				
				echo $n "$tmp$c"
				;;			
			esac
			;;
		*)
			echo $n "$PS1$c"
			;;
		esac

		read cmd || cmd="exit"
		;;
	*)	;;
	esac

	case "$MAIL" in
	"")
		;;
	*)
		: check for mail every 10 commands
		case "$mailcheck" in
		1111111111)
			mailcheck=
			if [ -f $MAIL ]; then
				newsize="`set - \`wc -c $MAIL\`;echo $1`"
			else
				newsize=0
			fi
			if test "$newsize" -gt "$mailsize"; then
				mailprompt="You have new mail"
			else
				mailprompt=
			fi
			mailsize=$newsize
			;;
		*)
			mailcheck=1$mailcheck
			;;
		esac
		;;
	esac
	hist=no

	case "$cmd" in
	"")
		continue
		;;
	sh)
		sh
		run=no
		;;
	!!)
		cmd=$lastcmd
		echoit=yes
		getcmd=no
		continue
		;;
	*:p)
		cmd="`expr \"$cmd\" : '\(.*\):p'` +~+p"
		getcmd=no
		continue
		;;
	foreach[\ \	]*)
		while test "$line" != "end"; do
			echo $n "$PS2$c"
			read line
			cmd="${cmd};$line"
		done
		echo "$cmd" > /tmp/bcsh$$
		ed - /tmp/bcsh$$ << ++++
		s/end/done/
		s/foreach[ 	]\(.*\)(/for \1 in /
		s/)//
		s/;/;do /
		w
++++
		;;
	for[\ \	]*|while[\ \	]*)
		# try to catch the most common cshell-to-Bourne-shell
		# mistakes

		echo $n "$PS2$c"
		read line
		case "$line" in
		*do)
			line="do :"
			;;
		*do*)
			;;
		*)
			line="do $line"
			;;
		esac

		cmd="${cmd};$line"
		while test "$line" != "done"  && test "$line" != "end"
		do
			echo $n "$PS2$c"
			read line
			case "$line" in
			end)
				line=done
				;;
			esac
			cmd="${cmd};$line"
		done
		echo "$cmd" > /tmp/bcsh$$
		;;
	if[\ \	]*)
		while test "$line" != "fi" && test "$line" != "endif"
		do
			echo $n "$PS2$c"
			read line
			case "$line" in
			*[a-z]*then)
				line="`expr \"$line\" : '\(.*\)then'`;then"
				;;
			endif)
				line=fi
				;;
			esac
			cmd="${cmd};$line"
		done
		echo "$cmd" > /tmp/bcsh$$
		case "`grep then /tmp/bcsh$$`" in
		"")
			# fix 'if foo bar' cases

			ed - /tmp/bcsh$$ << ++++
			s/)/);then/
			s/.*/;fi/
			w
++++
			;;
		esac
		;;
	case[\ \	]*)
		while test "$line" != "esac"
		do
			echo $n "$PS2$c"
			read line
			cmd="${cmd}@$line"
		done
		cmd="`echo \"$cmd\" | tr '@' ' '`"
		echo "$cmd" > /tmp/bcsh$$
		;;
	switch[\ \	]*)
		while test "$line" != "endsw"
		do
			echo $n "$PS2$c"
			read line
			cmd="${cmd}@$line"
		done
		echo "$cmd" > /tmp/bcsh$$
		ed - /tmp/bcsh$$ << '++++'
		1,$s/@/\
/g
		g/switch.*(/s//case "/
		s/)/" in/
		1,$s/case[	 ]\(.*\):$/;;\
	\1)/
		2d
		1,$s/endsw/;;\
esac/
		g/breaksw/s///
		1,$s/default.*/;;\
	*)/
		w
++++
		cmd="`cat /tmp/bcsh$$`"
		;;
	*!*)
		hist=yes
		;;
	esac

	case "$hist" in
	yes)
		# deal with genuine exclamation marks, go back and parse again

		case "$cmd" in
		*\>![\ \	]*|*\\!*)
			cmd="`echo \"$cmd\" | sed -e 's@\\!@REALEXCLAMATIONMARK@g'`"
			exclaim=yes
			getcmd=no
			continue
			;;
		esac

		# break command into elements, parse each one

		tmp=
		for i in $cmd
		do
			# find element with !, peel off stuff up to !

			case "$i" in
			!)
				# most likely a typo for !!, so fix it
				front=
				$i=!!
				;;
			!!*)
				front=
				i="`expr \"$i\" : '.*\(!!.*\)'`"
				;;
			*!!*)
				front="`expr \"$i\" : '\(.*\)!!.*'`"
				i="`expr \"$i\" : '.*\(!!.*\)'`"
				;;
			!*)
				front=
				i="`expr \"$i\" : '.*!\(.*\)'`"
				;;
			*)
				tmp="$tmp$i "
				continue
				;;
			esac
			case "$i" in
			!!*)
				# want last command

				rest="`expr \"$i\" : '!!\(.*\)'`"
				i=$lastcmd
				;;
			-*)
				# we want to search back through the history list

				case "$i" in
				-)
					rest="`expr \"$i\" : '-\(.*\)'`"
					i=$lastcmd
					;;
				-[0-9]*)
					wanted="`expr \"$i\" : '-\([0-9][0-9]*\).*'`"
					rest="`expr \"$i\" : '-[0-9][0-9]*\(.*\)'`"
					i="`tail -n $wanted $histfile | sed -e "1q"`"
					;;
				esac
				;;
			[0-9]*)
				# find which number command is wanted

				wanted="`expr \"$i\" : '\([0-9][0-9]*\).*'`"
				rest="`expr \"$i\" : '[0-9][0-9]*\(.*\)'`"
				i="`grep -n . $histfile | grep \"^$wanted\"`"
				i="`expr \"$i\" : \"${wanted}.\(.*\)\"`"
				;;
			\?*)

				# find which 'command-contains' match is wanted

				case "$i" in
				\?{*}*)
					wanted="`expr \"$i\" : '?{\(.*\)}.*'`"
					rest="`expr \"$i\" : '?.*}\(.*\)'`"
					;;
				\?*:*)
					wanted="`expr \"$i\" : '?\(.*\):.*'`"
					rest="`expr \"$i\" : '?.*\(:.*\)'`"
					;;
				\?*)
					wanted="`expr \"$i\" : '?\(.*\)'`"
					rest=
					;;
				esac
				i="`grep \"$wanted\" $histfile | sed -n '$p'`"
				;;
			*)
				# find which 'start-of-command' match is wanted

				case "$i" in
				{*}*)
					wanted="`expr \"$i\" : '{\(.*\)}.*'`"
					rest="`expr \"$i\" : '.*}\(.*\)'`"
					;;
				*:*)
					wanted="`expr \"$i\" : '\(.*\):.*'`"
					rest="`expr \"$i\" : '.*\(:.*\)'`"
					;;
				*)
					wanted="$i"
					rest=
					;;
				esac
				i="`grep \"^$wanted\" $histfile | sed -n '$p'`"
				;;
			esac

			# see if we actually found anything to substitute

			case "$i" in
			"")
				badsub="Event not found"
				break
				;;
			*)
				badsub=no
				;;
			esac

			case "$rest" in
			"")
				tmp="$front$tmp$i "
				continue
				;;
			:[0-9]*)
				# find which element of $i is wanted

				number="`expr \"$rest\" : ':\([0-9][0-9]*\).*'`"
				rest="`expr \"$rest\" : ':[0-9][0-9]*\(.*\)'`"

				# count through $i till we get to the
				# right element

				counter=0
				for element in $i
				do
					case "$counter" in
					$number)
						break
						;;
					*)
						counter="`expr \"$counter\" + 1`"
						# counter=$[ $counter + 1 ]
						;;
					esac
				done
				case "$counter" in
				$number)
					badsub=no
					;;
				*)
					badsub="Bad command element"
					break
					;;
				esac
				tmp="$tmp$front$element$rest "
				continue
				;;
			:\$*)
				# spin through $i till we hit the last element

				rest="`expr \"$rest\" : ':\$\(.*\)'`"
				for element in $i
				do
					:
				done
				tmp="$tmp$front$element$rest "
				continue
				;;
			:\**)
				# we want all elements except the command itself

				rest="`expr \"$rest\" : ':\*\(.*\)'`"
				save=$i
				set - $i
				shift
				case "$*" in
				"")
					badsub="No arguments to command '$save'"
					break
					;;
				*)
					badsub=no
					;;
				esac
				tmp="$tmp$front$*$rest "
				continue
				;;
			:s*|:gs*)
				# we are doing a substitution
				# put / on end if needed

				case "$rest" in
				:s/*/*/*|:gs/*/*/*)
					;;
				:s/*/*|:gs/*/*)
					rest="${rest}/"
					;;
				esac

				# find what substitution is wanted

				first="`expr \"$rest\" : ':*s\/\(.*\)\/.*\/.*'`"
				second="`expr \"$i\" : ':*s/.*/\(.*\)/.*'`"

				# see if it is a global substitution

				case "$rest" in
				:gs*)
					global=g
					;;
				:s*)
					global=
					;;
				esac
				rest="`expr \"$rest\" : '.*/.*/.*/\(.*\)'`"
				i="`echo \"$i\" | sed -e \"s@$first@$second@$global\"`"

				# see if subsitution worked

				case "$i" in
				"")
					badsub="Substiution failed"
					break
					;;
				*)
					badsub=no
					;;
				esac
				tmp="$tmp$front$i$rest "
				continue
				;;
			*)
				tmp="$tmp$front$i$rest "
				;;
			esac
		done
		case "$badsub" in
		no)
			;;
		*)
			echo "$badsub"
			badsub=no
			continue
			;;
		esac
		cmd="$tmp"
		echoit=yes
		getcmd=no
		continue
		;;
	*)
		run=yes
		;;
	esac

	case "$cmd" in
	*\^*\^*\^*)
		# see if the substitution is global
		case "$cmd" in
		g*)
			global=g
			;;
		*)
			global=
			;;
		esac

		# put a '^' on the end if necessary
		case "$cmd" in
		*\^)
			;;
		*)
			cmd="${cmd}^"
			;;
		esac

		# find what substitution is wanted

		first="`expr \"$cmd\" : '*\^\(.*\)\^.*\^.*'`"
		second="`expr \"$cmd\" : '*\^.*\^\(.*\)\^.*'`"
		rest="`expr \"$cmd\" : '*\^.*\^.*\^\(.*\)'`"
		cmd="`echo \"$lastcmd\" | sed -e \"s@$first@$second@$global\"`$rest"

		# see if the substitution worked

		case "$cmd" in
		"")
			echo "Substitution failed"
			continue
			;;
		esac
		echoit=yes
		getcmd=no
		continue
		;;
	*~e)
		echo "$cmd" | sed -e "s@~e@@" > /tmp/bcsh$$
		$EDITOR /tmp/bcsh$$
		cmd="`cat /tmp/bcsh$$`"
		getcmd=no
		continue
		;;
	*~v)
		echo "$cmd" | sed -e "s@~v@@" > /tmp/bcsh$$
		echo "$lastcmd" > /tmp/bcsh$$
		$VISUAL /tmp/bcsh$$
		cmd="`cat /tmp/bcsh$$`"
		getcmd=no
		continue
		;;
	exec[\ \	]*)
		tail -n $savehist $histfile>/tmp/hist$$
		uniq /tmp/hist$$ > $histfile
		rm -f /tmp/*$$
		echo $cmd > /tmp/cmd$$
		. /tmp/cmd$$
		;;
	login[\ \	]*|newgrp[\ \	]*)
		tail -n $savehist $histfile>/tmp/hist$$
		uniq /tmp/hist$$ > $histfile
		rm -f /tmp/*$$
		echo $cmd > /tmp/cmd$$
		. /tmp/cmd$$
		;;
	logout|exit|bye)
		if test -s "$logoutfile"
			then
			# sh $logoutfile
			$SHELL $logoutfile
		fi
		tail -n $savehist $histfile > /tmp/hist$$
		uniq /tmp/hist$$ > $histfile
		rm -f /tmp/*$$
		exit 0
		;;
	h|history)
		grep -n . $histfile | tail -n $history | sed -e 's@:@	@' | $PAGER
		continue
		;;
	h[\ \	]\|*|h[\ \	]\>*|h\|*|h\>*)
		cmd="`echo \"$cmd\" | sed -e \"s@h@grep -n . $histfile | tail -n $history | sed -e 's@:@	@'@\"`"
		getcmd=no
		continue
		;;
	history[\ \	]*\|*|history[\ \	]*\>*)
		cmd="`echo \"$cmd\" | sed -e \"s@history@grep -n . $histfile | tail -n $history | sed -e 's@:@ @'@\"`"
		getcmd=no
		continue
		;;
	source[\ \	]*)
		set - $cmd
		shift
		echo . $*  > /tmp/cmd$$
		. /tmp/cmd$$
		run=no
		;;
	wait)
		wait
		run=no
		;;
	.[\ \	]*)
		echo $cmd > /tmp/cmd$$
		. /tmp/cmd$$
		run=no
		;;
	cd|cd[\ \	]*)
		# check if it will work first, or else this shell will terminate
		# if the cd dies.  If you have a built-in test, you might want
		# to replace the try-it-and-see below with a couple of tests,
		# but it is probably just as fast like this.

		echo $cmd > /tmp/cmd$$
		if ($SHELL /tmp/cmd$$) ; then
			. /tmp/cmd$$
		fi
		run=no
		;;
	awk[\ \	]*|dd[\ \	]*|cc[\ \	]*|make[\ \	]*)
		# these are the only commands I can think of whose syntax
		# includes an equals sign.  Add others as you find them.

		echo "$cmd" > /tmp/bcsh$$
		;;
	setenv*|*=*)
		# handle setting shell variables, turning cshell syntax to Bourne
		# syntax -- note all variables must be exported or they will not
		# be usable in other commands

		echo "$cmd" > /tmp/cmd$$
		ed - /tmp/cmd$$ << ++++
		g/^setenv[ 	]/s/[ 	]/@/
		g/^setenv@/s/[ 	]/=/
		g/^setenv@/s///
		g/^set/s///
		.t.
		\$s/=.*//
		s/^/export /
		w
++++
		. /tmp/cmd$$
		rm -f /tmp/cmd$$
		run=no
		;;
	unset[\ \	]*|umask[\ \	]*|export[\ \	]*|set[\ \	]*)
		# handle commands which twiddle current environment

		$cmd
		run=no
		;;
	alias|alias[\ \	])
		if [ -f $aliasfile ]; then
			$PAGER $aliasfile
		fi
		lastcmd=$cmd
		run=no
		continue
		;;
	alias[\ \	]*)
		case "$cmd" in
		alias[\ \	]\|*|alias[\ \	]\>*)
			cmd="`echo \"$cmd\" | sed -e \"s@alias@cat $aliasfile@\"`"
			getcmd=no
			continue
			;;
		alias[\ \	]*[\ \	]*)
			;;
		*)
			echo "Syntax: alias name command"
			cmd=
			continue
			;;
		esac
		set - $cmd
		shift
		cmd="$*"

		# make sure there is always 1 blank line in file so
		# unaliasing will always work -- ed normally refuses
		# to write an empty file
		echo "" >> $aliasfile
		cat << ++++ >> $aliasfile
$cmd
++++

#		ed - $aliasfile << '++++'
#		g/alias[ 	]/s///
#		g/^['"]\(.*\)['"]$/s//\1/
#		g/^/s//alias	/
#		w
#++++

		sort -u -o $aliasfile $aliasfile
		doalias=yes
		cmd="alias $cmd"
		run=no
		;;
	unalias[\ \	]*)
		set - $cmd
		case "$#" in
		2)
			cmd=$2
			;;
		*)
			echo "Syntax: unalias alias_name"
			continue
			;;
		esac
		ed - $aliasfile << ++++
		/^$cmd[ 	]/d
		w
++++
		case "`set - \`wc -l $aliasfile\`;echo $1`" in
		1)
			# just removed last alias
			doalias=no
			;;
		esac
		run=no
		;;
	*)
		case "$doalias" in
		yes)
			set - $cmd
			tmp="`grep \"^$1 \" $aliasfile`"
			case "$tmp" in
			$1[\ \	]*)
				shift
				cmd=$*
				set - $tmp
				shift
				tmp=$*
				case "$tmp" in
				*\$*)
					# uses positional variables

					cmd="set - $cmd ; $tmp"
					getcmd=no
					continue
					;;
				*)
					cmd="$tmp $cmd"
					getcmd=no
					continue
					;;
				esac
				;;
			*)
				echo "$cmd" > /tmp/bcsh$$
				;;
			esac
			;;
		no)
			echo "$cmd" > /tmp/bcsh$$
			;;
		esac
		;;
	esac

	case "$cmd" in
	*+~+p)
		cmd="`expr \"$cmd\" : '\(.*\)+~+p'`"
		echoit=yes
		run=no
		;;
	esac

	case "$cmd" in
	"")
		continue
		;;
	*)
		case "$exclaim" in
		yes)
			cmd="`echo \"$cmd\" | sed -e 's@REALEXCLAMATIONMARK@!@g'`"
			echo "$cmd" > /tmp/bcsh$$
			;;
		esac
		case "$echoit" in
		yes)
			echo $cmd
			;;
		esac
		case "$run" in
		yes)
			case "${noclobber+yes}" in
			yes)
				case "$cmd" in
				*\>![\ \	]*)
					ed - /tmp/bcsh$$ << ++++
					g/>!/s//>/
					w
++++
					;;
				*\>\>*)
					;;
				*\>*)
					outfile="`expr \"$cmd\" : '.*>\(.*\)'`"
					case "$outfile" in
					\&*)
						;;
					*)
						set - $outfile
						outfile="$1"
						if test -s "$outfile"
						then
							case "${iclobber+yes}" in
							yes)
								echo $n "Overwrite ${outfile}? $c"
								read answer
								case "$answer" in
								y*)
									;;
								*)
									echo ':' > /tmp/bcsh$$
									;;
								esac
								;;
							*)
								echo "${outfile}: file exists"
								echo ':' > /tmp/bcsh$$
								;;
							esac
						fi
						;;
					esac
					;;
				esac
				;;
			*)
				case "$cmd" in
				*\>![\ \	]*)
					ed - /tmp/bcsh$$ << ++++
					g/>!/s//>/g
					w
++++
					;;
				esac
				;;
			esac
			(trap 'exit 1' 2 3; $BASH /tmp/bcsh$$)
			;;
		esac
		case "$cmd" in
		$lastcmd)
			;;
		*)
			case "$exclaim" in
			yes)
				cmd="`echo \"$cmd\" | sed -e 's@!@\\\\!@g'`"
				;;
			esac

			cat << ++++ >> $histfile
$cmd
++++
			lastcmd=$cmd

			case "$inc_cmdno" in
			yes)
				cmdno="`expr \"$cmdno\" + 1`"
				# cmdno=$[$cmdno + 1]
				;;
			esac
			;;
		esac
		;;
	esac

	# The next commented-out line sets the prompt to include the command
	# number -- you should only un-comment this if it is the ONLY thing
	# you ever want as your prompt, because it will override attempts
	# to set PS1 from the command level.  If you want the command number
	# in your prompt without sacrificing the ability to change the prompt
	# later, replace the default setting for PS1 before the beginning of
	# the main loop with the following:  PS1='echo -n "${cmdno}% "'
	# Doing it this way is, however, slower than the simple version below.
	 
	PS1="${cmdno}% "

	getcmd=yes
	echoit=no
	exclaim=no
done
exit 0

# Christine Robertson  {linus, ihnp4, decvax}!utzoo!globetek!chris
