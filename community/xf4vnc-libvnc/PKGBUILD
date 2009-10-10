# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Sergej Pupykin <pupykin.s+arch@gmail.com>
# Contributor: Sergej Pupykin <pupykin.s+arch@gmail.com>

pkgname=xf4vnc-libvnc
pkgver=20090311
pkgrel=1
pkgdesc="The VNC client-side library"
arch=(i686 x86_64)
url="http://xf4vnc.sf.net"
license=('GPL')
depends=(libxext)
makedepends=(xf4vnc-vncproto)
source=(http://archlinux-stuff.googlecode.com/files/xf4vnc.cvs.$pkgver.tar.gz)

build() {
  cd $startdir/src/xf4vnc.cvs/src/lib/libvnc
  ./autogen.sh --prefix=/usr
  make DESTDIR=$startdir/pkg install
}
