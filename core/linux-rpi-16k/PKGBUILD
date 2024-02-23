# Maintainer: graysky <therealgraysky AT proton DOT me>
# Maintainer: Kevin Mihelich <kevin@archlinuxarm.org>
# Maintainer: Oleg Rakhmanov <oleg@archlinuxarm.org>
# Maintainer: Dave Higham <pepedog@archlinuxarm.org>
# Contributer: Jan Alexander Steffens (heftig) <heftig@archlinux.org>

buildarch=8

pkgbase=linux-rpi-16k
_commit=aa41065014c753edd2038294adb957f117315c77
_srcname=linux-${_commit}
_kernelname=${pkgbase#linux}
_regen=
pkgver=6.6.18
pkgrel=1
pkgdesc='Linux'
url="https://github.com/raspberrypi/linux"
arch=(aarch64)
license=(GPL2)
makedepends=(
  bc
  kmod
  inetutils
)
options=('!strip')
source=("linux-$pkgver-${_commit:0:10}.tar.gz::https://github.com/raspberrypi/linux/archive/${_commit}.tar.gz"
        cmdline.txt
        config.txt
        config8
        0001-Make-proc-cpuinfo-consistent-on-arm64-and-arm.patch
        linux.preset
        archarm.diffconfig
)
md5sums=('73b1701d7f2a47028ff01543995e8dea'
         '3bab7426d8c8818dda8353da3892a41f'
         '16c484af9f72b9275afcf83a6b8eab36'
         '88f7e25c6072b0b8b1ef421ae05ffe31'
         'a157c5bfc0f03d0728c92bd953b06265'
         '86d4a35722b5410e3b29fc92dae15d4b'
         'c8f84694321e249492c80149833671d7')

# setup vars
_kernel=kernel8.img KARCH=arm64 _image=Image _config=config8

prepare() {
  cd "${srcdir}/${_srcname}"

  # consistent behavior of lscpu on arm/arm64
  patch -p1 -i ../0001-Make-proc-cpuinfo-consistent-on-arm64-and-arm.patch

  echo "Setting version..."
  echo "-$pkgrel" > localversion.10-pkgrel
  echo "${pkgbase#linux}" > localversion.20-pkgname

  if [[ $_regen -eq 1 ]]; then
    # useful on two point releases to keep shit straight
    echo "Applying custom shit to bcm2712_defconfig"
    make bcm2712_defconfig
    cat ../archarm.diffconfig >> .config
    make oldconfig
    # bcm2712_defconfig inserts a value for CONFIG_LOCALVERSION= so set this to null
    sed '/^CONFIG_LOCALVERSION=/s,.*$,CONFIG_LOCALVERSION="",' .config >$startdir/newconfig.$_config
    echo "verify that newconfig.$_config is fit for purpose then redefine $_config"
    exit
  else
    echo "Setting config..."
    cp ../"$_config" .config
    make olddefconfig
    diff -u ../"$_config" .config || :

    make -s kernelrelease > version
    echo "Prepared $pkgbase version $(<version)"
  fi
}

build() {
  cd "${srcdir}/${_srcname}"

  export KCFLAGS=' -mcpu=cortex-a76'
  export KCPPFLAGS=' -mcpu=cortex-a76'

  make "$_image" modules dtbs
}

_package() {
  pkgdesc="Linux kernel and modules (RPi Foundation fork) with 16k pagesize for bcm2712/RPi5 ONLY"
  depends=(
    coreutils
    firmware-raspberrypi
    kmod
    linux-firmware
    'mkinitcpio>=0.7'
  )
  optdepends=(
    'wireless-regdb: to set the correct wireless channels of your country'
  )
  provides=(
    linux="${pkgver}"
    WIREGUARD-MODULE
  )
  conflicts=(
    linux
    linux-rpi
    uboot-raspberrypi
  )
  install=${pkgname}.install
  backup=(
    boot/config.txt
    boot/cmdline.txt
  )

  cd "${srcdir}/${_srcname}"
  local modulesdir="$pkgdir/usr/lib/modules/$(<version)"

  # Used by mkinitcpio to name the kernel
  echo "$pkgbase" | install -Dm644 /dev/stdin "$modulesdir/pkgbase"

  echo "Installing modules..."
  make INSTALL_MOD_PATH="$pkgdir/usr" INSTALL_MOD_STRIP=1 \
    DEPMOD=/doesnt/exist modules_install  # Suppress depmod

  # remove build link
  rm -f "$modulesdir"/build

  echo "Installing Arch ARM specific stuff..."
  mkdir -p "${pkgdir}"/boot
  make INSTALL_DTBS_PATH="${pkgdir}/boot" dtbs_install

  if [[ $CARCH == "aarch64" ]]; then
    # drop hard-coded devicetree=foo.dtb in /boot/config.txt for
    # autodetected load of supported of models at boot
    find "${pkgdir}/boot/broadcom" -type f -print0 | xargs -0 mv -t "${pkgdir}/boot"
    rmdir "${pkgdir}/boot/broadcom"
  fi

  # remove unneeded dtb files since this package is only for RPi5
  for i in bcm2837 bcm2711 bcm2710; do
    rm "${pkgdir}/boot/$i"*.dtb
  done

  cp arch/$KARCH/boot/$_image "${pkgdir}/boot/$_kernel"
  cp arch/$KARCH/boot/dts/overlays/README "${pkgdir}/boot/overlays"
  install -m644 ../config.txt "${pkgdir}/boot/config.txt"
  install -m644 ../cmdline.txt "${pkgdir}/boot"

  # sed expression for following substitutions
  local _subst="
    s|%PKGBASE%|${pkgbase}|g
    s|%KERNVER%|$(<version)|g
  "

  # install mkinitcpio preset file
  sed "${_subst}" ../linux.preset |
    install -Dm644 /dev/stdin "${pkgdir}/etc/mkinitcpio.d/${pkgbase}.preset"

  # rather than use another hook (90-linux.hook) rely on mkinitcpio's 90-mkinitcpio-install.hook
  # which avoids a double run of mkinitcpio that can occur
  touch "${pkgdir}/usr/lib/modules/$(<version)/vmlinuz"
}

_package-headers() {
  pkgdesc="Headers and scripts for building modules for Linux kernel"
  provides=("linux-headers=${pkgver}")
  conflicts=('linux-headers' 'linux-rpi-headers')

  cd ${_srcname}
  local builddir="$pkgdir/usr/lib/modules/$(<version)/build"

  echo "Installing build files..."
  install -Dt "$builddir" -m644 .config Makefile Module.symvers System.map \
    localversion.* version
  install -Dt "$builddir/kernel" -m644 kernel/Makefile
  install -Dt "$builddir/arch/$KARCH" -m644 "arch/$KARCH/Makefile"
  cp -t "$builddir" -a scripts

  # add xfs and shmem for aufs building
  mkdir -p "$builddir"/{fs/xfs,mm}

  echo "Installing headers..."
  cp -t "$builddir" -a include
  cp -t "$builddir/arch/$KARCH" -a "arch/$KARCH/include"
  install -Dt "$builddir/arch/$KARCH/kernel" -m644 "arch/$KARCH/kernel/asm-offsets.s"

  install -Dt "$builddir/drivers/md" -m644 drivers/md/*.h
  install -Dt "$builddir/net/mac80211" -m644 net/mac80211/*.h

  # https://bugs.archlinux.org/task/13146
  install -Dt "$builddir/drivers/media/i2c" -m644 drivers/media/i2c/msp3400-driver.h

  # https://bugs.archlinux.org/task/20402
  install -Dt "$builddir/drivers/media/usb/dvb-usb" -m644 drivers/media/usb/dvb-usb/*.h
  install -Dt "$builddir/drivers/media/dvb-frontends" -m644 drivers/media/dvb-frontends/*.h
  install -Dt "$builddir/drivers/media/tuners" -m644 drivers/media/tuners/*.h

  # https://bugs.archlinux.org/task/71392
  install -Dt "$builddir/drivers/iio/common/hid-sensors" -m644 drivers/iio/common/hid-sensors/*.h

  echo "Installing KConfig files..."
  find . -name 'Kconfig*' -exec install -Dm644 {} "$builddir/{}" \;

  echo "Removing unneeded architectures..."
  local _arch
  for _arch in "$builddir"/arch/*/; do
    if [[ $CARCH == "aarch64" ]]; then
      [[ $_arch = */"$KARCH"/ || $_arch == */arm/ ]] && continue
    else
      [[ $_arch = */"$KARCH"/ ]] && continue
    fi
    echo "Removing $(basename "$_arch")"
    rm -r "$_arch"
  done

  echo "Symlinking common aliases..."
  # https://archlinuxarm.org/forum/viewtopic.php?f=60&t=16354
  ln -sr arm "$builddir/arch/armv7h"
  ln -sr arm "$builddir/arch/armv7l"
  ln -sr arm64 "$builddir/arch/aarch64"

  echo "Removing documentation..."
  rm -r "$builddir/Documentation"

  echo "Removing broken symlinks..."
  find -L "$builddir" -type l -printf 'Removing %P\n' -delete

  echo "Removing loose objects..."
  find "$builddir" -type f -name '*.o' -printf 'Removing %P\n' -delete

  echo "Stripping build tools..."
  local file
  while read -rd '' file; do
    case "$(file -Sib "$file")" in
      application/x-sharedlib\;*)      # Libraries (.so)
        strip -v $STRIP_SHARED "$file" ;;
      application/x-archive\;*)        # Libraries (.a)
        strip -v $STRIP_STATIC "$file" ;;
      application/x-executable\;*)     # Binaries
        strip -v $STRIP_BINARIES "$file" ;;
      application/x-pie-executable\;*) # Relocatable binaries
        strip -v $STRIP_SHARED "$file" ;;
    esac
  done < <(find "$builddir" -type f -perm -u+x -print0)

  echo "Adding symlink..."
  mkdir -p "$pkgdir/usr/src"
  ln -sr "$builddir" "$pkgdir/usr/src/$pkgbase"
}

pkgname=("${pkgbase}" "${pkgbase}-headers")
for _p in ${pkgname[@]}; do
  eval "package_${_p}() {
    _package${_p#${pkgbase}}
  }"
done

# vim:set ts=8 sts=2 sw=2 et:
