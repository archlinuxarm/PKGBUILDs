# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Andrea Scarpino <andrea@archlinux.org>
# Contributor: Mateusz Herych <heniekk@gmail.com>

pkgname=libgadu
pkgver=1.8.2
pkgrel=3
pkgdesc="Gadu-Gadu protocol libraries"
arch=('i686' 'x86_64')
url="http://toxygen.net/libgadu/"
license=('GPL')
depends=('openssl')
source=(http://toxygen.net/libgadu/files/$pkgname-$pkgver.tar.gz)
md5sums=('1090f82f8a1bb99e9cdf5853188f625f')
options=('!libtool')

build() {
  cd ${srcdir}/$pkgname-$pkgver
  ./configure --prefix=/usr \
 	 --disable-static \
	 --enable-shared \
	 --with-pthread
  make || return 1
  make DESTDIR=${pkgdir} install || return 1
}
