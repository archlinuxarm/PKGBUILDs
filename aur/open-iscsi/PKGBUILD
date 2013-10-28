# Maintainer: Stefan Kirrmann <stefan.kirrmann at gmail dot com>
pkgname=open-iscsi
pkgver=2.0.873
_pkgver=2.0-873
pkgrel=5
pkgdesc="userland tools"
arch=('i686' 'x86_64')
url="http://www.open-iscsi.org"
license=('GPL')
depends=('bash')
install=$pkgname.install
backup=('etc/iscsi/iscsid.conf' 'etc/iscsi/initiatorname.iscsi')
source=("http://www.open-iscsi.org/bits/${pkgname}-${_pkgver}.tar.gz" \
        "open-iscsi.service")
options=('docs')
sha256sums=('7dd9f2f97da417560349a8da44ea4fcfe98bfd5ef284240a2cc4ff8e88ac7cd9'
            '7b8e37dd10a909a67ba7f7126f699920639be39adfa65f1d2b2bcd8846e58db7')

build() {
  cd ${srcdir}/${pkgname}-${_pkgver}

  # include iscsistart in the package
  sed -i -e '/^PROGRAMS = /s/$/ usr\/iscsistart/' Makefile

  # build breaks if the openslp package is installed
  sed -i -e 's/\(\.\/configure\)/ \1 --without-slp/g' Makefile 

  make user
}

package() {
  cd ${srcdir}/${pkgname}-${_pkgver}

  make DESTDIR=${pkgdir} install_user
  
  install -D -m644 ${srcdir}/${pkgname}-${_pkgver}/etc/iscsid.conf ${pkgdir}/etc/iscsi
  install -D -m644 ${srcdir}/open-iscsi.service ${pkgdir}/usr/lib/systemd/system/open-iscsi.service

  touch ${pkgdir}/etc/iscsi/initiatorname.iscsi
  
  # copy docs
  mkdir -p ${pkgdir}/usr/share/doc/${pkgname}
  install -m644 Changelog ${pkgdir}/usr/share/doc/${pkgname}/
  install -m644 README ${pkgdir}/usr/share/doc/${pkgname}/
}
