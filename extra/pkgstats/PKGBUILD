# $Id: $
# Maintainer: Pierre Schmitz <pierre@archlinux.de>

pkgname=pkgstats
pkgver=1.0
pkgrel=2
pkgdesc='submits a list of installed packages to the Arch Linux project'
arch=('i686' 'x86_64')
url='http://www.archlinux.de'
license=('GPL')
depends=('bash' 'curl' 'pacman')
source=('pkgstats.sh')
md5sums=('43096cb1ad7c1228510399c8d45a5780')

build() {
	install -D -m755 ${srcdir}/pkgstats.sh ${pkgdir}/usr/bin/pkgstats
}
