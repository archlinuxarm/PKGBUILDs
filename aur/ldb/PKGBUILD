# Maintainer: Christian Hesse <mail@eworm.de>
# Contributor: Marco A Rojas <marquicus at gmail dot com>

pkgname=ldb
pkgver=1.1.15
pkgrel=2
pkgdesc="LDAP-like embedded database, not at all LDAP standards compliant"
arch=('i686' 'x86_64')
url="http://ldb.samba.org/"
source=(http://samba.org/ftp/${pkgname}/${pkgname}-${pkgver}.tar.gz)
license=('GPLv3')
depends=('talloc' 'tevent' 'tdb')

build() {
	cd ${srcdir}/${pkgname}-${pkgver}

	# change to use python2
	sed -i -e "s|/usr/bin/env python$|/usr/bin/env python2|" buildtools/bin/waf
	export PYTHON=/usr/bin/python2

	./configure --prefix=/usr \
		--disable-rpath \
		--disable-rpath-install \
		--bundled-libraries=NONE \
		--builtin-libraries=NONE
	make
}

package() {
	cd ${srcdir}/${pkgname}-${pkgver}

	make DESTDIR=${pkgdir}/ install
}

sha256sums=('6bd8317e82747461394ab8ad1ee5873589d9a46d12f021571aca9fac45de8997')
