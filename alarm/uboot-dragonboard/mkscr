#!/bin/bash

if [[ ! -x /usr/bin/mkimage ]]; then
  echo "mkimage not found. Please install uboot-tools:"
  echo "  pacman -S uboot-tools"
  exit 1
fi

mkimage -A arm -O linux -T script -C none -n "U-Boot boot script" -d boot.txt boot.scr
