# Maintainer: Santi Villalba <sdvillal@gmail.com>
pkgname=python2-guessit
pkgver=0.10.2
pkgrel=1
pkgdesc="A library for guessing information from video files."
arch=(any)
url="http://pypi.python.org/pypi/guessit"
license=(LGPL)
depends=(python2 python2-babelfish python2-stevedore)
makedepends=(python2-distribute)
conflicts=(${pkgname}-git)
source=(http://pypi.python.org/packages/source/g/guessit/guessit-${pkgver}.tar.gz)
md5sums=('e4b8ad0f74fdd18e2264532051cdf54d')

package() {
  cd "${srcdir}/guessit-${pkgver}"
  python2 setup.py install --root="$pkgdir/" --optimize=1
}
