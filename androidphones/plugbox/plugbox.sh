# This script is taken from NexusOneHacks.net and modified for
# Plugbox Linux

mount -o rw,remount -t yaffs2 /dev/block/mtdblock3 /system
modprobe ext2

clear

rm -f /data/local/bin/fsrw
rm -f /data/local/bin/bootplugbox
rm -f /data/local/bin/unionfs
rm -f /data/local/bin/installer.sh
rm -f /data/local/bin/mountonly

mkdir /data/local/mnt

clear
rm  /system/bin/fsrw
rm  /system/bin/bootplugbox
rm  /system/bin/unionfs
rm -/system/bin/mountonly

cp -f fsrw /system/bin
cp -f bootplugbox /system/bin
cp -f unionfs /system/bin
cp -f mountonly /system/bin


cd /sdcard/plugbox

chmod 4777 *

cd /system/bin/

chmod 4777 *

cd /

clear

echo " "
echo "Plugbox Chroot Bootloader v0.1"
echo "Plugbox Bootloader is now installed!"
echo "This process does NOT damage Android!"
echo " "
echo "Original Installer by Charan Singh"
echo "Modified for Ubuntu Chroot by Max Lee at NexusOneHacks.net"
echo "Modified for Plugbox Linux by PlugApps.com"
echo " "
echo "To enter Plugbox Linux console just type 'bootplugbox'"
