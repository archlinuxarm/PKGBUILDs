# Maintainer: Geoffroy Carrier <geoffroy.carrier@koon.fr> 
pkgname=noyau
pkgver=2.1
pkgrel=2
pkgdesc="Bash script to ease kernel compilation"
arch=('i686' 'x86_64')
url="http://vincent.riquer.eu.org/Projets/noyau.sh/"
license=('GPL2')
depends=('bash')
source=(http://vincent.riquer.eu.org/Projets/noyau.sh/$pkgver/noyau.sh)
build() {
  install -Dm755 $srcdir/noyau.sh $pkgdir/usr/bin/noyau
}
md5sums=('37c9d8b125c9e3839487446bde2f359b')
