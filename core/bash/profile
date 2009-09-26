#
# /etc/profile
#

export PATH="/bin:/usr/bin:/sbin:/usr/sbin:/usr/X11R6/bin:/opt/bin"

export MANPATH="/usr/man:/usr/X11R6/man"
export LESSCHARSET="latin1"
export INPUTRC="/etc/inputrc"
export LESS="-R"

export LC_COLLATE="C"

export COLUMNS LINES

export PS1='[\u@\h \W]\$ '
export PS2='> '

umask 022

if [ "$TERM" = "xterm" -o "$TERM" = "xterm-color" -o "$TERM" = "rxvt" -o "$TERM" = "xterm-xfree86" ]; then
  PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME%%.*}:${PWD/$HOME/~}\007"'
fi

# load profiles from /etc/profile.d
#  (to disable a profile, just remove execute permission on it)
if [ `ls -A1 /etc/profile.d/ | wc -l` -gt 0 ]; then
  for profile in /etc/profile.d/*.sh; do
    if [ -x $profile ]; then
      . $profile
    fi
  done
  unset profile
fi
# End of file
