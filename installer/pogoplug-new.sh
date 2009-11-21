#!/bin/bash

# NOTE!!!! THIS DOESN'T WORK, DO NOT USE IT

echo "Make sure you run"
echo "mount -o rw,remount /"
echo "before running this script"
read enter
cd /
wget http://openpogo.com/pacman/installer/archplug.tar.gz
cp -a /usr/lib/* /lib/
mkdir /usr-tmp
mount /dev/mtdblock3 /usr-tmp
# Of course, this mounting and removing should only be on Pogoplug
rm -rf /usr-tmp/*
cp -a /usr/* /usr-tmp/
rm -rf /usr
mkdir /usr
umount /usr-tmp
rm -rf /usr-tmp
mount /dev/mtdblock3 /usr
tar xzf files.tar.gz
source /root/.bash_profile
rm archplug.tar.gz
chmod +x /usr/bin/*
pacman -Sy
pacman -Sf glibc busybox libarchive libdownload pacman openssl
echo "Run 'source /root/.bash_profile' next!!!"
echo "Run 'pacman -Sy' to refresh the repositories."
echo "Afterwards, use 'pacman -S ****' to install packages"
echo "and 'pacman -Rs ****' to uninstall packages."
echo "See http://openpogo.com for more details."

### TODO!!!
## Change stuff in archplug.tar.gz:
# New pacman pkg binaries (/usr/bin/*) from native repo
