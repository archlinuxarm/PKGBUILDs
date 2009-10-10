# Contributor: Laurent Meunier <laurent@deltalima.net>
# Maintainer: Daniel J Griffiths <ghost1227@archlinux.us>

pkgname=goocanvasmm
pkgver=0.14.0
pkgrel=1
pkgdesc="C++ wrappers for goocanvas"
arch=('i686' 'x86_64')
url="http://ftp.gnome.org/pub/gnome/sources/goocanvasmm/"
license=('GPL')
depends=('gegl' 'goocanvas')
source=(http://ftp.gnome.org/pub/gnome/sources/goocanvasmm/0.14/$pkgname-$pkgver.tar.bz2)
md5sums=('12ade6affd1c25b33ece61dac9d366e6')

build() {
  cd "$srcdir/$pkgname-$pkgver"

  ./configure --prefix=/usr
  make || return 1
  make DESTDIR="$pkgdir" install
}

# vim:set ts=2 sw=2 et:
