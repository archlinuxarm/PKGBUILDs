#
# /etc/zsh/zprofile
#

export PATH="/bin:/usr/bin:/sbin:/usr/sbin:/opt/bin"

export MANPATH="/usr/man"
export LESSCHARSET="latin1"
export INPUTRC="/etc/inputrc"
export LESS="-R"

# Locale settings (find your locale with 'locale -a')
export LANG="en_US"
export LC_COLLATE="C"

export COLUMNS LINES

export PS1='[%n@%m %1~]$ '
export PS2='> '

umask 022

if [ "$TERM" = "xterm" -o "$TERM" = "xterm-color" -o "$TERM" = "rxvt" -o "$TERM" = "xterm-xfree86" ]; then
  PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME%%.*}:${PWD/$HOME/~}\007"'
fi

# load profiles from /etc/profile.d
#  (to disable a profile, just remove execute permission on it)
for profile in /etc/profile.d/*.sh; do
  if [ -x $profile ]; then
    . $profile
  fi
done
unset profile

# End of file
