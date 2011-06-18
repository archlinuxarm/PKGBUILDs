# rc.d bash completion by Seblu <seblu@seblu.net>

_rc.d ()
{
	local action="help list start stop reload restart"
	local cur="${COMP_WORDS[COMP_CWORD]}"
	local caction="${COMP_WORDS[1]}"
	if ((${COMP_CWORD} == 1)); then
		COMPREPLY=($(compgen -W "${action}" -- "$cur"))
	elif [[ "$caction" =~ help|list ]]; then
		COMPREPLY=()
	elif [[ "$caction" == start ]]; then
		COMPREPLY=($(comm -23 <(cd /etc/rc.d && compgen -f -X 'functions*' "$cur"|sort) <(cd /run/daemons/ && compgen -f "$cur"|sort)))
	elif [[ "$caction" =~ stop|restart|reload ]]; then
		COMPREPLY=($(cd /run/daemons/ && compgen -f "$cur"|sort))
	elif ((${COMP_CWORD} > 1)); then
		COMPREPLY=($(cd /etc/rc.d && compgen -f -X 'functions*' "$cur"|sort))
	fi
}
complete -F _rc.d rc.d

# vim: set ts=2 sw=2 ft=sh noet:
