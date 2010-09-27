# prompt.bash
# Author: Noah Friedman <friedman@prep.ai.mit.edu>
# Created: 1992-01-15
# Public domain

# $Id: prompt.bash,v 1.2 1994/10/18 16:34:35 friedman Exp $

# Commentary:
# Code:

#:docstring prompt:
# Usage: prompt [chars]
#
# Various preformatted prompt strings selected by argument.  For a
# list of available arguments and corresponding formats, do 
# `type prompt'. 
#:end docstring:

###;;;autoload
function prompt ()
{
    case "$1" in
    d)     PS1='$(dirs) \$ '               ;;
    n)     PS1='\$ '                       ;;
    hsw)   PS1='\h[$SHLVL]: \w \$ '        ;;
    hw)    PS1='\h: \w \$ '                ;;
    sh)    PS1='[$SHLVL] \h\$ '            ;;
    sw)    PS1='[$SHLVL] \w \$ '           ;;
    uh)    PS1='\u@\h\$ '                  ;;
    uhsHw) PS1='\u@\h[$SHLVL]:\#: \w \$ '  ;;
    uhsw)  PS1='\u@\h[$SHLVL]: \w \$ '     ;;
    uhw)   PS1='\u@\h: \w \$ '             ;;
    uw)    PS1='(\u) \w \$ '               ;;
    w)     PS1='\w \$ '                    ;;
    esac
}

provide prompt

# prompt.bash ends here
