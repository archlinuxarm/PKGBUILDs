# xalias - convert csh alias commands to bash functions
# from Mohit Aron <aron@cs.rice.edu>
# posted to usenet as <4i5p17$bnu@larry.rice.edu>
function xalias ()
{
	if [ "x$2" = "x" ] 
	then
		declare -f $1
	else
		case $2 in
		*[#\!]*)
			comm=$(echo $2 | sed  's/\\!\*/\"$\@\"/g
					       s/\\!:\([1-9]\)/\"$\1\"/g
				               s/#/\\#/g')
			;;
		*)
			comm="$2 \"\$@\"" ;;
		esac

		eval function $1 \(\) "{" command "$comm"  "; }"
	fi
}
