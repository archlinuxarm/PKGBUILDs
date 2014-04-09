# Maintainer: Brice Waegeneire < brice dot wge at gmail dot com >
# Contributor: Alexander Lam <lambchop468 *AT* gmail.com>

pkgname=devmem
pkgver=2
pkgrel=2
pkgdesc="A small utility to access /dev/mem and read/write to any memory location"
arch=('any')
url="http://free-electrons.com/pub/mirror/devmem2.c"
license=('GPL2')
source=("http://free-electrons.com/pub/mirror/${pkgname}${pkgver}.c")

sha256sums=('3b15515693bae1ebd14d914e46d388edfec2175829ea1576a7a0c8606ebbe639')

build() {
  cd "$srcdir/"
  gcc devmem2.c -o devmem2
}

package() {
  install -D -m755 ${srcdir}/devmem2 ${pkgdir}/usr/bin/devmem2
}
  
# vim:set ts=2 sw=2 et:
