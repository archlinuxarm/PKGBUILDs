# Maintainer: Étienne Deparis <etienne [at] depar.is>

pkgname=python2-exconsole
pkgver=0.1.5
pkgrel=1
pkgdesc="Emergency/postmortem Python console"
license=("PSF")
url="https://pypi.python.org/pypi/python-exconsole"
makedepends=('python2-setuptools')
source=(http://pypi.python.org/packages/source/p/python-exconsole/python-exconsole-$pkgver.tar.gz)
md5sums=('595c7fd980dd425c541be960e7fd3eee')
arch=('any')
options=(!emptydirs)

package() {
  cd $srcdir/python-exconsole-$pkgver

  find . -type f -exec sed -i \
    -e'1s|^#!/usr/bin/env python$|#!/usr/bin/env python2|' \
    -e '1s|^#!/usr/bin/python$|#!/usr/bin/env python2|' \
    "{}" \;

  python2 setup.py install --root=$pkgdir
}

