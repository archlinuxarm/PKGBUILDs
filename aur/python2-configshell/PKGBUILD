# Contributor: Johannes Dewender  arch at JonnyJD dot net 
pkgname=python2-configshell
pkgver=1.2
pkgrel=1
epoch=
pkgdesc="python framework for building simple CLI-based applications"
arch=('any')
url="http://www.risingtidesystems.com/git/?p=configshell.git;a=summary"
license=('Apache')
depends=('python2-simpleparse' 'python2-urwid' 'epydoc')
provides=()
conflicts=()
options=()
source=()
md5sums=()

_pkgname=configshell

build() {
  if [ ! -e "$_pkgname-$pkgver.tgz" ]; then
    curl -flC - "http://www.risingtidesystems.com/git/?p=$_pkgname.git;a=snapshot;h=5fcb00e77153b6a9e188c00a06d88783e7d273f7;sf=tgz" -o "$_pkgname-$pkgver.tgz"
  fi
  bsdtar -xf "$srcdir/$_pkgname-$pkgver.tgz"
  cd "$srcdir/$_pkgname"
  python2 setup.py build
}

package() {
  cd "$srcdir/$_pkgname"
  python2 setup.py install --skip-build --root="$pkgdir/" --optimize=1
}

# vim:set ts=2 sw=2 et:
