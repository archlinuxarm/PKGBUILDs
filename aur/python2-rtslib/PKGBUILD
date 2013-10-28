# Contributor: Johannes Dewender  arch at JonnyJD dot net
pkgname=python2-rtslib
pkgver=2.2
pkgrel=2
epoch=
pkgdesc="RisingTide Systems generic SCSI target API in python"
arch=('any')
url="http://www.risingtidesystems.com/git/?p=rtslib.git;a=summary"
license=('Apache')
depends=('python2-ipaddr' 'python2-netifaces' 'python2-configobj')
provides=()
conflicts=()
backup=()
options=()
install=

_pkgname=rtslib

build() {
  if [ ! -e "$_pkgname-$pkgver.tgz" ]; then
    curl -fLC - "http://www.risingtidesystems.com/git/?p=$_pkgname.git;a=snapshot;h=909a40c908247135f30b27eb5d69b292aeedd0b4;sf=tgz" -o "$_pkgname-$pkgver.tgz"
  fi
  bsdtar -xf "$srcdir/$_pkgname-$pkgver.tgz"

  cd "$srcdir/$_pkgname"
  python2 setup.py build
}

package() {
  cd "$srcdir/$_pkgname"
  python2 setup.py install --skip-build --root="$pkgdir/" --optimize=1

  install -d "$pkgdir/var/target/fabric"
  for file in specs/*; do
    install -m 644 "$file" "$pkgdir/var/target/fabric/"
  done
}

# vim:set ts=2 sw=2 et:
