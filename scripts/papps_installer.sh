#! /bin/bash

#
# Papps_Installer for Plugapps
# Works on *all* Plug devices (Pogoplug v1/2; Dockstar; Tonidoplug)
# Version 0.5.1 # Halfie (becomes 1.0 upon testing)
#

###########################
## Function Declarations ##
###########################

# Introduction
function Intro () {
	cat <<INTRO
	
<< Welcome to the PlugApps for Pogoplug v1/v2/DockStar USB Installer. >>

>> (Preferred) To install to usb (pogoplug v2) please use ./papps_installer --usb
>> (Advanced) To install to nand please use ./papps_installer --nand
---
If you are lost, please ask for help, or risk bricking your plug.
INTRO
}

## Nand Install
function Nand () {
	local FlashType=nand
	
	cat <<NAND
	
For Advanced Users Only...
>>> Tonido/Sheevaplug users press enter to continue
>>> Pogoplug users please use the --usb installer (or continue at your own risk)

Press ENTER to continue...
NAND
	read enter

# Let's give one final warning
	WarningNand

# Lets mount everything up & prepare
	cd /
	echo '---'; sleep 1 ; echo 'Mounting /dev/mtdblock3'
	mount -o rw,remount /
	mkdir -p /new_root
	umount /dev/mtdblock3
	mount /dev/mtdblock3 /new_root

# Clean everything out
	echo '---'; sleep 1; echo 'Cleaning /dev/mtdblock3'
	rm -rf /new_root/*

# Download the flash image & MD5 check
	echo '---'; sleep 1; echo 'Downloading/Checksumming Flash Image'
	Download
	echo '---'; sleep 1; echo 'Extracting Image'
	Extract
	echo '---'; sleep 1; echo 'Installing image'
	Install
	echo '---'; sleep 1; echo 'Modifying Boot Script (/etc/init.d/rcS)'
	ModifyrcS
	Reboot
}

## USB Install
function Usb () {
	local FlashType=usb
	
	cat <<USB

Welcome to the USB installer (Preferred method of installation)...
>>> Please start by removing all of your usb devices, and set aside one erasable disk

Press ENTER to continue...
USB
	
# Lets give a final Warning
	WarningUsb

# Find the correct disk to format
echo '---'; sleep 1 ; echo 'Listing Disks...'
	ListDisk

# Decide & format disk
	Format

# Lets mount everything up & prepare
	cd /
	echo '---'; sleep 1 ; echo "Mounting /dev/$d"
	mount -o rw,remount /

# Shutdown hbmgr
	killall hbwd
	mkdir -p /new_root
	mount /dev/$d /new_root

# Clean everything out
echo '---'; sleep 1; echo "Cleaning /dev/$d"
rm -rf /new_root/*

# Download the flash image & MD5 check
echo '---'; sleep 1; echo 'Downloading/Checksumming Flash Image'
Download
echo '---'; sleep 1; echo 'Extracting Image'
Extract
echo '---'; sleep 1; echo 'Installing image'
Install
echo '---'; sleep 1; echo 'Modifying Boot Script (/etc/init.d/rcS)'
ModifyrcS
Reboot
}

function WarningNand () {
	cat <<WNAND
If you have important data in /opt (aka /dev/mtdblock3), please backup before running this utility.
...
Are you sure you want to run a nand flash? [Y/n]
WNAND

function WarningUsb () {
	cat <<WUSB
If you have important data on your disk, please backup now. ALL DATA WILL BE ERASED!
...
Are you sure you want to run a usb install? [Y/n]
WUSB

read item
	case "$item" in
		y*|Y*) ;;
		n|N) echo "Fine, don't continue!"; exit 1;;
		*) echo "Fail, please answer Y/n"; exit 1;;
	esac
}

# List availiable disks (just in case someone didn't read the directions & unplug)
function ListDisk () {
	# List disks to format
	/sbin/fdisk -l | grep 'Disk /dev/sd' | awk '{print NR " "$2 " " $3 " MB" }'
}

function Format () {
	local d=''
	
	cat <<FORMAT
If you saw more than one disk please select the correct one here:

Type the number of the disk (EG. '1 /dev/sda: 8086 MB' ; you would type '1'):
FORMAT
	read item
		case "$item" in
			1) dd if=/dev/zero of=/dev/sda bs=512 count=1; /sbin/fdisk /dev/sda <<< $'n\np\n1\n\n\nw\n'; local d=sda ;;
			2) dd if=/dev/zero of=/dev/sdb bs=512 count=1; /sbin/fdisk /dev/sdb <<< $'n\np\n1\n\n\nw\n'; local d=sdb ;;
			3) dd if=/dev/zero of=/dev/sdc bs=512 count=1; /sbin/fdisk /dev/sdc <<< $'n\np\n1\n\n\nw\n'; local d=sdc ;;
			4) dd if=/dev/zero of=/dev/sdd bs=512 count=1; /sbin/fdisk /dev/sdd <<< $'n\np\n1\n\n\nw\n'; local d=sdd ;;
			5) dd if=/dev/zero of=/dev/sde bs=512 count=1; /sbin/fdisk /dev/sde <<< $'n\np\n1\n\n\nw\n'; local d=sde ;;
			6) dd if=/dev/zero of=/dev/sdf bs=512 count=1; /sbin/fdisk /dev/sdf <<< $'n\np\n1\n\n\nw\n'; local d=sdf ;;
			*) echo "You selected > 7, or inputed an incorrect number"; exit 1 ;;
		esac
}

# Download images & check
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

# MD5 hash check
function MD5 () {
	if [[ $(cat $2 | cut -d' ' -f1) != $(md5sum $1 | cut -d' ' -f1) ]]; then
			 echo "Bad md5 detected on $1"
			exit 1
	fi
}

function Extract () {
	tar -xzf plugbox-pogoplug.tar.gz -C /new_root/
	## place some variable for *.tar.gz & call in appropriate function (usb or nand)
}

# Install the extracted tar
function Install () {
	touch /plugapps
	if [ -f /root/.bash_profile ]; then
		rm /root/.bash_profile
	fi
	touch root/.bash_profile
	echo "export TERM=linux" > root/.bash_profile
	rm plugbox-pogoplug.tar.gz
	## add variable here too
}

# Modify the default rcS (need to get rid of this & use default rc.conf)
function ModifyrcS () {
	cd /etc/init.d
	mv rcS rcS.backup
	if [[ "$FlashType" == "nand" ]]; then
		wget http://plugapps.com/os/pogoplug/v1/rcS
	fi	
	if [[ "$FlashType" == "usb" ]]; then
		wget http://plugapps.com/os/pogoplug/v2/rcS
	fi	
	chmod 755 rcS
	cat <<MODRCS
Would like to change your hostname or do other boot tweaks?
edit: /etc/init.d/rcS
MODRCS
}

# Reboot (Hopefully everything went smoothly)
function Reboot () {
	cat <<REBOOT
Congrats, you've just flashed your device!
You now need to reboot so that the changes can take effect.
You will need to remove your previous ssh keys (on client) otherwise you won't be able to login.
	rm ~/.ssh/known_hosts
	
Log back in with:
	User: root
	Pass: ceadmin

If everything completed successfully you will be greeted with 'root@PlugApps' upon reboot.

Press ENTER to reboot...
REBOOT
read enter
}

###############################
## END Function Declarations ##
###############################


if [[ $# < 1 ]]; then
	Intro
	exit 1
fi

if [[ "$1" == "--nand" ]]; then
	Nand
	
elif [[ "$1" == "--usb" ]]; then
	Usb
	
else
	Intro
	exit 1
fi