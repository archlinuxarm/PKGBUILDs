# U-Boot: Rock64
# Maintainer: Kevin Mihelich 
# Maintainer: Adam <adam900710@gmail.com>

buildarch=8

pkgname=uboot-rockpro64
pkgver=2019.04
pkgrel=1
pkgdesc="U-Boot for RockPro64"
arch=('aarch64')
url='http://www.denx.de/wiki/U-Boot/WebHome'
license=('GPL')
backup=('boot/boot.txt' 'boot/boot.scr')
makedepends=('bc' 'git' 'rockchip-tools')
install=${pkgname}.install
_commit_rkbin=587600675cb8cc77d559e9c506d250ce17d9e2b1
source=("ftp://ftp.denx.de/pub/u-boot/u-boot-${pkgver}.tar.bz2"
        "git+https://github.com/rockchip-linux/rkbin.git#commit=$_commit_rkbin"
        'rk3399trust.ini'
        'boot.txt'
        'mkscr'
	'0001-board-rockchip-rk3399-add-Rockpro64-board-support.patch')
sha256sums=('76b7772d156b3ddd7644c8a1736081e55b78828537ff714065d21dbade229bef'
	    'SKIP'
	    'dd5c56c6957f670e7e52d145aa66238b17a54edd87b787bfda050abb1d22d2db'
	    '5698bcfbf2fca73daff77d1ca490ce187b65a8b574c7500ea63a9ffd0e6d28e1'
	    'a4fc8b6b92bc364d6542670d294aa618a8501fb8729f415cc0a3eed776ef0c8e'
	    '2c0ac7700c589d030aa810bff1c9dbea2c71d4a8e09d03632a6627b59464ff35')

build() {
  cd u-boot-${pkgver}

  unset CLFAGS CXXFLAGS CPPFLAGS LDFLAGS

  patch -Np1 -i ../0001-board-rockchip-rk3399-add-Rockpro64-board-support.patch
  make rockpro64-rk3399_defconfig
  echo 'CONFIG_IDENT_STRING=" Arch Linux ARM"' >> .config
  make EXTRAVERSION=-${pkgrel}
}

package() {
  cd u-boot-${pkgver}

  mkdir -p "${pkgdir}/boot"

  tools/mkimage -n rk3399 -T rksd -d ../rkbin/bin/rk33/rk3399_ddr_800MHz_v1.20.bin "${pkgdir}/boot/idbloader.img"
  cat ../rkbin/bin/rk33/rk3399_miniloader_v1.19.bin >> "${pkgdir}/boot/idbloader.img"

  loaderimage --pack --uboot u-boot-dtb.bin "${pkgdir}/boot/uboot.img" 0x200000

  trust_merger ../rk3399trust.ini

  cp trust.img "${pkgdir}/boot"

  tools/mkimage -A arm -O linux -T script -C none -n "U-Boot boot script" -d ../boot.txt "${pkgdir}/boot/boot.scr"
  cp ../{boot.txt,mkscr} "${pkgdir}"/boot
}
