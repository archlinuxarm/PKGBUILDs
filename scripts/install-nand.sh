#! /bin/bash
#
# PlugBox for Pogoplug v1 NAND Flash Install Script
# Version 0.2.4
#
# Warnings
echo "Welcome to the PlugBox for Pogoplug v1 NAND Installer"
echo ""
echo "Any files previously in /opt (mtdblock3) from OpenPogo will be ERASED!"
echo "Exit the Installer if you have not backed up custom configurations, etc."
echo ""
echo ""
echo "Are you sure you want to run the installer?"
echo "Press ENTER if yes, CTRL+C if no."
read enter
#
# Making directories and cleaning out the USB drive
#
cd /
mount -o rw,remount /
mkdir -p /new_root
umount /dev/mtdblock3
mount /dev/mtdblock3 /new_root
rm -rf /new_root/*
#
# Downloading and Extracting
#
echo -e "\033[1mDownloading PlugBox Linux\033[0m"
cd /new_root/
wget http://plugapps.com/os/pogoplug/plugbox-pogoplug.tar.gz
echo "Extracting PlugBox OS. This may take a few minutes."
tar -xzf plugbox-pogoplug.tar.gz
mv new_root/* ./
rmdir new_root
touch plugapps
rm /root/.bash_profile
touch root/.bash_profile
echo "export TERM=linux" > root/.bash_profile
rm plugbox-pogoplug.tar.gz
#
# Modifying Boot Files
#
cd /etc/init.d
mv rcS rcS.backup
wget http://plugapps.com/os/pogoplug/v1/rcS
chmod 755 rcS
#
# Rebooting
#
echo "Your Pogoplug Needs to be Restarted."
echo "You will have to remove SSH keys on Linux/Mac OS X to log back in."
echo "Type 'rm ~/.ssh/known_hosts"
echo ""
echo "You'll notice PlugApps are ready when you login and see a root@PogoPlug prompt."
echo "Press ENTER to reboot."
read enter
/sbin/reboot