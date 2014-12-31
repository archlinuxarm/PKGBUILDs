#!/bin/sh

for x in $(cat /proc/cmdline); do
	case ${x} in 
		m_bpp=*) export bpp=${x#*=} ;;
		hdmimode=*) export mode=${x#*=} ;;
	esac
done

HPD_STATE=/sys/class/amhdmitx/amhdmitx0/hpd_state
DISP_CAP=/sys/class/amhdmitx/amhdmitx0/disp_cap
DISP_MODE=/sys/class/display/mode

hdmi=`cat $HPD_STATE`
if [ $hdmi -eq 1 ]; then
    echo $mode > $DISP_MODE
fi

outputmode=$mode

case $mode in
    800x480*) fbset -fb /dev/fb0 -g 800 480 800 960 $bpp ;;
    vga*)   fbset -fb /dev/fb0 -g 640 480 640 960 $bpp ;;
    480*)   fbset -fb /dev/fb0 -g 720 480 720 960 $bpp ;;
    svga*)  fbset -fb /dev/fb0 -g 800 600 800 1200 $bpp ;;
    576*)   fbset -fb /dev/fb0 -g 720 576 720 1152 $bpp ;;
    720*)   fbset -fb /dev/fb0 -g 1280 720 1280 1440 $bpp ;;
    800*)   fbset -fb /dev/fb0 -g 1280 800 1280 1600 $bpp ;;
    sxga*)  fbset -fb /dev/fb0 -g 1280 1024 1280 2048 $bpp ;;
    wsxga*) fbset -fb /dev/fb0 -g 1440 900 1440 1800 $bpp ;;
    1080*)  fbset -fb /dev/fb0 -g 1920 1080 1920 2160 $bpp ;;
    1920x1200*) fbset -fb /dev/fb0 -g 1920 1200 1920 2400 $bpp ;;
esac
fbset -fb /dev/fb1 -g 32 32 32 32 32

echo $outputmode > /sys/class/display/mode

echo 0 > /sys/class/ppmgr/ppscaler
echo 0 > /sys/class/graphics/fb0/free_scale
echo 1 > /sys/class/graphics/fb0/freescale_mode


case $outputmode in
        800x480*) M="0 0 799 479" ;;
        vga*)  M="0 0 639 749" ;;
        svga*) M="0 0 799 599" ;;
        sxga*) M="0 0 1279 1023" ;;
        wsxga*) M="0 0 1439 899" ;;
        480*) M="0 0 719 479" ;;
        576*) M="0 0 719 575" ;;
        720*) M="0 0 1279 719" ;;
        800*) M="0 0 1279 799" ;;
        1080*) M="0 0 1919 1079" ;;
        1920x1200*) M="0 0 1919 1199" ;;
esac

echo $M > /sys/class/graphics/fb0/free_scale_axis
echo $M > /sys/class/graphics/fb0/window_axis


echo 0x10001 > /sys/class/graphics/fb0/free_scale
echo 0 > /sys/class/graphics/fb1/free_scale
