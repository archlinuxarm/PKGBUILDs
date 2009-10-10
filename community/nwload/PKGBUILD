# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Sergej Pupykin <pupykin.s+arch@gmail.com>
# Contributor: Sergej Pupykin <pupykin.s+arch@gmail.com>

pkgname=nwload
pkgver=0.2e
pkgrel=2
pkgdesc="network load monitor"
arch=('i686' 'x86_64')
url="http://www.hczim.de/software/nwload.html"
license=('GPL')
depends=(python tk)
source=(http://www.hczim.de/software/$pkgname-$pkgver.tar.gz)
md5sums=('058a76887431efe3c3d12c5682a7226d')

build() {
  cd $startdir/src/$pkgname-$pkgver
  make || return 1
  install -D -m0755 nwload $startdir/pkg/usr/bin/nwload && \
  install -D -m0644 nwload.1 $startdir/pkg/usr/man/man1/nwload.1
}
