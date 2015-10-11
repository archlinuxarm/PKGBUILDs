# Maintainer: Spyros Stathopoulos <spystath@gmail.com>

pkgname=owfs
pkgver=3.1p0
pkgrel=2
pkgdesc="OWFS is an easy way to use the powerful 1-wire system of Dallas/Maxim."
arch=('i686' 'x86_64')
url="http://www.owfs.org/"
license=('GPL')
source=("http://downloads.sourceforge.net/project/${pkgname}/${pkgname}/${pkgver}/${pkgname}-${pkgver}.tar.gz")
depends=('avahi' 'fuse' 'perl')
makedepends=('swig')
sha1sums=('f82bf91e4f78a667c09a44d486139f1359f0950c')

prepare() {
  cd "${srcdir}/${pkgname}-${pkgver}"
   sed -i -e 's|<libusb.h>|<libusb-1.0/libusb.h>|' "module/owlib/src/include/ow.h"
}

build() {
  cd "${srcdir}/${pkgname}-${pkgver}"
  ./configure --prefix=/usr \
              --libexecdir=/usr/lib \
              --disable-owtcl \
              --disable-owphp \
              --disable-owpython \
              --disable-zero \
              --with-systemdunitdir=/usr/lib/systemd/system

  make
}

package() {
  cd "${srcdir}/${pkgname}-${pkgver}"

  make DESTDIR="${pkgdir}" install
}

# vim:ts=2:sw=2:et
