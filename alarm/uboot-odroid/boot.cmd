#
# Authors:
#   Zoltan Tombol <zoltan dot tombol at gmail>
#
# boot.cmd
# --------
#
# Boot script serving as a fallback when `extlinux.conf' is not
# available.
#
# It supports booting multiple image formats and customising the
# environment using a text file.
#
# Generate the loadable script using the following command.
#
#   ./tools/mkimage -A arm -T script -n 'Fallback boot script' \
#                   -d boot.cmd boot.scr
#
#
# Boot process
# ------------
#
# First, the environment is imported from `uEnv.txt'. Then, the boot
# images are tried one by one until one succeeds as shown below.
#
#   1. Load environment file `uEnv.txt'
#   2. Attempt booting in the following order
#     a. Flattened Image Tree `Image.itb'
#     b. Compressed kernel image `zImage'
#     c. Legacy `uImage'
#
# See `Environment' below to learn how to select the device tree and
# initial ramdisk to load.
#
#
# Environment
# -----------
#
# The following variables are defined by the compiled-in environment.
#
#   Memory layout:
#
#     fdt_addr_r      - flattened device tree
#     ramdisk_addr_r  - initial ramdisk
#     kernel_addr_r   - kernel
#     pxefile_addr_r  - PXE configuration file and `extlinux.conf'
#     scriptaddr      - boot script `boot.scr'
#     loadaddr        - default load address
#
#   Boot device and partition:
#
#     devtype         - device type, e.g. `mmc' or `usb'
#     devnum          - device number, e.g. `0' for `mmc 0'
#     distro_bootpart - partition number, e.g. `1' for `mmc 0:1'
#     prefix          - boot directory path, e.g. `/' or `/boot/'
#
#
# The following variables can be modified in `uEnv.txt' to customise the
# boot process. The default values are shown in square brackets.
#
#   Kernel parameters:
#
#     root       - partition of root filesystem   [/dev/mmcblk0p1]
#     console    - console settings               [ttySAC1,115200n8]
#     optargs    - additional kernel parameters
#     video      - video mode
#
#   Device tree and initial ramdisk:
#
#     initrdname - initial ramdisk file           [uInitrd]
#     fdtdir     - directory of device tree files [dtbs]
#     fdtfile    - name of device tree file       [exynos4412-odroid{x,x2,u3}.dtb]
#
#   Miscellaneous:
#
#     uenvcmd    - command to run after loading `uEnv.txt'
#

###############################################################################
# Common command variables.
###############################################################################

setenv kernel_args "
	setenv bootargs root='${root}' rootwait rw console='${console}'
		video='${video}' '${optargs}';"

setenv load_kernel "
	load ${devtype} ${devnum}:${distro_bootpart} ${kernel_addr_r}
		${prefix}'${kernelname}';"

setenv load_dtb "
	load ${devtype} ${devnum}:${distro_bootpart} ${fdt_addr_r}
		${prefix}'${fdtdir}/${fdtfile}';"

setenv check_dtb "
	if run load_dtb; then
		setenv fdtaddr ${fdt_addr_r};
	else
		setenv fdtaddr;
	fi;"

setenv load_initrd "
	load ${devtype} ${devnum}:${distro_bootpart} ${ramdisk_addr_r}
		${prefix}'${initrdname}';"

setenv check_initrd "
	if run load_initrd; then
		setenv initrdaddr ${ramdisk_addr_r};
	else
		setenv initrdaddr -;
	fi;"


###############################################################################
# Load environment from `uEnv.txt'.
###############################################################################

setenv load_bootenv "
	load ${devtype} ${devnum}:${distro_bootpart} ${scriptaddr}
		${prefix}uEnv.txt;"

setenv import_bootenv "env import -t ${scriptaddr} '${filesize}';"

setenv scan_dev_for_bootenv "
	if test -e ${devtype}
			${devnum}:${distro_bootpart}
			${prefix}uEnv.txt; then
		echo Found environment file ${prefix}uEnv.txt;
		run load_bootenv;
		run import_bootenv;
		if test -n '${uenvcmd}'; then
			echo Running uenvcmd ...;
			run uenvcmd;
		fi;
	fi;"


###############################################################################
# Boot from Flattened Image Tree `Image.itb'.
###############################################################################

setenv boot_fit "
	setenv kernel_addr_r ${scriptaddr};
	setenv kernelname Image.itb;
	run load_kernel;
	run kernel_args;
	bootm ${kernel_addr_r}#${boardname};"

setenv scan_dev_for_fit "
	if test -e ${devtype}
			${devnum}:${distro_bootpart}
			${prefix}Image.itb; then
		echo Found FIT image ${prefix}Image.itb;
		run boot_fit;
		echo BOOTING FAILED: continuing...;
	fi;"


###############################################################################
# Boot from compressed kernel image `zImage'.
###############################################################################

setenv boot_zimg "
	setenv kernelname zImage;
	run check_dtb;
	run check_initrd;
	run load_kernel;
	run kernel_args;
	bootz ${kernel_addr_r} '${initrdaddr}' '${fdtaddr}';"

setenv scan_dev_for_zimg "
	if test -e ${devtype}
			${devnum}:${distro_bootpart}
			${prefix}zImage; then
		echo Found zImage ${prefix}zImage;
		run boot_zimg;
		echo BOOTING FAILED: continuing...;
	fi;"


###############################################################################
# Boot from legacy `uImage'.
###############################################################################

setenv boot_uimg "
	setenv kernelname uImage;
	run check_dtb;
	run check_initrd;
	run load_kernel;
	run kernel_args;
	bootm ${kernel_addr_r} '${initrdaddr}' '${fdtaddr}';"

setenv scan_dev_for_uimg "
	if test -e ${devtype}
			${devnum}:${distro_bootpart}
			${prefix}uImage; then
		echo Found legacy uImage ${prefix}uImage;
		run boot_uimg;
		echo BOOTING FAILED: continuing...;
	fi;"


###############################################################################
# Main
###############################################################################

# Defaults.
setenv root       '/dev/mmcblk0p1'
setenv fdtdir     'dtbs'
setenv initrdname 'uInitrd'

setenv autoboot "
	run scan_dev_for_bootenv;
	run scan_dev_for_fit;
	run scan_dev_for_zimg;
	run scan_dev_for_uimg;"

run autoboot
