# Maintainer: Geoffroy Carrier <geoffroy@archlinux.org>
pkgname=luarocks
_rver=3981
pkgver=1.0.1
pkgrel=1
pkgdesc='Deployment and management system for Lua modules'
arch=('i686' 'x86_64')
url="http://luarocks.org/"
depends=('lua')
license=('custom')
source=(http://luaforge.net/frs/download.php/$_rver/$pkgname-$pkgver.tar.gz)
build() {
  cd "$srcdir/$pkgname-$pkgver"
  ./configure --prefix=/usr --sysconfdir=/etc/luarocks || return 1
  make || return 1
  make install DESTDIR="$pkgdir" || return 1
  install -D COPYING "$pkgdir/usr/share/licenses/$pkgname/COPYING"
}
md5sums=('e6fad9ddecf79808fda7fd257bfbde06')
