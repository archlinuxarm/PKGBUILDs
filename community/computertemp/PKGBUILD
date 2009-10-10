# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Roman Kyrylych <Roman.Kyrylych@gmail.com>
# Contributor: William Rea <sillywilly@gmail.com>

pkgname=computertemp
pkgver=0.9.6.1
pkgrel=4
pkgdesc="GNOME applet that shows the temperature of your CPU"
arch=('i686' 'x86_64')
url="http://computertemp.berlios.de"
license=('GPL')
depends=('gnome-panel' 'gnome-python-desktop' 'hddtemp' 'gconf>=2.18.0.1-4')
makedepends=('intltool' 'gnome-doc-utils>=0.11.2')
source=(http://download.berlios.de/computertemp/computertemp-$pkgver.tar.gz)
install=$pkgname.install
md5sums=('3c42a1ab447c29e9eb1e51b98f9af595')

build() {
  cd $srcdir/$pkgname-$pkgver
  ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var \
    --disable-scrollkeeper
  make || return 1
  make -j1 GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1 DESTDIR=$pkgdir install || return 1

  mkdir -p $pkgdir/usr/share/gconf/schemas
  gconf-merge-schema $pkgdir/usr/share/gconf/schemas/${pkgname}.schemas \
    $pkgdir/etc/gconf/schemas/*.schemas
  rm -f $pkgdir/etc/gconf/schemas/*.schemas
}
