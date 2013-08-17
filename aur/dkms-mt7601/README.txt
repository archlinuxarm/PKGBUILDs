This is an Arch Linux dkms package for the wifi chipset Ralink7601 (MT7601)

I bought a wifi USB dongle from Logic Supply, they provide instructions for
buildign the driver:

http://www.logicsupply.com/products/uwn100
https://docs.google.com/document/d/1-CIGQYdk8ZhU3D3UCNn70jc7C9HdXvEZAsiNW71fGIE/edit

I tested on the BeagelBone Black SBC.

$ cat /proc/cpuinfo
processor       : 0
model name      : ARMv7 Processor rev 2 (v7l)
BogoMIPS        : 660.76
Features        : swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls
CPU implementer : 0x41
CPU architecture: 7
CPU variant       : 0x3
CPU part          : 0xc08
CPU revision      : 2
Hardware          : Generic AM33XX (Flattened Device Tree)

$ uname -a
Linux bbb_001 3.8.13-8-ARCH #1 SMP Sun Aug 11 02:59:32 CDT 2013 armv7l GNU/Linux


#Create the package:
makepkg -A --asroot -f

#Install
pacman -Uv dkms-mt7601-v0.0.0-1-armv7h.pkg.tar.xz

NOTES:

* I was able to get the module to get loaded adn bring up the network over wifi.

* The build package probably needs improvement from someone who knows more than I.
