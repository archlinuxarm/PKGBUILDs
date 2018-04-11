# Maintainer: Spyros Stathopoulos <spystath@gmail.com>

pkgname=owfs
pkgver=3.2p1
pkgrel=1
pkgdesc="OWFS is an easy way to use the powerful 1-wire system of Dallas/Maxim."
arch=('i686' 'x86_64')
url="http://www.owfs.org/"
license=('GPL')
source=("http://downloads.sourceforge.net/project/${pkgname}/${pkgname}/${pkgver}/${pkgname}-${pkgver}.tar.gz"
        "systemd_journal.patch"
        "owfs.conf")
depends=('avahi' 'fuse' 'perl')
makedepends=('swig')
sha1sums=('4ee76e686bec769acde5bfbda148a8693df244c6'
          'ff13b0a3d56206e32cf0307042351298e3f27d2d'
          '99a72935a8d2bdcd79a745983cda85fd5d3035f4')

prepare() {
  cd "${srcdir}/${pkgname}-${pkgver}"
  sed -i -e 's|<libusb.h>|<libusb-1.0/libusb.h>|' "module/owlib/src/include/ow.h"
  patch -p1 < ../systemd_journal.patch
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

  install -Dm644 "${srcdir}/owfs.conf" "${pkgdir}/usr/lib/sysusers.d/owfs.conf"
}

# vim:ts=2:sw=2:et
