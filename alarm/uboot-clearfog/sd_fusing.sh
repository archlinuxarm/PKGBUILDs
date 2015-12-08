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

if [ -z $1 ]
then
    echo "usage: ./sd_fusing.sh <SD Reader's device file>"
    exit 0
fi

if [ -b $1 ]
then
    echo "$1 reader is identified."
else
    echo "$1 is NOT identified."
    exit 0
fi

####################################
# fusing images

echo "u-boot fusing"
dd if=/boot/u-boot-clearfog.mmc of=/dev/mmcblk0 bs=512 seek=1

echo "u-boot environemnt fusing"
dd if=/dev/zero of=/dev/mmcblk0 bs=512 seek=1920 count=128
dd if=/boot/clearfog.env of=/dev/mmcblk0 bs=512 seek=1920

####################################
#<Message Display>
echo "U-boot image is fused successfully."
