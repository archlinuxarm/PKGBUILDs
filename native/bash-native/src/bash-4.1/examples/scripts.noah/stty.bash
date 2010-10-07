# stty.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-01-11
# Last modified: 1993-09-29
# Public domain

# Conversion to bash v2 syntax done by Chet Ramey

# Commentary:
# Code:

require remap_keybindings

#:docstring stty:
# Track changes to certain keybindings with stty, and make those changes
# reflect in bash's readline bindings as well. 
#
# This requires bash version 1.10 or newer, since previous versions did not
# implement the `bind' builtin.
#:end docstring:

###;;;autoload
function stty ()
{
    local erase="backward-delete-char"
    local kill="unix-line-discard"
    local werase="backward-kill-word"
    local lnext="quoted-insert"
    local readline_function=""
    local key=""
    local stty_command=""

    while [ $# -gt 0 ]; do
       case "$1" in
          erase | kill | werase | lnext )
             key=$(echo "${2}" | cat -v | sed 's/\^/\\C-/')
             readline_function=$(eval echo \$${1})

             # Get rid of any current bindings; the whole point of this
             # function is to make the distinction between readline
             # bindings and particular cbreak characters transparent; old
             # readline keybindings shouldn't hang around.
	     # could use bind -r here instead of binding to self-insert
             remap_keybindings "${readline_function}" "self-insert"
             
             # Bind new key to appropriate readline function
             bind "\"${key}\": ${readline_function}"

             stty_command="${stty_command} ${1} ${2}"
             shift 2
            ;;
          *)
             stty_command="${stty_command} ${1}"
             shift
            ;;
       esac
    done

    command stty ${stty_command}
}

provide stty

# stty.bash ends here
