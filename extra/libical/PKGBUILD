# $Id: $
# Maintainer: Andrea Scarpino <andrea@archlinux.org>
# Contributor: Pierre Schmitz <pierre@archlinux.de>

pkgname=libical
pkgver=0.44
pkgrel=1
pkgdesc="An open source reference implementation of the icalendar data type and serialization format"
arch=('i686' 'x86_64')
url='http://sourceforge.net/projects/freeassociation/'
license=('LGPL' 'MPL')
depends=('glibc')
options=('!libtool')
source=("http://downloads.sourceforge.net/freeassociation/${pkgname}-${pkgver}.tar.gz")
md5sums=('e0403c31e1ed82569325685f8c15959c')

build() {
  cd ${srcdir}/${pkgname}-${pkgver}
  ./configure --prefix=/usr \
    --enable-shared \
    --disable-static
  make || return 1
  make DESTDIR=${pkgdir} install
}
