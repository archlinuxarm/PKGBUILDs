# ARMv7 multi-platform
# Maintainer: Kevin Mihelich <kevin@archlinuxarm.org>

buildarch=4

pkgbase=linux-armv7
_srcname=linux-5.11
_kernelname=${pkgbase#linux}
_desc="ARMv7 multi-platform"
pkgver=5.11.2
pkgrel=1
rcnver=5.11.0
rcnrel=armv7-x10
arch=('armv7h')
url="http://www.kernel.org/"
license=('GPL2')
makedepends=('xmlto' 'docbook-xsl' 'kmod' 'inetutils' 'bc' 'git' 'uboot-tools' 'vboot-utils' 'dtc')
options=('!strip')
source=("http://www.kernel.org/pub/linux/kernel/v5.x/${_srcname}.tar.xz"
        "http://www.kernel.org/pub/linux/kernel/v5.x/patch-${pkgver}.xz"
        "http://rcn-ee.com/deb/stretch-armhf/v${rcnver}-${rcnrel}/patch-${rcnver%.0}-${rcnrel}.diff.xz"
        '0001-ARM-atags-add-support-for-Marvell-s-u-boot.patch'
        '0002-ARM-atags-fdt-retrieve-MAC-addresses-from-Marvell-bo.patch'
        '0003-SMILE-Plug-device-tree-file.patch'
        '0004-fix-mvsdio-eMMC-timing.patch'
        '0005-net-smsc95xx-Allow-mac-address-to-be-set-as-a-parame.patch'
        '0006-set-default-cubietruck-led-triggers.patch'
        '0007-exynos4412-odroid-set-higher-minimum-buck2-regulator.patch'
        '0008-ARM-dove-enable-ethernet-on-D3Plug.patch'
        '0009-USB-Armory-MkII-support.patch'
        'config'
        'kernel.its'
        'kernel.keyblock'
        'kernel_data_key.vbprivk'
        'linux.preset'
        '60-linux.hook'
        '90-linux.hook')
md5sums=('d2985a3f16ef1ea3405c04c406e29dcc'
         '4f2437097d20455c71d1b1866469d6be'
         'fe141ac54d974f1082bc1e5399c71447'
         '6c3c583d8fa165315791216c5dce9fe4'
         '0b1fac542a49c8756c4c66d55df7a609'
         '5b303c16f3957d0c79285bcddae34218'
         '85501c220fbb3c80b5a2836e4764dbe0'
         '6e791015c502e6931ca1d27a4dde006a'
         'f704f034ac86b058c58ef82499a0cd0e'
         'eef2303908ced1604187ae76cdf2b9d0'
         '56f2d15c883be64c182073744540a7c6'
         '0e76e8e7d3d6c5961cd7276a4df9f3a3'
         '5102e6401a360fce96c502adf026eff5'
         '4f2379ed84258050edb858ee8d281678'
         '61c5ff73c136ed07a7aadbf58db3d96a'
         '584777ae88bce2c5659960151b64c7d8'
         '86d4a35722b5410e3b29fc92dae15d4b'
         'ce6c81ad1ad1f8b333fd6077d47abdaf'
         '3e2a512f8da5db5fe9f17875405e56a3')

prepare() {
  cd "${srcdir}/${_srcname}"

  # add upstream patch
  git apply --whitespace=nowarn ../patch-${pkgver}

  # RCN patch
  git apply ../patch-${rcnver%.0}-${rcnrel}.diff

  # ALARM patches
  git apply ../0001-ARM-atags-add-support-for-Marvell-s-u-boot.patch
  git apply ../0002-ARM-atags-fdt-retrieve-MAC-addresses-from-Marvell-bo.patch
  git apply ../0003-SMILE-Plug-device-tree-file.patch
  git apply ../0004-fix-mvsdio-eMMC-timing.patch
  git apply ../0005-net-smsc95xx-Allow-mac-address-to-be-set-as-a-parame.patch
  git apply ../0006-set-default-cubietruck-led-triggers.patch
  git apply ../0007-exynos4412-odroid-set-higher-minimum-buck2-regulator.patch
  git apply ../0008-ARM-dove-enable-ethernet-on-D3Plug.patch
  git apply ../0009-USB-Armory-MkII-support.patch

  cat "${srcdir}/config" > ./.config

  # add pkgrel to extraversion
  sed -ri "s|^(EXTRAVERSION =)(.*)|\1 \2-${pkgrel}|" Makefile

  # don't run depmod on 'make install'. We'll do this ourselves in packaging
  sed -i '2iexit 0' scripts/depmod.sh
}

build() {
  cd "${srcdir}/${_srcname}"

  # get kernel version
  make prepare

  # load configuration
  # Configure the kernel. Replace the line below with one of your choice.
  #make menuconfig # CLI menu for configuration
  #make nconfig # new CLI menu for configuration
  #make xconfig # X-based configuration
  #make oldconfig # using old config from previous kernel version
  # ... or manually edit .config

  # Copy back our configuration (use with new kernel version)
  #cp ./.config ../${pkgbase}.config

  ####################
  # stop here
  # this is useful to configure the kernel
  #msg "Stopping build"
  #return 1
  ####################

  #yes "" | make config

  # build!
  make ${MAKEFLAGS} zImage modules dtbs
}

_package() {
  pkgdesc="The Linux Kernel and modules - ${_desc}"
  depends=('coreutils' 'linux-firmware' 'kmod' 'mkinitcpio>=0.7')
  optdepends=('crda: to set the correct wireless channels of your country')
  backup=("etc/mkinitcpio.d/${pkgbase}.preset")
  provides=("linux=${pkgver}" "WIREGUARD-MODULE")
  conflicts=('linux')
  replaces=('linux-mvebu' 'linux-udoo' 'linux-sun4i' 'linux-sun5i' 'linux-sun7i' 'linux-usbarmory' 'linux-wandboard' 'linux-clearfog')
  install=${pkgname}.install

  cd "${srcdir}/${_srcname}"

  KARCH=arm

  # get kernel version
  _kernver="$(make kernelrelease)"
  _basekernel=${_kernver%%-*}
  _basekernel=${_basekernel%.*}

  mkdir -p "${pkgdir}"/{boot,usr/lib/modules}
  make INSTALL_MOD_PATH="${pkgdir}/usr" modules_install
  make INSTALL_DTBS_PATH="${pkgdir}/boot/dtbs" dtbs_install
  cp arch/$KARCH/boot/zImage "${pkgdir}/boot/zImage"

  # make room for external modules
  local _extramodules="extramodules-${_basekernel}${_kernelname}"
  ln -s "../${_extramodules}" "${pkgdir}/usr/lib/modules/${_kernver}/extramodules"

  # add real version for building modules and running depmod from hook
  echo "${_kernver}" |
    install -Dm644 /dev/stdin "${pkgdir}/usr/lib/modules/${_extramodules}/version"

  # remove build and source links
  rm "${pkgdir}"/usr/lib/modules/${_kernver}/{source,build}

  # now we call depmod...
  depmod -b "${pkgdir}/usr" -F System.map "${_kernver}"

  # sed expression for following substitutions
  local _subst="
    s|%PKGBASE%|${pkgbase}|g
    s|%KERNVER%|${_kernver}|g
    s|%EXTRAMODULES%|${_extramodules}|g
  "

  # install mkinitcpio preset file
  sed "${_subst}" ../linux.preset |
    install -Dm644 /dev/stdin "${pkgdir}/etc/mkinitcpio.d/${pkgbase}.preset"

  # install pacman hooks
  sed "${_subst}" ../60-linux.hook |
    install -Dm644 /dev/stdin "${pkgdir}/usr/share/libalpm/hooks/60-${pkgbase}.hook"
  sed "${_subst}" ../90-linux.hook |
    install -Dm644 /dev/stdin "${pkgdir}/usr/share/libalpm/hooks/90-${pkgbase}.hook"
}

_package-headers() {
  pkgdesc="Header files and scripts for building modules for linux kernel - ${_desc}"
  provides=("linux-headers=${pkgver}")
  conflicts=('linux-headers')
  replaces=('linux-mvebu-headers' 'linux-sun4i-headers' 'linux-sun5i-headers' 'linux-sun7i-headers' 'linux-usbarmory-headers' 'linux-wandboard-headers' 'linux-clearfog-headers')

  cd ${_srcname}
  local _builddir="${pkgdir}/usr/lib/modules/${_kernver}/build"

  install -Dt "${_builddir}" -m644 Makefile .config Module.symvers
  install -Dt "${_builddir}/kernel" -m644 kernel/Makefile

  mkdir "${_builddir}/.tmp_versions"

  cp -t "${_builddir}" -a include scripts

  install -Dt "${_builddir}/arch/${KARCH}" -m644 arch/${KARCH}/Makefile
  install -Dt "${_builddir}/arch/${KARCH}/kernel" -m644 arch/${KARCH}/kernel/asm-offsets.s

  cp -t "${_builddir}/arch/${KARCH}" -a arch/${KARCH}/include
  for i in dove omap2; do
    mkdir -p "${_builddir}/arch/${KARCH}/mach-${i}"
    cp -t "${_builddir}/arch/${KARCH}/mach-${i}" -a arch/$KARCH/mach-${i}/include
  done
  for i in omap orion versatile; do
    mkdir -p "${_builddir}/arch/${KARCH}/plat-${i}"
    cp -t "${_builddir}/arch/${KARCH}/plat-${i}" -a arch/$KARCH/plat-${i}/include
  done

  install -Dt "${_builddir}/drivers/md" -m644 drivers/md/*.h
  install -Dt "${_builddir}/net/mac80211" -m644 net/mac80211/*.h

  # http://bugs.archlinux.org/task/13146
  install -Dt "${_builddir}/drivers/media/i2c" -m644 drivers/media/i2c/msp3400-driver.h

  # http://bugs.archlinux.org/task/20402
  install -Dt "${_builddir}/drivers/media/usb/dvb-usb" -m644 drivers/media/usb/dvb-usb/*.h
  install -Dt "${_builddir}/drivers/media/dvb-frontends" -m644 drivers/media/dvb-frontends/*.h
  install -Dt "${_builddir}/drivers/media/tuners" -m644 drivers/media/tuners/*.h

  # add xfs and shmem for aufs building
  mkdir -p "${_builddir}"/{fs/xfs,mm}

  # copy in Kconfig files
  find . -name Kconfig\* -exec install -Dm644 {} "${_builddir}/{}" \;

  # remove unneeded architectures
  local _arch
  for _arch in "${_builddir}"/arch/*/; do
    [[ ${_arch} == */${KARCH}/ ]] && continue
    rm -r "${_arch}"
  done

  # remove files already in linux-docs package
  rm -r "${_builddir}/Documentation"

  # remove now broken symlinks
  find -L "${_builddir}" -type l -printf 'Removing %P\n' -delete

  # Fix permissions
  chmod -R u=rwX,go=rX "${_builddir}"

  # strip scripts directory
  local _binary _strip
  while read -rd '' _binary; do
    case "$(file -bi "${_binary}")" in
      *application/x-sharedlib*)  _strip="${STRIP_SHARED}"   ;; # Libraries (.so)
      *application/x-archive*)    _strip="${STRIP_STATIC}"   ;; # Libraries (.a)
      *application/x-executable*) _strip="${STRIP_BINARIES}" ;; # Binaries
      *) continue ;;
    esac
    /usr/bin/strip ${_strip} "${_binary}"
  done < <(find "${_builddir}/scripts" -type f -perm -u+w -print0 2>/dev/null)
}

