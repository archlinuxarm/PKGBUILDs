# Maintainer: Milo Mirate <mmirate@gmx.com>
_pkgname=plumbum
pkgname=python2-plumbum
pkgver=1.4.2
pkgrel=1
pkgdesc="Shell combinators library."
arch=('any')
url="http://pypi.python.org/pypi/plumbum"
license=('GPL')
groups=()
depends=('python2' 'python2-six')
makedepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=(!emptydirs)
install=
source=("http://pypi.python.org/packages/source/p/${_pkgname}/${_pkgname}-${pkgver}.tar.gz")
md5sums=('38b526af9012a5282ae91dfe372cefd3')

package() {
  cd "$srcdir/${_pkgname}-$pkgver"
  python2 setup.py install --root="$pkgdir/" --optimize=1
}

# vim:set ts=2 sw=2 et:
