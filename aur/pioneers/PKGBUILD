# Maintainer: bug <bug2000@gmail.com>
# Contributor: Lone_Wolf <lonewolf@xs4all.nl>
pkgname=pioneers
pkgver=15.3
pkgrel=1
pkgdesc="A clone of the famous Siedler of Catan game"
arch=('i686' 'x86_64')
url="http://pio.sourceforge.net/"
license=('GPL2')
depends=('gtk2')
makedepends=('pkgconfig' 'intltool' 'librsvg')
source=(http://downloads.sourceforge.net/pio/$pkgname-$pkgver.tar.gz)
md5sums=('00a50e39eb87de293843b33733d40bf9')
build() {
  cd "$srcdir/$pkgname-$pkgver"
  ./configure --prefix=/usr
  make || return 1
}
package() {
  cd "$srcdir/$pkgname-$pkgver"
  make DESTDIR="$pkgdir" install
}
