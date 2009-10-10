# Maintainer: Varun Acharya <varun@archlinux.org>
# Contributor: Nicolai Lissner <nlissne@linux01.gwdg.de>
# changes to make a static build against the older libdvdread by denis on AUR

pkgname=dvdbackup
pkgver=0.1.1
pkgrel=2
pkgdesc="Backup contents of video-dvd to harddisk"
arch=('i686' 'x86_64')
url="http://dvd-create.sourceforge.net"
license="GPL"
depends=()
source=(http://dvd-create.sourceforge.net/$pkgname-$pkgver.tar.gz
	http://dvd-create.sourceforge.net/libdvdread-0.9.3.tar.gz)
md5sums=('53a071d1def5ee49d702a4dd080d25ac' '761db1225098c0834485396d9285e5ff')

build() {
	cd $startdir/src/libdvdread-0.9.3
	./configure --prefix=/usr --disable-warnings
	make || return 1
	cp ./dvdread/.libs/libdvdread.a $startdir/src/$pkgname/src
	cd $startdir/src/$pkgname/src
	gcc $CFLAGS -I$startdir/src/libdvdread-0.9.3 -c dvdbackup.c -o dvdbackup.o
	gcc -o dvdbackup dvdbackup.o libdvdread.a -ldl
	install -m 755 -d $startdir/pkg/usr/bin
	install -m 755 dvdbackup $startdir/pkg/usr/bin
}
