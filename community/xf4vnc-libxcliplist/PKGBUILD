# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Sergej Pupykin <pupykin.s+arch@gmail.com>
# Contributor: Sergej Pupykin <pupykin.s+arch@gmail.com>

pkgname=xf4vnc-libxcliplist
pkgver=20090311
pkgrel=1
pkgdesc="The XCliplist client-side library"
arch=(i686 x86_64)
url="http://xf4vnc.sf.net"
license=('GPL')
depends=(libxext)
makedepends=(xf4vnc-xcliplistproto)
source=(http://archlinux-stuff.googlecode.com/files/xf4vnc.cvs.$pkgver.tar.gz)

build() {
  cd $startdir/src/xf4vnc.cvs/src/lib/libXcliplist
  ./autogen.sh --prefix=/usr
  make DESTDIR=$startdir/pkg install
}
