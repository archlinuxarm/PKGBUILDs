#! /bin/bash
#
# PlugBox for Pogoplug v1/2 and Seagate DockStar USB Drive Install Script
# Version 0.2.4
#
# Warnings
echo -e "\033[1mWelcome to the PlugApps for Pogoplug v1/v2/DockStar USB Installer.\033[0m"
echo ""
echo "Remove ALL USB drives now. Press ENTER when done."
read enter
echo "Waiting 20 seconds for cleanup."
sleep 20
echo "Please insert ONLY the drive you want PlugApps to be installed on."
echo "ALL DATA ON THE DRIVE WILL BE ERASED"
echo ""
echo "Are you sure you want to run the installer?"
echo "Press ENTER if yes, CTRL+C if no."
read enter
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
echo -e "\033[1mDownloading PlugBox Linux\033[0m"
wget http://plugapps.com/os/pogoplug/plugbox-pogoplug.tar.gz
echo "Extracting PlugBox OS. This may take a few minutes."
tar -xzf plugbox-pogoplug.tar.gz
mv new_root/* ./
rmdir new_root
touch plugapps
touch root/.bash_profile
echo "export TERM=linux" > root/.bash_profile
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
echo "Your Pogoplug Needs to be Restarted."
echo "You will have to remove SSH keys on Linux/Mac OS X to log back in."
echo "Type 'rm ~/.ssh/known_hosts"
echo ""
echo "Also, to boot back into the regular Pogoplug installation, just remove the drive you installed PlugApps on and restart your device."
echo ""
echo "You'll notice PlugApps are ready when you login and see a root@PogoPlug prompt."
echo "Press ENTER to reboot."
read enter
/sbin/reboot