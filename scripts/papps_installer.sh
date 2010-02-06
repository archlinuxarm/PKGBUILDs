#! /bin/bash

#
# Papps_Installer for Plugapps
# Works on *all* Plug devices (Pogoplug v1/2; Dockstar; Tonidoplug)
# Version 0.5.1 # Halfie
#

###########################
## Function Declarations ##
###########################

function MD5 () {
	if [[ $(cat $2 | cut -d' ' -f1) != $(md5sum $1 | cut -d' ' -f1) ]]; then
			 "Bad md5 detected on $1"
			exit 1
	fi
}

function Download()
{
	 "Downloading files"
	
	local f=''
	
	for f in Modules.tar.gz Modules.tar.gz.md5 uImage uImage.md5; do
		if [[ ! -f sheeva-$KVer-$f ]]; then
			
			## Make sure wget is present ##
			if [[ -z "$(which wget)" ]]; then
				 "Something is horribly wrong & you're missing wget"
				## Add auto-fetch wget here ##
				exit 1
			fi
			
			## Grab the files ##
			wget -c http://sheeva.with-linux.com/sheeva/$KVer/sheeva-$KVer-$f
                        if [[ ! -f sheeva-$KVer-$f ]]; then
                                 "Unable to retrieve sheeva-$KVer-$f"
                                exit 1
                        fi
                fi
        done
        MD5 sheeva-$KVer-Modules.tar.gz sheeva-$KVer-Modules.tar.gz.md5
        MD5 sheeva-$KVer-uImage sheeva-$KVer-uImage.md5
}

## Introduction

function Intro () {
CAT <<INTRO
Welcome to the PlugApps for Pogoplug v1/v2/DockStar USB Installer.

>> To install to nand please use ./papps_installer --nand
>> To install to usb (pogoplug v2) please use ./papps_installer --usb
---
If you are lost, please ask for help or risk bricking your plug.
INTRO
}

#
# Making directories and cleaning out the USB drive
cd /
mount -o rw,remount /
killall hbwd
mkdir -p /new_root
mount /dev/sda1 /new_root
rm -rf /new_root/*
#
# Downloading and Extracting
#
cd /new_root/
 -e "\033[1mDownloading PlugBox Linux\033[0m"
wget http://plugapps.com/os/pogoplug/plugbox-pogoplug.tar.gz
 "Extracting PlugBox OS. This may take a few minutes."
tar -xzf plugbox-pogoplug.tar.gz
mv new_root/* ./
rmdir new_root
touch plugapps
touch root/.bash_profile
 "export TERM=linux" > root/.bash_profile
rm plugbox-pogoplug.tar.gz
#
# Modifying Boot Files
#
cd /etc/init.d
mv rcS rcS.backup
wget http://plugapps.com/os/pogoplug/v2/rcS
chmod 755 rcS
#
# Rebooting
#
 "Your Pogoplug Needs to be Restarted."
 "You will have to remove SSH keys on Linux/Mac OS X to log back in."
 "Type 'rm ~/.ssh/known_hosts"
 ""
 "Also, to boot back into the regular Pogoplug installation, just remove the drive you installed PlugApps on and restart your device."
 ""
 "You'll notice PlugApps are ready when you login and see a root@PogoPlug prompt."
 "Press ENTER to reboot."
read enter
/sbin/reboot


if [[ $# < 1 ]]; then
	Intro
	exit 1
fi

if [[ "$1" == "--nand" ]]; then
	Download
	Nand
	
	
elif [[ "$1" == "--usb" ]]; then
	Download
	Usb
	
else
	Usage
	exit 1
fi
