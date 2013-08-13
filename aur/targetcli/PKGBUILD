# Contributor: Johannes Dewender  arch at JonnyJD dot net
pkgname=targetcli
pkgver=2.1
pkgrel=1
epoch=
pkgdesc="The targetcli administration shell."
arch=('any')
url="http://linux-iscsi.org/wiki/RTSadmin"
license=('Apache')
groups=()
depends=('python2-rtslib' 'lio-utils' 'python2-configshell')
optdepends=()
replaces=('rtsadmin')
backup=()
options=()
install=
source=()
md5sums=()

build() {
  if [ ! -e "$pkgname-$pkgver.tgz" ]; then
    curl -fLC - "http://www.risingtidesystems.com/git/?p=$pkgname.git;a=snapshot;h=95cdcb7dd669d840d14bd099d68fce4dcdc9cc64;sf=tgz" -o "$pkgname-$pkgver.tgz"
  fi
  bsdtar -xf "$srcdir/$pkgname-$pkgver.tgz"
  cd "$srcdir/$pkgname"
  python2 setup.py build
}

package() {
  cd "$srcdir/$pkgname"
  python2 setup.py install --skip-build --root="$pkgdir/" --optimize=1
}

# vim:set ts=2 sw=2 et:
