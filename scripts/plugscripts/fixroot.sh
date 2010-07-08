#!/bin/sh

if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root." 1>&2
   exit 1
fi

echo "This script will fix fstab for you so that tools like df work correctly."
echo ""
echo ""
echo "Please select your root device from the list below:"

showMenu () {
        echo "1) NAND"
        echo "2) SD"
        echo "3) USB"
	echo "4) Exit"
}
showMenu
read CHOICE

case "$CHOICE" in
                "1"|"NAND")
                        echo "ubi0:rootfs            /            ubifs    defaults,noatime    0      0" >> /etc/fstab
                        ;;
                "2"|"SD")
                        echo "/dev/mmcblk0p1            /            ext2    defaults,noatime    0      0" >> /etc/fstab
                        ;;
		"3"|"USB") 
			echo "/dev/sda1            /            ext2    defaults,noatime    0      0" >> /etc/fstab
			;;
                "4"|"Exit")
                        exit
                        ;;

esac

clear
echo "Below is the new fstab, check it before rebooting!"
echo ""
echo ""
echo ""
echo ""
cat /etc/fstab
