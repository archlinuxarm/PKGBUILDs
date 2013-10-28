# Maintainer: Jaroslav Lichtblau <dragonlord@aur.archlinux.org>
# Contributor: Lothar Gesslein <ulmen@cryptomilch.de>

pkgname=aoetools
pkgver=36
pkgrel=1
pkgdesc="ATA over Ethernet Tools"
arch=('i686' 'x86_64')
url="http://aoetools.sourceforge.net/"
license=('GPL')
source=(http://downloads.sourceforge.net/$pkgname/$pkgname-$pkgver.tar.gz)
sha256sums=('fb5e2cd0de7644cc1ec04ee3aeb43211cf7445a0c19e13d6b3ed5a8fbdf215ff')

build() {
  cd ${srcdir}/$pkgname-$pkgver

  make
}

package() {
  cd ${srcdir}/$pkgname-$pkgver

  make DESTDIR=${pkgdir} install
}
