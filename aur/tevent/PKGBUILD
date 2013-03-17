# Maintainer: Christian Hesse <mail@eworm.de>
# Contributor: Marco A Rojas <marquicus at gmail dot com>
# Contributor: Ng Oon-Ee <ngoonee.talk@gmail.com>
# Contributor: Thomas Burdick <thomas.burdick@gmail.com>

pkgname=tevent
pkgver=0.9.18
pkgrel=1
pkgdesc="Tevent is an event system based on the talloc memory management library"
arch=('i686' 'x86_64')
url="https://tevent.samba.org/"
source=("http://samba.org/ftp/tevent/${pkgname}-${pkgver}.tar.gz")
license=('GPLv3')
depends=('talloc')
makedepends=('python2')

build() {
	cd ${srcdir}/${pkgname}-${pkgver}

	# change to use python2
	sed -i -e "s|/usr/bin/env python$|/usr/bin/env python2|" buildtools/bin/waf
	export PYTHON=/usr/bin/python2

	./configure --prefix=/usr
}

package() {
	cd ${srcdir}/${pkgname}-${pkgver}
	
	make
	make DESTDIR=${pkgdir}/ install
}

sha256sums=('5c636a0c55a7b59745bae0d8ae3900b5ea8c09bfff6001dcd95f1db9cd06ea4f')