_package-smileplug() {
  pkgdesc="The Linux Kernel - ${_desc} - Marvell SMILE Plug"
  depends=('linux-armv7')
  provides=('linux-armv7-uimage')
  conflicts=('linux-armv7-uimage' 'linux-armv7-rc-uimage')
  replaces=('linux-mvebu-smileplug')

  cd "${srcdir}/${_srcname}"

  mkdir -p "${pkgdir}/boot"
  cat arch/$KARCH/boot/zImage arch/$KARCH/boot/dts/armada-370-smileplug.dtb > myimage
  mkimage -A arm -O linux -T kernel -C none -a 0x00008000 -e 0x00008000 -n "${pkgname}" -d myimage "${pkgdir}/boot/uImage"
}

_package-mirabox() {
  pkgdesc="The Linux Kernel - ${_desc} - Globalscale Mirabox"
  depends=('linux-armv7')
  provides=('linux-armv7-uimage')
  conflicts=('linux-armv7-uimage' 'linux-armv7-rc-uimage')
  replaces=('linux-mvebu-mirabox')

  cd "${srcdir}/${_srcname}"

  mkdir -p "${pkgdir}/boot"
  cat arch/$KARCH/boot/zImage arch/$KARCH/boot/dts/armada-370-mirabox.dtb > myimage
  mkimage -A arm -O linux -T kernel -C none -a 0x00008000 -e 0x00008000 -n "${pkgname}" -d myimage "${pkgdir}/boot/uImage"
}

_package-ax3() {
  pkgdesc="The Linux Kernel - ${_desc} - OpenBlocks AX3-4"
  depends=('linux-armv7')
  provides=('linux-armv7-uimage')
  conflicts=('linux-armv7-uimage' 'linux-armv7-rc-uimage')
  replaces=('linux-mvebu-ax3')

  cd "${srcdir}/${_srcname}"

  mkdir -p "${pkgdir}/boot"
  cat arch/$KARCH/boot/zImage arch/$KARCH/boot/dts/armada-xp-openblocks-ax3-4.dtb > myimage
  mkimage -A arm -O linux -T kernel -C none -a 0x00008000 -e 0x00008000 -n "${pkgname}" -d myimage "${pkgdir}/boot/uImage"
}

_package-d3plug() {
  pkgdesc="The Linux Kernel - ${_desc} - Globalscale D3Plug"
  depends=('linux-armv7')
  provides=('linux-armv7-uimage')
  conflicts=('linux-armv7-uimage' 'linux-armv7-rc-uimage')
  replaces=('linux-d3plug')

  cd "${srcdir}/${_srcname}"

  mkdir -p "${pkgdir}/boot"
  cat arch/$KARCH/boot/zImage arch/$KARCH/boot/dts/dove-d3plug.dtb > myimage
  mkimage -A arm -O linux -T kernel -C none -a 0x00008000 -e 0x00008000 -n "${pkgname}" -d myimage "${pkgdir}/boot/uImage"
}

