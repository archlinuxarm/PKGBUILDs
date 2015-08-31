# Maintainer: Kyle Keen <keenerd@gmail.com>
# Contributor: Danilo Luvizotto <danilo.luvizotto@gmail.com>
# Contributor: Wieland Hoffmann <the_mineo@web.de>
# Contributor: G_Syme <demichan(at)mail(dot)upb(dot)de>
pkgname=p910nd
pkgver=0.97
pkgrel=1
pkgdesc="A small printer daemon intended for diskless workstations that passes jobs directly to the printer"
arch=('i686' 'x86_64' 'armv5' 'armv5h' 'armv6' 'armv6h' 'armv7' 'armv7h')
url="http://p910nd.sourceforge.net"
license=('GPL2')
depends=('glibc')
backup=('etc/conf.d/p910nd')
#options=('emptydirs')
source=(http://downloads.sf.net/$pkgname/$pkgname-$pkgver.tar.bz2 \
        $pkgname.service \
        $pkgname.conf)
md5sums=('69461a6c54dca0b13ecad5b83864b43e'
         'e37030a69b7bf302cfb23d88e25ebbdf'
         'ea1db6d612058532c525efedd54990f2')

CONFIGDIR=/etc/conf.d
INITSCRIPT=""
SCRIPTDIR=""

build() {
  cd "$srcdir/$pkgname-$pkgver"
  # TODO: this should be in its own subdir, but needs to be created at boot 
  sed -i "s|/var/lock/subsys|/run/lock|" $pkgname.c
  # modern linux FSH
  sed -i 's|sbin|bin|' *
  sed -i 's|var/lock|run/lock|' *
  sed -i 's|var/run|run|' *
  sed -i 's|$(INSTALL) $(INITSCRIPT) $(DESTDIR)$(SCRIPTDIR)/$(PROG)||' Makefile
  make CONFIGDIR=$CONFIGDIR INITSCRIPT=$INITSCRIPT SCRIPTDIR=$SCRIPTDIR
  sed -i 's|P910ND_OPTS=""|P910ND_OPTS="-f /dev/usb/lp0"|' $pkgname.conf
}

package() {
  cd "$srcdir/$pkgname-$pkgver"
  make CONFIGDIR=$CONFIGDIR INITSCRIPT=$INITSCRIPT SCRIPTDIR=$SCRIPTDIR DESTDIR="$pkgdir" install
  #install -dm755 "$pkgdir/run/lock/$pkgname"
  install -Dm644 "$srcdir/$pkgname.service" "$pkgdir/usr/lib/systemd/system/$pkgname.service"
  install -Dm644 "$srcdir/$pkgname.conf" "$pkgdir/usr/lib/tmpfiles.d/$pkgname.conf"
}

