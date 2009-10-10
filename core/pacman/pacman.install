#!/bin/sh
# arg 1:  the new package version
# arg 2:  the old package version
post_upgrade() {
	# one time stuff for md5sum issue with older pacman versions
	if [ "$(vercmp $2 3.0.2)" -lt 0 ]; then
		_resetbackups
	fi
}

_resetbackups() {
	echo ">>> Performing one-time reset of NoUpgrade md5sums. After this reset"
	echo ">>> you are able to remove all NoUpgrade lines of already protected"
	echo ">>> files from pacman.conf."
	echo ">>>"

	# path variables
    pacconf="/etc/pacman.conf"
    dbpath="/var/lib/pacman/local"

    # get a list of NoUpgrade files from the user's pacman.conf
    echo ">>> Retrieving pacman.conf NoUpgrade list..."
    config=$(grep "^NoUpgrade" $pacconf | cut -d'=' -f2)
    # add the standard list of files, even if they are already above
    config="$config \
    etc/passwd etc/group etc/shadow etc/sudoers \
    etc/fstab etc/raidtab etc/ld.so.conf \
    etc/rc.conf etc/rc.local \
    etc/modprobe.conf etc/modules.conf \
    etc/lilo.conf boot/grub/menu.lst"

    # blank md5sum for use in sed expression
    zeroes='00000000000000000000000000000000'

    for file in $config; do
        echo ">>> -> finding owner of /$file..."
        line=$(LC_ALL=C LANG=C pacman -Qo /$file 2>/dev/null)
        # if file is owned by a package, go find its incorrectly stored sum
        if [ ! -z "$line" ]; then
            # get the name and version of the package owning file
            name=$(echo $line | awk '{print $5}')
            version=$(echo $line | awk '{print $6}')
            # set the path to the backup array holding the md5sum
            path="$dbpath/$name-$version/files"
            # run a sed on the path to reset the line containing $file
            # NOTE: literal tab characters in sed expression after $file
            echo ">>> -> resetting sum of /$file..."
            sed -i "s#$file [0-9a-fA-F]*#$file  $zeroes#" $path
        else
            echo ">>> -> $file is unowned."
        fi
    done
}
