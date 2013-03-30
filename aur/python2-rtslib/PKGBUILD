# Contributor: Johannes Dewender  arch at JonnyJD dot net
pkgname=python2-rtslib
pkgver=2.1
pkgrel=5
epoch=
pkgdesc="RisingTide Systems generic SCSI target API in python"
arch=('any')
url="http://www.risingtidesystems.com/git/?p=rtslib.git;a=summary"
license=('AGPL3')
depends=('python2-ipaddr' 'python2-netifaces' 'python2-configobj')
provides=()
conflicts=()
backup=()
options=()
install=
source=('gz-modules-upstream.patch')
md5sums=('4add05b9cbd12258dc993f19728fbafe')

_pkgname=rtslib

build() {
  if [ ! -e "$_pkgname-$pkgver.tgz" ]; then
    curl -fLC - "http://www.risingtidesystems.com/git/?p=$_pkgname.git;a=snapshot;h=c4ba14ff21f68e56d6a3a227a0220d1a6b6df958;sf=tgz" -o "$_pkgname-$pkgver.tgz"
  fi
  bsdtar -xf "$srcdir/$_pkgname-$pkgver.tgz"

  cd "$srcdir/$_pkgname"
  patch -p1 < ../gz-modules-upstream.patch
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
