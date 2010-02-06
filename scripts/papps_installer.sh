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
			 echo "Bad md5 detected on $1"
			exit 1
	fi
}

function Download () {
	cd /new_root/
	
	if [[ ! -f plugbox-pogoplug.tar.gz ]]; then
	
		## Make sure wget is present ##
		if [[ -z "$(which wget)" ]]; then
		 	echo "Something is horribly wrong & you're missing wget"
			## Add auto-fetch wget here (use curl or something)##
		exit 1
		fi
	
		if [[ "$FlashType" == "nand" ]]; then
			wget -c http://plugapps.com/os/pogoplug/plugbox-pogoplug.tar.gz
			wget -c http://plugapps.com/os/pogoplug/plugbox-pogoplug.tar.gz.MD5
				if [[ ! -f plugbox-pogoplug.tar.gz ]]; then
					echo "Unable to download flash image:("
					exit 1
				fi
		elif [[ "$FlashType" == "usb" ]]; then
			wget -c http://plugapps.com/os/pogoplug/plugbox-pogoplug.tar.gz
			wget -c http://plugapps.com/os/pogoplug/plugbox-pogoplug.tar.gz.MD5
				if [[ ! -f plugbox-pogoplug.tar.gz ]]; then
					echo "Unable to download flash image :("
					exit 1
				fi
		else
			echo "For some reason you didn't get any of the files :("
		fi
	fi
	MD5 plugbox-pogoplug.tar.gz plugbox-pogoplug.tar.gz.MD5
}


function notDownload () {
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
    MD5 
}

## Introduction

function Intro () {
	cat <<INTRO
	
<< Welcome to the PlugApps for Pogoplug v1/v2/DockStar USB Installer. >>

>> To install to nand please use ./papps_installer --nand
>> To install to usb (pogoplug v2) please use ./papps_installer --usb
---
If you are lost, please ask for help, or risk bricking your plug.
INTRO
}

#function ListDisks () {}

#function Format () {}

function Nand () {
	local FlashType=nand
	
	cat <<NAND
	
For Advanced Users Only...
>>> Tonido/Sheevaplug users press enter to continue
>>> Pogoplug users please use the --usb installer (or continue at your own risk)
NAND
read enter

# Let's give one final warning
Warning

# Lets mount everything up & prepare
cd /
echo '---'; sleep 1 ; echo 'Mounting /dev/mtdblock3'
#mount -o rw,remount /
#mkdir -p /new_root
#umount /dev/mtdblock3
#mount /dev/mtdblock3 /new_root

# Just clean everything out
echo '---'; sleep 1; echo 'Cleaning /dev/mtdblock3'
#rm -rf /new_root/*

# Download the flash image & MD5 check
echo '---'; sleep 1; echo 'Downloading/Checksumming Flash Image'
Download
}

function Warning () {
	cat <<WARNING
If you have important data in /opt (aka /dev/mtdblock3), please backup before running this utility.
...
Are you sure you want to run a nand flash? [Y/n]
WARNING

read item
	case "$item" in
		y*|Y*) ;;
		n|N) echo "Fine, don't continue!"; exit 1;;
		*) echo "Fail, please answer Y/n"; exit 1;;
	esac
}

function Usb () {
	# Making directories and cleaning out the USB drive
	cd /
	mount -o rw,remount /
	killall hbwd
	mkdir -p /new_root
	mount /dev/sda1 /new_root
	rm -rf /new_root/*
	#Extract
}

function Extract () {
	tar -xzf plugbox-pogoplug.tar.gz
	## place some variable for *.tar.gz & call in appropriate function (usb or nand)
}

###############################
## END Function Declarations ##
###############################

#
# Downloading and Extracting
#

#wget http://plugapps.com/os/pogoplug/plugbox-pogoplug.tar.gz
# "Extracting PlugBox OS. This may take a few minutes."
#
#mv new_root/* ./
#rmdir new_root
#touch plugapps
#touch root/.bash_profile
# "export TERM=linux" > root/.bash_profile
#rm plugbox-pogoplug.tar.gz


#
# Modifying Boot Files
#
#cd /etc/init.d
#mv rcS rcS.backup
#wget http://plugapps.com/os/pogoplug/v2/rcS
#chmod 755 rcS


#
# Rebooting
#
# "Your Pogoplug Needs to be Restarted."
# "You will have to remove SSH keys on Linux/Mac OS X to log back in."
# "Type 'rm ~/.ssh/known_hosts"
# ""
# "Also, to boot back into the regular Pogoplug installation, just remove the drive you installed PlugApps on and restart your device."
# ""
# "You'll notice PlugApps are ready when you login and see a root@PogoPlug prompt."
# "Press ENTER to reboot."
#read enter
#/sbin/reboot


if [[ $# < 1 ]]; then
	Intro
	
	exit 1
fi

if [[ "$1" == "--nand" ]]; then
	Nand
	
elif [[ "$1" == "--usb" ]]; then
	Usb
	
else
	Usage
	exit 1
fi
