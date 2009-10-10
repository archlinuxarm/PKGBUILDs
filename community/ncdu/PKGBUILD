# Contributor: lp76 <l.peduto@gmail.com>
# Maintainer: Daenyth <Daenyth+Arch AT gmail DOT com>
pkgname=ncdu
pkgver=1.5
pkgrel=1
pkgdesc="An NCurses version of the famous old 'du' unix command"
arch=('i686' 'x86_64')
url="http://dev.yorhel.nl/ncdu/"
license=('MIT')
depends=('ncurses')
source=(http://dev.yorhel.nl/download/$pkgname-$pkgver.tar.gz)
md5sums=('90a69cc3b2e9f0324eb14e6ce1df0f22')

build() {
  cd $srcdir/$pkgname-$pkgver
  ./configure --prefix=/usr
  make || return 1
  make DESTDIR=$pkgdir install || return 1
  install -Dm644 COPYING $pkgdir/usr/share/licenses/$pkgname/LICENSE
}
