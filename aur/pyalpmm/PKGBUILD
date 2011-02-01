# Contributor: Andrea Scarpino <andrea@archlinux.org>

pkgname=pyalpmm
pkgver=0.3
pkgrel=1
pkgdesc="The Python ALPM high-level API"
arch=('i686' 'x86_64')
license=('GPL')
url="http://www.infolexikon.de/code/pyalpmm"
depends=('python>=2.6.2')
makedepends=('swig')
source=("http://static.evigo.net/infolexikon/releases/${pkgname}-${pkgver}.tgz")
md5sums=('8eed9a315eff7c444ce1501db47e748e')

build() {
  cd ${srcdir}/${pkgname}-${pkgver}
  make || return 1
  make install DESTDIR=${pkgdir} || return 1
  install -Dm755 mmacman ${pkgdir}/usr/bin/mmacman || return 1
  install -Dm644 pyalpmm.conf ${pkgdir}/etc/pyalpmm.conf || return 1
}

