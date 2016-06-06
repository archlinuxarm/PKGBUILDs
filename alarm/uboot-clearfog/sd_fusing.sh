#!/usr/bin/bash
####################################
#
# Copyright (C) 2011 Samsung Electronics Co., Ltd.
#              http://www.samsung.com/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
####################################
#
# Modified for Arch Linux ARM package uboot-clearfog
#
####################################

_device=$1

sd_fuse() {
  ####################################
  # fusing images

  echo "u-boot fusing version $1"
  dd if=./u-boot-clearfog.$1.mmc of=$_device bs=512 seek=1

  echo "u-boot environemnt fusing"
  dd if=/dev/zero of=$_device bs=512 seek=1920 count=128
  dd if=./clearfog.env of=$_device bs=512 seek=1920

  ####################################
  #<Message Display>
  echo "U-boot image is fused successfully."

}

choose_uboot() {
  while :
  do
    cat<<EOF
    Please choose uboot to flash:
       <1st slot, 2nd slot>
    1) PCIe, PCIe (default)
    2) PCIe, mSata
    3) mSata, PCIe
    4) mSata, mSata
EOF
      read -n1 -s
      case "$REPLY" in
      "1")  sd_fuse pp 
            break
            ;;
      "2")  sd_fuse ps
            break
            ;;
      "3")  sd_fuse sp
            break
            ;;
      "4")  sd_fuse ss
            break
            ;;
       * )  echo "invalid option"     ;;
      esac
  done
}


if [ -z $_device ]
then
    echo "usage: ./sd_fusing.sh <SD Reader's device file>"
    exit 0
fi

if [ -b $_device ]
then
    echo "$_device reader is identified."
else
    echo "$_device is NOT identified."
    exit 0
fi

choose_uboot