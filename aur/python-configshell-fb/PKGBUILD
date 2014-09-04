# Contributor: Johannes Dewender  arch at JonnyJD dot net 
pkgname=python-configshell-fb
_pkgname=configshell-fb
pkgver=1.1.fb14
pkgrel=1
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
sha256sums=('280b6a1361a0d27804e281d54d5513f7dc2258a7a8404ebd9ae53f97ae992d26')

build() {
  cd "$srcdir/$_pkgname-$pkgver"
  python setup.py build
}

package() {
  cd "$srcdir/$_pkgname-$pkgver"
  python setup.py install --skip-build --root="$pkgdir/" --optimize=1
}

# vim:set ts=2 sw=2 et:
