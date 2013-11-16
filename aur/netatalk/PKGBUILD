# Maintainer: SJ_UnderWater
# Based on netatalk package :
# Maintainer: Dominik Dingel <mail at wodar dot de>
# Contributor: William Udovich <nerdzrule7 at earthlink dot net>
# Contributor: Farhan Yousaf <farhany at xaviya dot com>

pkgname=netatalk
pkgver=3.1.0
pkgrel=1
pkgdesc='A kernel-level implementation of AFP services'
arch=('i686' 'x86_64')
url='http://netatalk.sourceforge.net'
license=('GPL')
depends=('avahi>=0.6' 'libldap' 'libgcrypt>=1.2.3' 'libevent' 'python' 'dbus-glib')
replaces=('netatalk-git' 'netatalk2')
backup=('etc/afp.conf'
	'etc/extmap.conf')
options=('!libtool')
install=$pkgname.install
changelog=$pkgname.changelog
source=(http://softlayer-dal.dl.sourceforge.net/project/$pkgname/$pkgname/${pkgver%.*}/$pkgname-$pkgver.tar.bz2)
md5sums=('30ae75457ed4d28ae1c8620d5579cd60')

build() {
	cd $pkgname-$pkgver
	msg2 'Fixing...'
	: | tee test/afpd/Make*
	sed -i -e 's:AX_CHECK_DOCBOOK:[AX_CHECK_DOCBOOK]:' -e 's:"/lib/systemd:"/usr/lib/systemd:' -e 's/x"linux/x"generic/' macros/netatalk.m4
	sed -i 's:AM_CONFIG_HEADER:AC_CONFIG_HEADERS:' {configure.ac,libevent/configure.in}
	cp /usr/share/automake*/missing .
	autoreconf -i >/dev/null
	msg2 'Configuring...'
	CFLAGS="-Wno-unused-result -O2" ./configure --prefix=/usr --localstatedir=/var/state --sysconfdir=/etc --sbindir=/usr/bin --with-init-style=systemd \
		--with-cracklib --with-cnid-cdb-backend --enable-pgp-uam --with-libevent=no
	sed -i -e s/-Ino// -e s/-Lno// etc/netatalk/Makefile
	msg2 'Making...'
	make >/dev/null
}
package() {
	cd $pkgname-$pkgver
	msg2 'Building...'
	make DESTDIR="$pkgdir" install >/dev/null
}
