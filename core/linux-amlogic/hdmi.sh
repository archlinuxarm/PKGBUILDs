#!/bin/sh

#bpp=32
bpp=24

#mode=1080p60hz
mode=720p60hz

HPD_STATE=/sys/class/amhdmitx/amhdmitx0/hpd_state
DISP_CAP=/sys/class/amhdmitx/amhdmitx0/disp_cap
DISP_MODE=/sys/class/display/mode

echo $mode > $DISP_MODE

# Set framebuffer geometry to match the resolution
case $mode in
	1080*)
	fbset -fb /dev/fb0 -g 1920 1080 1920 2160 $bpp
	fbset -fb /dev/fb1 -g 32 32 32 32 32
	echo $mode > /sys/class/display/mode
	echo 0 > /sys/class/graphics/fb0/free_scale
	echo 1 > /sys/class/graphics/fb0/freescale_mode
	echo 0 0 1919 1079 > /sys/class/graphics/fb0/free_scale_axis
	echo 0 0 1919 1079 > /sys/class/graphics/fb0/window_axis
	echo 0 > /sys/class/graphics/fb1/free_scale
	;;
      *)
	fbset -fb /dev/fb0 -g 1280 720 1280 1440 $bpp
	fbset -fb /dev/fb1 -g 32 32 32 32 32
	echo $mode > /sys/class/display/mode
	echo 0 > /sys/class/graphics/fb0/free_scale
	echo 1 > /sys/class/graphics/fb0/freescale_mode
	echo 0 0 1279 719 > /sys/class/graphics/fb0/free_scale_axis
	echo 0 0 1279 719 > /sys/class/graphics/fb0/window_axis
	echo 0 > /sys/class/graphics/fb1/free_scale
	;;
esac

#echo 1 > /sys/class/video/disable_video

# Enable framebuffer device
echo 0 > /sys/class/graphics/fb0/blank

# Blank fb1 to prevent static noise
#echo 0 > /sys/class/graphics/fb1/blank

#echo -en "\033[?25h"
echo 1 > /sys/devices/virtual/graphics/fbcon/cursor_blink

#/etc/webmin/start &

#su -c 'hciattach /dev/ttyS1 any'
