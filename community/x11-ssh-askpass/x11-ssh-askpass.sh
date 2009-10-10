#
# /etc/profile.d/ssh-x11-askpass.sh
#
# Maintained by Charles Mauch <cmauch@gmail.com>

if [ -f "/usr/lib/openssh/x11-ssh-askpass" ] ; then
  SSH_ASKPASS="/usr/lib/openssh/x11-ssh-askpass"
  export SSH_ASKPASS
fi

# /etc/profile.d/ssh-x11-askpass.sh ends here.
