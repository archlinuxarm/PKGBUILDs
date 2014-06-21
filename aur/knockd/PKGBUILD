# $Id: PKGBUILD 20410 2008-12-03 09:44:51Z tom $
# Maintainer: Mike Sampson <mike at sambodata dot com>
# Contriburor: Steven <steven at stebalien dot com>
# Contributor: Aaron Griffin <aaron@archlinux.org>
# Contributor: judd <jvinet@zeroflux.org>

pkgname=knockd
pkgver=0.7
pkgrel=1
pkgdesc="A simple port-knocking daemon"
arch=('i686' 'x86_64')
url="http://www.zeroflux.org/projects/knock"
license=(GPL)
depends=('libpcap>=1.0.0')
backup=('etc/knockd.conf')
source=(http://www.zeroflux.org/proj/knock/files/knock-$pkgver.tar.gz
        knockd knockd.logrotate knockd.service limits.h.patch)

md5sums=('cb6373fd4ccb42eeca3ff406b7fdb8a7'
         '72302d899887996694586c3d479de245'
         '56a4113ec89ba2e96f79ab23c914d52a'
         'da806fb9fbdf638df4e0011070ff9ef1'
         'e56dc7359f36c3b07da6710c404d671e')

build() {
  cd "$srcdir/knock-$pkgver"
  patch -Np0 -i "$srcdir/limits.h.patch"
  ./configure --prefix=/usr --sbin=/usr/bin --sysconfdir=/etc
  make
}

package() {
  cd "$srcdir/knock-$pkgver"
  make DESTDIR="$pkgdir" MANDIR=/usr/share/man install
  #install -D -m755 "$srcdir/knockd" "$pkgdir/etc/rc.d/knockd"
  install -D -m755 "$srcdir/knockd.service" "$pkgdir/usr/lib/systemd/system/knockd.service"
  install -D -m644 "$srcdir/knockd.logrotate" "$pkgdir/etc/logrotate.d/knockd"
}
