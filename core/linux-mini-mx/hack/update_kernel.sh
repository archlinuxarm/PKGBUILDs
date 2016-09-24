#!/usr/bin/env bash

# This script use the original recovery.raw ROM
# to bootstrap arch linux on Mini MX device.
# With this hack we can keep the original
# android system in the system ROM, and
# only boot the arch linux, when the SD card
# is inserted into device.

temp=`mktemp -d`

cd ${temp}

# First you should extract somehow TM the recovery image.
# hint: root, dd, mtd
cp /target/save/recovery.raw .

# Extract the recovery ROM
abootimg -x recovery.raw

# Create a new boot.cfg with the required parameters
# espcially the root device and its type
cat > ${temp}/boot.cfg<<EOF
pagesize = 0x800
kerneladdr = 0x1080000
ramdiskaddr = 0x1000000
secondaddr = 0xf00000
name = MiniMX
cmdline = root=/dev/mmcblk1p2 rootfstype=ext4
EOF

# Backup the previous kernel image fist
cp /target/arch-kernel.img /target/arch-kernel.img.old

# Generate our loader image
abootimg --create /target/arch-kernel.img -k /boot/Image -r /boot/initramfs-linux.img -s ${temp}/stage2.img -f ${temp}/boot.cfg

