# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Sergej Pupykin <pupykin.s+arch@gmail.com>
# Contributor: Alexander Mieland <dma147@linux-stats.org>

pkgname=xchatosd
pkgver=5.19
pkgrel=1
pkgdesc="OSD plugin/Script for xchat"
arch=(i686 x86_64)
url="http://sourceforge.net/projects/xchatosd/"
depends=('xchat' 'xosd')
license=('GPL2')
source=(http://downloads.sourceforge.net/sourceforge/xchatosd/xchatosd-$pkgver.tar.gz
	build-fix.patch)
md5sums=('d95091553dc19e32aefb03ec46ab641d' '9fa6903a34afc2e9f4b92ea6eb1f56bc')

build() {
  cd $startdir/src/$pkgname-$pkgver
  patch -Np1 <../build-fix.patch
    if [ "$CARCH" == "x86_64" ] ; then
    sed -re 's|-L/usr/local/lib|-L/usr/local/lib -fPIC|' -i Makefile
    fi
  make || return 1
  install -D -m 755 xchatosd.so $startdir/pkg/usr/lib/xchat/plugins/xchatosd.so
}
