# 
# /etc/profile
#
# This file is intended to be used for ALL common
# Bourne-compatible shells. Shell specifics should be
# handled in /etc/profile.$SHELL where $SHELL is the name
# of the binary being run (discounting symlinks)
#
# Sections taken from SuSe's /etc/profile
# Note the explicit use of 'test' to cover all bases
#  and potentially incompatible shells

#Determine our shell without using $SHELL, which may lie
shell="sh"
if test -f /proc/mounts; then
   case $(/bin/ls -l /proc/$$/exe) in
        *bash) shell=bash ;;
        *dash) shell=dash ;;
        *ash)  shell=ash ;;
        *ksh)  shell=ksh ;;
        *zsh)  shell=zsh ;;
    esac
fi

# Load shell specific profile settings
test -f "/etc/profile.$shell" &&  . "/etc/profile.$shell"

#Set our umask
umask 022

# Set our default path
PATH="/bin:/usr/bin:/sbin:/usr/sbin"
export PATH

# Export default pkg-config path
PKG_CONFIG_PATH="/usr/lib/pkgconfig"
export PKG_CONFIG_PATH

# Some readline stuff that is fairly common
HISTSIZE=1000
HISTCONTROL="erasedups"

INPUTRC="/etc/inputrc"
LESS="-R"
LC_COLLATE="C"

export HISTSIZE HISTCONTROL INPUTRC LESS LC_COLLATE

# Load profiles from /etc/profile.d
if test -d /etc/profile.d/; then
    for profile in /etc/profile.d/*.sh; do
        test -x $profile && . $profile
    done
    unset profile
fi

# Termcap is outdated, old, and crusty, kill it.
unset TERMCAP

# Man is much better than us at figuring this out
unset MANPATH
