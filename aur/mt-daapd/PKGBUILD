# Maintainer: Marcin Mikolajczyk <marcinmikolajcz at gmail dot com>
# Contributor: Gary Wright <wriggary at g mail dot com> $
# Contributor: Sergej Pupykin <pupykin.s+arch@gmail.com>
# Contributor: Jon Kristian Nilsen <jokr.nilsen@gmail.com>
# Contributor: Alan Orth <alan.orth at gmail dot com>

pkgname=mt-daapd
pkgver=0.2.4.2
pkgrel=9
pkgdesc="A multi-threaded DAAP server compatible with iTunes music sharing"
arch=('i686' 'x86_64')
url="http://www.mt-daapd.org/"
license=('LGPL')
depends=('libid3tag' 'avahi')
backup=(etc/mt-daapd/mt-daapd.conf)
source=(http://downloads.sourceforge.net/$pkgname/$pkgname-$pkgver.tar.gz \
	mt-daapd fix-libs.patch mt-daapd.service mt-daapd.install)
md5sums=('67bef9fb14d487693b0dfb792c3f1b05'
         '26cf6ff799ef06d262a8648d6890a250'
         '59896e760486a51170d8b0e7ed5a2506'
         'fde3fa3d5d48c05241b21b8b28d7ffa2'
         '17c0262f291a8aaa15efc5c8e3a8e2f7')

install=mt-daapd.install

build() {
  cd "$srcdir/$pkgname-$pkgver"

  sed -i 's/AM_CONFIG_HEADER/AC_CONFIG_HEADERS/' configure.in
  mv configure.in configure.ac

  sed -i 's|DEFAULT_CONFIGFILE "/etc/mt-daapd.conf"|DEFAULT_CONFIGFILE "/etc/mt-daapd/mt-daapd.conf"|' src/main.c
  patch configure.ac < "$srcdir/fix-libs.patch" 
  aclocal
  automake --add-missing
  autoreconf
  ./configure --prefix=/usr --sysconfdir=/etc/mt-daapd --sbindir=/usr/bin --enable-avahi --enable-mdns
  make  
}

package() {
  cd "$srcdir/$pkgname-$pkgver"

  make DESTDIR="$pkgdir" install 

  install -D -m644 contrib/mt-daapd.playlist "$pkgdir/etc/mt-daapd/sample.playlist" || return 1
  install -D -m644 contrib/mt-daapd.conf "$pkgdir/etc/mt-daapd/mt-daapd.conf" || return 1
  install -D -m755 ../mt-daapd "$pkgdir/etc/rc.d/mt-daapd" || return 1
  install -D -m644 ../mt-daapd.service "$pkgdir/usr/lib/systemd/system/mt-daapd.service" || return 1

  # create cache directories
  install -d "$pkgdir/var/cache/mt-daapd" || return 1
}
