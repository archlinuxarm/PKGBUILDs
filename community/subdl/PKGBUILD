# Maintainer: Geoffroy Carrier <geoffroy.carrier@aur.archlinux.org>
# Contributor: rich_o <rich_o@lavabit.com>
pkgname=subdl
pkgver=1.0.2
pkgrel=1
pkgdesc="A command-line tool for downloading subtitles from opensubtitles.org."
arch=('i686' 'x86_64')
url="http://www.cubewano.org/subdl"
license=('GPL3')
depends=('python')
source=(http://www.cubewano.org/$pkgname/downloads/$pkgver/$pkgname-$pkgver.tar.gz)
build() {
  cd "$srcdir/$pkgname-$pkgver"
  install -Dm755 $pkgname $startdir/pkg/usr/bin/$pkgname || return 1
}
