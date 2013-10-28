# Contributor: Johannes Dewender  arch at JonnyJD dot net
pkgname=targetcli
pkgver=2.1
pkgrel=2
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
    curl -fLC - "http://www.risingtidesystems.com/git/?p=$pkgname.git;a=snapshot;h=a7c313ce9810a0b178e0e8cfaf5ffc22d8868e30;sf=tgz" -o "$pkgname-$pkgver.tgz"
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
