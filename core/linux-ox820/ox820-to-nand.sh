#!/bin/bash

##########################################
echo "# ox820 NAND kernel update."
echo "# NOTE: This is configured for Pogoplug V3"
echo ""
TESTOX820=`cat /proc/cpuinfo | grep 'Oxsemi NAS'`

if [  "$TESTOX820" = "" ]; then
  echo "Not an OX820 board!"
  exit;
fi

NSLOT=A
NOFFSET="0x500000"
if [ "$1" = "--slot-b" ]; then
  NSLOT=B
  NOFFSET="0xB00000"
fi

echo "Flashing Kernel to slot $NSLOT..."
## Flash kernel
# erase
flash_erase /dev/mtd1 $NOFFSET 20
# write
nandwrite -p -s $NOFFSET /dev/mtd1 boot/uImage

