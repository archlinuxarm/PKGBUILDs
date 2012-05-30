# Maintainer: Mitch Bigelow <ipha00@gmail.com>

pkgname=pianobar
pkgver=2012.05.06
pkgrel=2
pkgdesc="console-based frontend for Pandora"
url="http://6xq.net/0017"
arch=('i686' 'x86_64')
license=('MIT')
depends=('libao' 'faad2' 'libmad' 'gnutls' 'json-c')
source=(http://6xq.net/media/00/16/pianobar-$pkgver.tar.bz2)
sha256sums=('b143882ca50303d560f49567d1a508ca4b48208db4eb8aa67f369fcaae708d7a')
_builddir="$pkgname-$pkgver"

build() {
    cd "$_builddir"
    make
}

package() {
    cd "$_builddir"

    make DESTDIR=$pkgdir PREFIX=/usr install
    install -vDm644 COPYING "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
