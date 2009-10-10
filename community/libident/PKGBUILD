# Maintainer: Mateusz Herych <heniekk@gmail.com>
pkgname=libident
pkgver=0.32
pkgrel=1
pkgdesc="New libident C library"
arch=('i686' 'x86_64')
url="http://www.remlab.net/libident/"
license=('custom')
source=(http://www.remlab.net/files/libident/libident-$pkgver.tar.gz)
md5sums=('9b9346eacc28d842d164881f0efa3388')

build() {
	cd $startdir/src/$pkgname-$pkgver
	./configure --prefix=/usr \
		--sysconfdir=/etc \
		--mandir=/usr/share/man
	make || return 1
	make DESTDIR=$startdir/pkg install || return 1
	install -D -m 755 COPYING $startdir/pkg/usr/share/licenses/$pkgname/COPYING
}
