# $Id: PKGBUILD 81227 2012-12-13 19:50:09Z mtorromeo $
# Maintainer: Massimiliano Torromeo <massimiliano.torromeo@gmail.com>
# Contributor: William J Bowman <bluephoenix47@gmail.com>

pkgname=python2-certifi
_libname=${pkgname/python2-/}
pkgver=1.0.1
pkgrel=1
pkgdesc="Mozilla's SSL Certs"
arch=(any)
url="http://pypi.python.org/pypi/certifi"
license=('GPL')
depends=('python2')
makedepends=('python2-setuptools')
source=(http://pypi.python.org/packages/source/${_libname:0:1}/$_libname/$_libname-$pkgver.tar.gz)

prepare() {
    cd "$srcdir/$_libname-$pkgver"
	sed -i '1 s|python$|python2|' certifi/core.py
}

build() {
    cd "$srcdir/$_libname-$pkgver"
	python2 setup.py build
}

package() {
    cd "$srcdir/$_libname-$pkgver"
	python2 setup.py install --skip-build -O1 --root="$pkgdir"
	install -m0644 -D "LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

sha256sums=('f2c18c7edd349ec10378ef43ff16f81ae1d0af61d5d2858a8b383e8f6814a9cc')
