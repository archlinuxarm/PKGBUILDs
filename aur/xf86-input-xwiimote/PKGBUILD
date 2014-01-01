# Maintainer: David Herrmann <dh.herrmann@googlemail.com>
pkgname=xf86-input-xwiimote
pkgver=0.5
pkgrel=1
pkgdesc='X.Org Nintendo Wii Remote input driver'
arch=('i686' 'x86_64')
url="http://github.com/dvdhrm/$pkgname"
license=('MIT')
depends=('xwiimote>=2' 'udev')
makedepends=('xorg-server-devel>=1.10.99.902')
conflicts=('xorg-server<1.10.99.902')
options=(!libtool)
source=(https://github.com/dvdhrm/$pkgname/releases/download/$pkgname-$pkgver/$pkgname-$pkgver.tar.xz)
#source=($pkgname-$pkgver.tar.xz)
md5sums=('8211779b393768cb4729e74358258c95')

build() {
  cd "$srcdir/$pkgname-$pkgver"
  ./configure --prefix=/usr
  make
}

package() {
  cd "$srcdir/$pkgname-$pkgver"
  mkdir -p "$pkgdir/usr/share/licenses/$pkgname/"
  cp COPYING "$pkgdir/usr/share/licenses/$pkgname/"
  mkdir -p "$pkgdir/etc/X11/xorg.conf.d/"
  cp "60-xorg-xwiimote.conf" "$pkgdir/etc/X11/xorg.conf.d/60-xwiimote.conf"
  make DESTDIR="$pkgdir/" install
}
