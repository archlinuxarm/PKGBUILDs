#!/bin/sh
#
# handy functions:
#
# a function to display a failure message and then exit 
fail ( ) {
	echo -e $@
	exit 1
}

# "get with default" function
# this function prompts the user with a query and default reply
# it returns the user reply
getwd ( ) {
	query="$1"
	default="$2"
	echo -en "$query [$default]" | cat >&2
	read response
	if [ x$response = "x" ]; then
		response=$default
	fi
	echo $response
}

# "get yes no" function
# this function prompts the user with a query and will continue to do so
# until they reply with either "y" or "n"
getyn ( ) {
	query="$@"
	echo -en $query | cat >&2
	read response
	while [ x$response != "xy" -a x$response != "xn" ]; do
		echo -e "\n'y' or 'n' only please...\n" | cat >&2
		echo -en $query | cat >&2
		read response
	done	
	echo $response
}

# configuration 
#
# set defaults
defaultusername="scponly"
defaulthomedirprefix="/home"
defaultwriteabledir="incoming"

osname=`uname -s | tr ' ' '_'`
# pathname to platform/OS specific setup scripts
prescript="build_extras/arch/$osname.pre.sh"
postscript="build_extras/arch/$osname.post.sh"

# the following is a list of binaries that will be staged in the target dir
BINARIES=`grep '#define PROG_' config.h | cut -f2 -d\" | grep -v ^cd$`

# we set the install path in a variable so the presetup script can overwrite it on systems
# which require it
INSTALL_PATHNAME="install -c"

# attempt a best guess at required libs, we can append things in the presetup script if we need to
LDSOFOUND=0

# default to useradd, not pw
USE_PW=0

if [ x/usr/bin/ldd = x ]; then
	echo "this script requires the program ldd to determine which"
	fail "shared libraries to copy into your chrooted dir..."
fi

if [ x`uname -s` = "xOpenBSD" ]; then
	for bin in $BINARIES; do
		GREP_LIST="$GREP_LIST -e $bin"
	done
	LIB_LIST=`ldd $BINARIES 2> /dev/null | /usr/bin/tr -s " " | cut -f5 -d" " | /usrgrep -v "^Name" | /usrgrep -v $GREP_LIST | /usr/bin/sort -u`
else
	LIB_LIST=`ldd $BINARIES 2> /dev/null | cut -f2 -d\> | cut -f1 -d\( | grep "^ " | sort -u`
fi

#
#	we also need to add some form of ld.so, here are some good guesses.
#
LDSO_LIST="/lib/ld.so /libexec/ld-elf.so /libexec/ld-elf.so.1 /usr/libexec/ld.so /lib64/ld-linux-x86-64.so.2 /lib/ld-linux.so.2 /usr/libexec/ld-elf.so.1"
for lib in $LDSO_LIST; do
	if [ -f $lib ]; then
		LDSOFOUND=1;
		LIB_LIST="$LIB_LIST $lib"
	fi
done

#
#	TODO - i've since forgotten which OS this is for, it should be relocated to a presetup script
#
ls /lib/libnss_compat* > /dev/null 2>&1 
if [ $? -eq 0 ]; then
	LIB_LIST="$LIB_LIST /lib/libnss_compat*"
fi

# check that the configure options are correct for chrooted operation:

if [ x/usr/sbin/useradd = x ]; then
    if [ x = x ]; then
		echo "this script requires the program useradd or pw to add your"
		fail "chrooted scponly user."
	else
  		USE_PW=1;
    fi
fi

# we need to be root
if [ `id -u` != "0" ]; then
	fail "you must be root to run this script\n"
fi

echo
echo Next we need to set the home directory for this scponly user.
echo please note that the user\'s home directory MUST NOT be writeable
echo by the scponly user.  this is important so that the scponly user
echo cannot subvert the .ssh configuration parameters.
echo
echo for this reason, a writeable subdirectory will be created that
echo the scponly user can write into.  
echo

if [ "$2" != "" ] ; then
	targetuser=$2
else
targetuser=`getwd "Username to install" "$defaultusername"`
fi
username_collision=`id $targetuser > /dev/null 2> /dev/null; echo $?`
if [ $username_collision -eq 0 ] ; then
	fail "the user $targetuser already exists.  please remove this user and their home directory and try again."
fi 

if [ "$1" != "" ] ; then
	targetdir=$1
