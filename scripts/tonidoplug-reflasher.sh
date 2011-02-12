#!/bin/bash
#
# PlugApps Plug Flasher
#
# The following script will take a rootfs file and flash it
# on a TonidoPlug's NAND. Stick this on a PlugApps USB drive
# along with a rootfs.tar.gz (set it below), SSH in, and run this.
#
# It's based on the official TonidoPlug reflasher: http://www.tonido.com/support/TonidoPlug_Flash_LinuxOS
#
# Set this to where you put the rootfs on the USB drive! e.g. make a folder
# named "flash" on your drive and put the rootfs in there.
#
ROOTFS=/flash/PlugApps-Linux-2011.02-beta1-rootfs.tar.gz
#
echo "This script will completely erase your TonidoPlug's firmware"
echo "and will replace it with PlugApps. You will then be able to"
echo "use PlugApps from the internal storage instead of from USB."
echo ""
echo "This will take up to 15 minutes."
echo ""
echo "If this is not what you want, press CTRL-X now."
echo "Otherwise, press ENTER to start reflashing."
#
echo "Accessing internal storage..."
ubiattach /dev/ubi_ctrl -m 2
mount -t ubifs ubi0:rootfs /mnt/
echo "Wiping internal filesystem contents..."
rm -rf /mnt/*
echo "Flashing PlugApps..."
tar -C /mnt/ -xzf $ROOTFS >& /dev/null
echo "Flashing kernel..."
flash_eraseall -j /dev/mtd1
nandwrite -pm /dev/mtd1 /mnt/boot/uImage
echo "Unmounting..."
umount /mnt/
ubidetach /dev/ubi_ctrl -m 2
#
echo "All done, you now have PlugApps on your TonidoPlug!"
echo "Remove all USB drives attached to the plug and reboot."
