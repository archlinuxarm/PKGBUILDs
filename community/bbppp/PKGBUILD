# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Sergej Pupykin <pupykin.s+arch@gmail.com>
# Maintainer: dorphell <dorphell@archlinux.org>
# Contributor: Tom Newsom <Jeepster@gmx.co.uk>
#
pkgname=bbppp
pkgver=0.2.5
pkgrel=1
pkgdesc="bbppp displays the status of your ppp-link, Blackbox"
arch=(i686 x86_64)
depends=('blackbox')
url="http://bbtools.sourceforge.net/"
license=('GPL')
source=(http://downloads.sourceforge.net/sourceforge/bbtools/bbppp-$pkgver.tar.gz)
md5sums=('5162b849f2d5f2ee374b6f5344e1a7e2')

build() {
  cd $startdir/src/$pkgname-$pkgver

  ./configure --prefix=/usr
  make || return 1
  make prefix=$startdir/pkg/usr install
}
