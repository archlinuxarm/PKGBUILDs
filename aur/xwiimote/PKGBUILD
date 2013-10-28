# Maintainer: David Herrmann <dh.herrmann@googlemail.com>
pkgname=xwiimote
pkgver=2
pkgrel=1
pkgdesc='Userspace utilities to control connected Nintendo Wii Remotes'
arch=('i686' 'x86_64')
url="http://dvdhrm.github.io/$pkgname"
license=('MIT')
depends=('udev' 'ncurses')
options=(!libtool)
source=(https://github.com/dvdhrm/xwiimote/releases/download/$pkgname-$pkgver/$pkgname-$pkgver.tar.xz)
#source=($pkgname-$pkgver.tar.xz)
md5sums=('763f2c1acedfea9c8cee448a674efb76')

build() {
  cd "$srcdir/$pkgname-$pkgver"
  ./configure --prefix=/usr --with-doxygen=no
  make
}

package() {
  cd "$srcdir/$pkgname-$pkgver"
  mkdir -p "$pkgdir/usr/share/licenses/$pkgname/"
  cp LICENSE "$pkgdir/usr/share/licenses/$pkgname/"
  mkdir -p "$pkgdir/etc/X11/xorg.conf.d/"
  cp "res/50-xorg-fix-xwiimote.conf" "$pkgdir/etc/X11/xorg.conf.d/50-fix-xwiimote.conf"
  make DESTDIR="$pkgdir/" install
}
