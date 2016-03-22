# Maintainer: Étienne Deparis <etienne [at] depar.is>

pkgname=python2-catcher
pkgver=0.1.7
pkgrel=1
pkgdesc="Beautiful stack traces for Python"
license=("PSF")
url="https://pypi.python.org/pypi/python-catcher"
depends=('python2-requests' 'python2-mako')
makedepends=('python2-setuptools')
source=(http://pypi.python.org/packages/source/p/python-catcher/python-catcher-$pkgver.tar.gz)
arch=('any')
options=(!emptydirs)

package() {
  cd $srcdir/python-catcher-$pkgver

  find . -type f -exec sed -i \
    -e'1s|^#!/usr/bin/env python$|#!/usr/bin/env python2|' \
    -e '1s|^#!/usr/bin/python$|#!/usr/bin/env python2|' \
    "{}" \;

  python2 setup.py install --root=$pkgdir
}

md5sums=('eecb82153f46586841162613008bab47')
