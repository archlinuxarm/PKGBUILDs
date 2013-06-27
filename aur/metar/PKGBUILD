# Maintainer: Dr Small <drsmall@mycroftserver.homelinux.org>

pkgname=metar
pkgver=20061030.1
pkgrel=1
pkgdesc="An utility to download/decode METAR reports."
arch=('i686')
url="http://packages.debian.org/unstable/utils/metar"
license=('GPL')
depends=('curl')
source=(http://ftp.de.debian.org/debian/pool/main/m/$pkgname/${pkgname}_${pkgver}.orig.tar.gz)
md5sums=('9349aee1e492473aabc846db8b35c348')

build() {
cd "${startdir}/src/$pkgname-$pkgver"

./configure --prefix=/usr -mandir=/usr/share/man
make || return 1
make DESTDIR=${pkgdir} install
}
