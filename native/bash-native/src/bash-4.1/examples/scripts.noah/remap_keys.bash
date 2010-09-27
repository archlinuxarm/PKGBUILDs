# remap_keybindings.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-01-11
# Last modified: 1993-02-03
# Public domain

# Conversion to bash v2 syntax done by Chet Ramey

# Commentary:
# Code:

#:docstring remap_keybindings:
# Usage: remap_keybindings old_function new_function
#
# Clear all readline keybindings associated with OLD_FUNCTION (a Readline
# function) rebinding them to NEW_FUNCTION (`self-insert' by default)
#
# This requires bash version 1.10 or newer, since previous versions did not
# implement the `bind' builtin.
#:end docstring:

###;;;autoload
function remap_keybindings ()
{
    local unbind_function="$1"
    local bind_function="${2:-'self-insert'}"
    local bind_output
    local arg

    # If they're the same thing, the work has already been done.  :-)
    if [ "${unbind_function}" = "${bind_function}" ]; then
       return 0
    fi

    while : ; do
       bind_output="$(bind -q ${unbind_function} 2> /dev/null)"

       case "${bind_output}" in
          "${unbind_function} can be invoked via"* ) ;;
          "" ) return 1 ;;         # probably bad argument to bind
          *) return 0 ;;           # unbound
       esac

       # Format of bind_output is like:
       # 'quoted-insert can be invoked via "\C-q", "\C-v".'
       # 'self-insert can be invoked via " ", "!", """, "$", "%", ...'
       set -- ${bind_output}
       shift 5
       
       for arg in "$@" ; do
          # strip off trailing `.' or `,'
          arg=${arg%.};
          arg=${arg%,};
       
          case ${arg} in
             ..)
                # bind -q didn't provide whole list of key bindings; jump
                # to top loop to get more
                continue 2 ; 
               ;;
             *)
                bind "${arg}: ${bind_function}"
               ;;
          esac
       done
    done
}

provide remap_keybindings

# remap_keybindings.bash ends here
