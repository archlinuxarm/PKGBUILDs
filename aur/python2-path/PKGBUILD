# Contributor: Anxo Beltrán <anxo.beltran.alvarez@gmail.com>
# Contributor: David Danier <david.danier@team23.de>
pkgname=python2-path
pkgver=5.1
pkgrel=2
pkgdesc="path.py provides a class (path) for working with files and directories. Less typing than os.path, more fun, a few new tricks"
arch=('any')
url="https://pypi.python.org/pypi/path.py"
license=('MIT')
depends=('python2')
options=(!emptydirs)
source=(http://pypi.python.org/packages/source/p/path.py/path.py-${pkgver}.zip)
sha256sums=('976b1392527c77383eb827de7fd44dacaf1297a63aa0df526f47af302f479d54')

package() {
  cd ${srcdir}/path.py-${pkgver}
  # python2 setup.py build || return 1
  python2 setup.py install --root=${pkgdir} --optimize=1 || return 1
}
