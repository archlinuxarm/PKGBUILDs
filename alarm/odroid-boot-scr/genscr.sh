#!/bin/bash

HDTV_TYPE="dvi hdmi"
HDTV_FORMAT="480p60hz 576p50hz 720p60hz 720p50hz 1080p60hz 1080i60hz 1080i50hz 1080p50hz 1080p30hz 1080p25hz 1080p24hz"
RAM="1023M 2047M"

for hdtv_type in `echo $HDTV_TYPE`; do
  for hdtv_format in `echo $HDTV_FORMAT`; do
    for ram in `echo $RAM`; do
      echo "setenv fdt_high \"0xffffffff\"" >> ./boot.tmp
      echo "setenv hdtv_type \"$hdtv_type\"" >> ./boot.tmp
      echo "setenv hdtv_format \"$hdtv_format\"" >> ./boot.tmp
      echo "setenv bootcmd \"fatload mmc 0:1 0x40008000 zImage; bootm 0x40008000\"" >> ./boot.tmp
      echo "setenv bootargs \"console=tty1 console=ttySAC1,115200n8 hdtv_type=\${hdtv_type} hdtv_format=\${hdtv_format} root=/dev/mmcblk0p2 rootwait rw mem=\${ram}\"" >> ./boot.tmp
      echo "boot" >> ./boot.tmp
      mkimage -A arm -T script -C none -n "Boot.scr for $hdtv_type at $hdtv_format" -d ./boot.tmp ./boot-$hdtv_type-$hdtv_format-$ram.scr
      rm -rf boot.tmp
    done
  done
done
