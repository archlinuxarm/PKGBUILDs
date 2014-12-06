# Contributor: Johannes Dewender  arch at JonnyJD dot net 
pkgname=('python-configshell-fb' 'python2-configshell-fb')
_pkgname=configshell-fb
pkgver=1.1.fb16
pkgrel=2
pkgdesc="python framework for building simple CLI applications (free branch)"
arch=('any')
url="https://github.com/agrover/configshell-fb"
license=('Apache')
options=()
makedepends=('python-setuptools' 'python2-setuptools')
source=(https://fedorahosted.org/releases/t/a/targetcli-fb/$_pkgname-$pkgver.tar.gz)
sha256sums=('391187e213bcec4eb81d34baad973afd6ff544af1fe07816ff784a3eb278e0c5')

package_python-configshell-fb() {
  depends=('python-pyparsing' 'python-urwid')
  provides=('python-configshell')
  conflicts=('python-configshell')
  cd "$srcdir/$_pkgname-$pkgver"
  python setup.py install --root="$pkgdir/" --optimize=1
}

package_python2-configshell-fb() {
  depends=('python2-pyparsing' 'python2-urwid')
  provides=('python2-configshell')
  conflicts=('python2-configshell')
  cd "$srcdir/$_pkgname-$pkgver"
  python2 setup.py install --root="$pkgdir/" --optimize=1
}

# vim:set ts=2 sw=2 et:
