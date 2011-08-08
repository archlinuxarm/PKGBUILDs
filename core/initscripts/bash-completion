# rc.d bash completion by Seblu <seblu@seblu.net>

_rc_d()
{
	local action cur prev
	action="help list start stop reload restart"
	_get_comp_words_by_ref cur prev
	if ((COMP_CWORD == 1)); then
		COMPREPLY=($(compgen -W "${action}" -- "$cur"))
	elif [[ "$prev" == help ]]; then
		COMPREPLY=()
	elif [[ "$prev" == list ]]; then
		((COMP_CWORD == 2)) && COMPREPLY=($(compgen -W "started stopped" -- "$cur")) || COMPREPLY=()
	elif [[ "$prev" == start ]]; then
		COMPREPLY=($(comm -23 <(cd /etc/rc.d && compgen -f -X 'functions*' "$cur"|sort) <(cd /run/daemons/ && compgen -f "$cur"|sort)))
	elif [[ "$prev" =~ stop|restart|reload ]]; then
		COMPREPLY=($(cd /run/daemons/ && compgen -f "$cur"|sort))
	elif ((COMP_CWORD > 1)); then
		COMPREPLY=($(cd /etc/rc.d && compgen -f -X 'functions*' "$cur"|sort))
	fi
}
complete -F _rc_d rc.d

# vim: set ts=2 sw=2 ft=sh noet:
