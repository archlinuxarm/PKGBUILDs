# U-Boot: ODROID XU3
# Maintainer: Kevin Mihelich <kevin@archlinuxarm.org>

buildarch=4

pkgname=uboot-odroid-xu3
pkgver=2012.07
pkgrel=1
pkgdesc="U-Boot for ODROID-XU3"
arch=('armv7h')
url="https://github.com/hardkernel/linux/tree/odroidxu3-3.10.y"
license=('GPL')
_commit=814386d3e43b8ab8d81f04aa7fe402952503d8fe
source=("https://github.com/hardkernel/linux/raw/${_commit}/tools/hardkernel/prebuilt_uboot/bl1.bin"
        "https://github.com/hardkernel/linux/raw/${_commit}/tools/hardkernel/prebuilt_uboot/bl2.bin"
        "https://github.com/hardkernel/linux/raw/${_commit}/tools/hardkernel/prebuilt_uboot/tzsw.bin"
        "https://github.com/hardkernel/linux/raw/${_commit}/tools/hardkernel/prebuilt_uboot/u-boot.bin"
        "https://github.com/hardkernel/linux/raw/${_commit}/tools/hardkernel/prebuilt_uboot/sd_fusing.sh")
md5sums=('3908379f82f972ece88ca1b5a280b5fd'
         '8580c2b2701bf5936f42104b3cc5d725'
         'fd01dda20b999e0b731c7063431a42b3'
         '85ff8b8e20fe4c7639ed87781f473512'
         '916a38e34d6f678083902f1a601467f0')

package() {
  mkdir -p "${pkgdir}"/boot
  cp {bl{1,2},tzsw,u-boot}.bin sd_fusing.sh "${pkgdir}"/boot
}
