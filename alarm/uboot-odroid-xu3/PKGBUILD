# U-Boot: ODROID XU3
# Maintainer: Kevin Mihelich <kevin@archlinuxarm.org>

buildarch=4

pkgname=uboot-odroid-xu3
pkgver=2016.11
pkgrel=1
pkgdesc="U-Boot for ODROID-XU3"
arch=('armv7h')
url='http://www.denx.de/wiki/U-Boot/WebHome'
license=('GPL')
install=$pkgname.install
backup=('boot/boot.txt' 'boot/boot.scr')
makedepends=('bc' 'dtc' 'git')
_commit=fe2f831fd44a4071f58a42f260164544697aa666
source=("ftp://ftp.denx.de/pub/u-boot/u-boot-${pkgver}.tar.bz2"
        "bl1.bin::https://github.com/hardkernel/u-boot/raw/${_commit}/sd_fuse/hardkernel/bl1.bin.hardkernel"
        'http://archlinuxarm.org/builder/src/xu3/bl2.bin'
        "tzsw.bin::https://github.com/hardkernel/u-boot/raw/${_commit}/sd_fuse/hardkernel/tzsw.bin.hardkernel"
        '0001-arch-linux-arm-modifications.patch'
        'sd_fusing.sh'
        'boot.txt'
        'mkscr')
md5sums=('ca1f6e019d08aff8d0ca1beb2e66737d'
         '38fb058aa3bcc568f9547c85517949b9'
         '09c42bed980921cfc914e97e067ba9a3'
         'fd01dda20b999e0b731c7063431a42b3'
         '683f66010666783d086af222c052d57e'
         '8a31acf5da5722698f54d1fe15c482bb'
         '52306aa4cf2c3499ecfcea026fb2741c'
         '021623a04afd29ac3f368977140cfbfd')

prepare() {
  cd u-boot-${pkgver}

  git apply ../0001-arch-linux-arm-modifications.patch
}

build() {
  cd u-boot-${pkgver}

  unset CFLAGS CXXFLAGS CPPFLAGS

  make distclean
  make odroid-xu3_config
  make EXTRAVERSION=-${pkgrel}
}

package() {
  cd u-boot-${pkgver}

  mkdir -p "${pkgdir}"/boot

  cp u-boot-dtb.bin ${pkgdir}/boot/u-boot.bin
  cp ../{{bl{1,2},tzsw}.bin,sd_fusing.sh} "${pkgdir}"/boot
  chmod +x "${pkgdir}"/boot/sd_fusing.sh

  tools/mkimage -A arm -O linux -T script -C none -n "U-Boot boot script" -d ../boot.txt "${pkgdir}"/boot/boot.scr
  cp ../{boot.txt,mkscr} "${pkgdir}"/boot
}
