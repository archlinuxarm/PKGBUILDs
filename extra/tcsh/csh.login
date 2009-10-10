#############################################################################
##
## Gentoo's csh.login
##
## 2003-01-13  -- Alain Penders (alain@gentoo.org)
##
##     Initial version.  Inspired by the Suse version.
##


##
## Default terminal initialization
##
if ( -o /dev/$tty && ${?prompt} ) then
    # Console
    if ( ! ${?TERM} )           setenv TERM linux
    if ( "$TERM" == "unknown" ) setenv TERM linux
    # No tset available on SlackWare
    if ( -x "`which stty`" ) stty sane cr0 pass8 dec
    if ( -x "`which tset`" ) tset -I -Q
    unsetenv TERMCAP
    settc km yes
endif

##
## Default UMASK
##
umask 022

##
## Set our SHELL variable.
##
setenv SHELL /bin/tcsh

##
## Setup a default MAIL variable
##
if ( -f /var/spool/mail/$USER ) then
    setenv MAIL /var/spool/mail/$USER
    set mail=$MAIL
endif

##
## If we're root, report who's logging in and out.
##
if ( "$uid" == "0" ) then
    set who=( "%n has %a %l from %M." )
    set watch=( any any )
endif

##
## Show the MOTD once the first time, and once after it has been changed.
##
## Note: if this is a SSH login, SSH will always show the MOTD, so we
## skip it.  Create ~/.hushlogin is you don't want SSH to show it.
##
if (-f /etc/motd ) then
    if ( ! $?SSH_CLIENT ) then
        cmp -s /etc/motd ~/.hushmotd
        if ($status) then
            tee ~/.hushmotd < /etc/motd
            echo "((( MOTD shown only once, unless it is changed )))"
        endif
    endif
endif

##
## Send us home.
##
cd

