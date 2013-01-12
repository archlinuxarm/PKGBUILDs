pkgname=python2-requests-0.14
pkgver=0.14.2
pkgrel=2
pkgdesc="Python HTTP for Humans."
url="http://python-requests.org"
depends=('python2' )
makedepends=('python2-distribute' )
optdepends=('python2-certifi: SSL support')
license=('custom: ISC')
arch=('any')
source=("http://pypi.python.org/packages/source/r/requests/requests-$pkgver.tar.gz")
conflicts=('python2-requests')
build() {
    cd $srcdir/requests-$pkgver
    python2 setup.py build
}

package() {
    cd $srcdir/requests-$pkgver
    python2 setup.py install --root="$pkgdir"
}


md5sums=('488508ba3e8270992ad5b3fb54d364ca')