else
targetdir=`getwd "home directory you wish to set for this user" "$defaulthomedirprefix/$targetuser"`
fi

if [ "$3" != "" ] ; then
	writeabledir=$3
else
writeabledir=`getwd "name of the writeable subdirectory" "$defaultwriteabledir"`
fi

#
#	if you would like to overwrite/extend any of the variables above, do so in the system specific
#	presetup script.  
#
if [ -f "$prescript" ]; then
#
#	this system has a pre-chroot setup script, lets run it
#
	. "$prescript"
fi

# if neither the presetup script or the best guess could find ld.so, we have to bail here
if [ $LDSOFOUND -eq 0 ]; then
	fail i cant find your equivalent of ld.so
fi

#
#	ACTUAL MODIFICATIONS BEGIN HERE
#

# this part shouldnt strictly be requried, but i'll leave it in until i'm sure of it
if [ ! -d $targetdir ]; then
	$INSTALL_PATHNAME -d $targetdir
	chmod 755 $targetdir
fi

if [ ! -d $targetdir/etc ]; then
	$INSTALL_PATHNAME -d $targetdir/etc
	chown 0:0 $targetdir/etc
	chmod 755 $targetdir/etc
fi

# add all our binaries
for bin in $BINARIES; do
	$INSTALL_PATHNAME -d $targetdir/`/usr/bin/dirname $bin`
	$INSTALL_PATHNAME $bin $targetdir$bin
done

# and the libs they require
if [ "x$LIB_LIST" != "x" ]; then
	for lib in $LIB_LIST; do
		$INSTALL_PATHNAME -d $targetdir/`/usr/bin/dirname $lib`
		$INSTALL_PATHNAME $lib $targetdir/$lib
	done
fi

# /dev/null is needed inside the chroot
mkdir -p $targetdir/dev
mknod -m 666 $targetdir/dev/null c 1 3

if [ "x$USE_PW" = x0 ] ; then
    /usr/sbin/useradd -d "$targetdir" -s "/usr/sbin/scponlyc" $targetuser
    if [ $? -ne 0 ]; then
         fail "if this user exists, remove it and try again"
    fi
else
     useradd -n $targetuser -s "/usr/sbin/scponlyc" -d "$targetdir"
    if [ $? -ne 0 ]; then
         fail "if this user exists, remove it and try again"
    fi
fi 

#
#	we must ensure certain directories are root owned.
#
chown 0:0 $targetdir 
if [ -d $targetdir/.ssh ]; then
	chown 0:0 $targetdir/.ssh
fi

if [ ! -d $targetdir/$writeabledir ]; then
	echo -e "\ncreating  $targetdir/$writeabledir directory for uploading files"
	$INSTALL_PATHNAME -o $targetuser -d $targetdir/$writeabledir
fi

#
#	set the perms on the writeable dir so that the new user owns it
#
newuid=`id -u $targetuser`
newgid=`id -g $targetuser`
chown $newuid:$newgid $targetdir/$writeabledir

if [ -f "$postscript" ]; then
#
#   this system has a post-chroot setup script, lets run it
#
    . "$postscript"
else
#
#	otherwise, revert to the old "best guess" system, which sucks
#
	echo
	echo "Your platform ($osname) does not have a platform specific setup script."
	echo "This install script will attempt a best guess."
	echo "If you perform customizations, please consider sending me your changes."
	echo "Look to the templates in build_extras/arch."
	echo " - joe at sublimation dot org"
	echo
	if [ x = x ]; then
	#
	#	ok we dont have pwd_mkdb, lets improvise:
	#
		grep $targetuser /etc/passwd > $targetdir/etc/passwd
		# Debian: copy /etc/group into the jail, for /usr/bin/groups to work
		cp /etc/group $targetdir/etc/group

	else
	#
	#	this is for systems which do have pwd_mkdb
	#
		grep $targetuser /etc/master.passwd > $targetdir/etc/master.passwd
		 -d "$targetdir/etc" $targetdir/etc/master.passwd
		rm -rf $targetdir/etc/master.passwd $targetdir/etc/spwd.db
	fi
fi

#
#   the final step is setting the password
#
echo "please set the password for $targetuser:"
passwd $targetuser

echo "if you experience a warning with winscp regarding groups, please install"
echo "the provided hacked out fake groups program into your chroot, like so:"
echo "cp groups $targetdir/bin/groups" 
