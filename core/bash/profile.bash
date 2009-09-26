#
# /etc/profile.bash
# Global settings for bash shells
#

PS1='[\u@\h \W]\$ '
PS2='> '
PS3='> '
PS4='+ '

export PS1 PS2 PS3 PS4

#In the future we may want to add more ulimit entries here,
# in the offchance that /etc/security/limits.conf is skipped
ulimit -Sc 0 #Don't create core files

if test "$TERM" = "xterm" -o \
        "$TERM" = "xterm-color" -o \
        "$TERM" = "xterm-256color" -o \
        "$TERM" = "rxvt" -o \
        "$TERM" = "rxvt-unicode" -o \
        "$TERM" = "xterm-xfree86"; then
    PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME%%.*}:${PWD/$HOME/~}\007"'
    export PROMPT_COMMAND
fi
