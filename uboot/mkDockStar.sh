#!/bin/sh
die() {
    echo "$1"
    exit 1
}
grep 00:10:75:12:34:56 include/configs/sheevaplug.h && die "Please change the MAC in include/configs/sheevaplug.h"
grep \\.ip\\. include/configs/sheevaplug.h && die "Please change the IPs in include/configs/sheevaplug.h"
make mrproper
make sheevaplug_config
make u-boot.kwb
echo "Building u-boot.bin.pagesize"
# Slow but does the trick
bytes=$(stat --format='%s' u-boot.bin)
fillbytes=$[262144-$bytes]
cp u-boot.bin u-boot.bin.pagesize
while [ $fillbytes -ne 0 ]; do
  printf "\xff" >>u-boot.bin.pagesize
  fillbytes=$[$fillbytes-1]
done
