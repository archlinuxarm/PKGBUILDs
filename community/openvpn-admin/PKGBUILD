# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Sergej Pupykin <pupykin.s+arch@gmail.com>
# Contributor: William Rea <sillywilly@gmail.com>

pkgname=openvpn-admin
pkgver=1.9.4.2
_pkgver=1.9.4
_pkgverm=2
pkgrel=1
pkgdesc="A GUI for OpenVPN that is written in Mono"
arch=('i686' 'x86_64')
license=('LGPL')
url="http://sourceforge.net/projects/openvpn-admin"
depends=('gtk-sharp-2' 'openvpn')
makedepends=('intltool')
source=(http://downloads.sourceforge.net/openvpn-admin/$pkgname-$_pkgver-$_pkgverm.tar.gz
	Configuration.cs.patch)
md5sums=('e8cbda2384f4a2f8ce40c994272ccf41' 'bfdf3a3c965cb92bfb7a3209236c4437')

build() {
  export MONO_SHARED_DIR=$startdir/src/.wabi
  mkdir -p $MONO_SHARED_DIR

  cd $startdir/src/$pkgname-$_pkgver
  patch -p1 -i ../Configuration.cs.patch
  sed -i 's|-reference:Mono.Posix|-reference:Mono.Unix|g' src/Makefile.am
  ./configure --prefix=/usr --sysconfdir=/etc
  make || return 1
  make DESTDIR=$startdir/pkg install

  rm -fr $MONO_SHARED_DIR
}
