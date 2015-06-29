# Maintainer: Santi Villalba <sdvillal@gmail.com>
pkgname=python2-guessit
pkgver=0.10.3
pkgrel=1
pkgdesc="A library for guessing information from video files."
arch=(any)
url="http://pypi.python.org/pypi/guessit"
license=(LGPL)
depends=(python2 python2-babelfish python2-stevedore)
makedepends=(python2-distribute)
conflicts=(${pkgname}-git)
source=(http://pypi.python.org/packages/source/g/guessit/guessit-${pkgver}.tar.gz)
md5sums=('11e658f9b0473d61d681579c03c196c3')

package() {
  cd "${srcdir}/guessit-${pkgver}"
  python2 setup.py install --root="$pkgdir/" --optimize=1
}