_package-cubox() {
  pkgdesc="The Linux Kernel - ${_desc} - SolidRun Cubox (Marvell)"
  depends=('linux-armv7')
  provides=('linux-armv7-uimage')
  conflicts=('linux-armv7-uimage' 'linux-armv7-rc-uimage')
  #replaces=('linux-cubox')

  cd "${srcdir}/${_srcname}"

  mkdir -p "${pkgdir}/boot"
  cat arch/$KARCH/boot/zImage arch/$KARCH/boot/dts/dove-cubox.dtb > myimage
  mkimage -A arm -O linux -T kernel -C none -a 0x00008000 -e 0x00008000 -n "${pkgname}" -d myimage "${pkgdir}/boot/uImage"
}

_package-chromebook() {
  pkgdesc="The Linux Kernel - ${_desc} - Chromebooks"
  depends=('linux-armv7')
  conflicts=('linux-armv7-rc-chromebook')
  install=${pkgname}.install

  cd "${srcdir}/${_srcname}"

  cp ../kernel.its .
  mkimage -D "-I dts -O dtb -p 2048" -f kernel.its vmlinux.uimg
  dd if=/dev/zero of=bootloader.bin bs=512 count=1
  echo 'console=tty0 init=/sbin/init root=PARTUUID=%U/PARTNROFF=1 rootwait rw noinitrd' > cmdline
  vbutil_kernel \
    --pack vmlinux.kpart \
    --version 1 \
    --vmlinuz vmlinux.uimg \
    --arch arm \
    --keyblock ../kernel.keyblock \
    --signprivate ../kernel_data_key.vbprivk \
    --config cmdline \
    --bootloader bootloader.bin

  mkdir -p "${pkgdir}/boot"
  cp vmlinux.kpart "${pkgdir}/boot"
}

_package-odroidc1() {
  pkgdesc="The Linux Kernel - ${_desc} - ODROID-C1"
  depends=('linux-armv7')
  provides=('linux-armv7-uimage')
  conflicts=('linux-armv7-uimage' 'linux-armv7-rc-uimage')

  cd "${srcdir}/${_srcname}"

  mkdir -p "${pkgdir}/boot"
  mkimage -A arm -O linux -T kernel -C none -a 0x00208000 -e 0x00208000 -n "${pkgname}" -d arch/$KARCH/boot/zImage "${pkgdir}/boot/uImage"
}

pkgname=("${pkgbase}" "${pkgbase}-headers" "${pkgbase}-smileplug" "${pkgbase}-mirabox" "${pkgbase}-ax3" "${pkgbase}-d3plug" "${pkgbase}-cubox" "${pkgbase}-chromebook" "${pkgbase}-odroidc1")
for _p in ${pkgname[@]}; do
  eval "package_${_p}() {
    _package${_p#${pkgbase}}
  }"
done
