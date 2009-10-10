# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Contributor: William Rea <sillywilly@gmail.com>

pkgname=bhv
pkgver=0.0.10
pkgrel=2
pkgdesc="Allows you to examine the contents of web browser history files"
arch=('i686' 'x86_64')
url="http://www.lbtechservices.com/software/oss/bhv/"
license=('GPL')
depends=('gnome-python-extras' 'python-pysqlite')
makedepends=('intltool')
source=(http://www.lbtechservices.com/downloads/bhv/bhv-$pkgver.tar.gz)
md5sums=('b6997feb14f1bb74e8181a2c5f9716b0')

build() {
  cd $startdir/src/$pkgname-$pkgver

  find . -name Makefile.in -exec sed -i -e 's/-scrollkeeper-update.*//' {} \;
  if [ -f omf.make ]; then
    sed -i -e 's/-scrollkeeper-update.*//' omf.make
  fi
  ./configure --prefix=/usr
  make || return 1
  make prefix=$startdir/pkg/usr install
}
