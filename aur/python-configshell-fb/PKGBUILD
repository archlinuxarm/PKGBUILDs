# Contributor: Johannes Dewender  arch at JonnyJD dot net 
pkgname=python-configshell-fb
_pkgname=configshell-fb
pkgver=1.1.fb13
pkgrel=1
epoch=
pkgdesc="python framework for building simple CLI applications (free branch)"
arch=('any')
url="https://github.com/agrover/configshell-fb"
license=('Apache')
depends=('python-pyparsing' 'python-urwid')
makedepends=('python-setuptools')
provides=('python-configshell')
conflicts=('python-configshell')
options=()
source=(https://fedorahosted.org/releases/t/a/targetcli-fb/$_pkgname-$pkgver.tar.gz)
md5sums=('a10cdd7bc418bb86c71aebff112bc63a')

build() {
  cd "$srcdir/$_pkgname-$pkgver"
  python setup.py build
}

package() {
  cd "$srcdir/$_pkgname-$pkgver"
  python setup.py install --skip-build --root="$pkgdir/" --optimize=1
}

# vim:set ts=2 sw=2 et:
