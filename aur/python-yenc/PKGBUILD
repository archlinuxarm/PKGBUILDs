# Contributor: Jonathan Curran <jonc@icicled.net>
# Maintainer: Clément Démoulins <clement@archivel.fr>

pkgname=python-yenc
pkgver=0.3
pkgrel=2
arch=('i686' 'x86_64')
license=('GPL')
pkgdesc="yenc decoder for python"
url="http://sabnzbd.sourceforge.net/"
depends=('python2')
source=(http://sabnzbd.sourceforge.net/yenc-$pkgver.tar.gz)
md5sums=('7b3edd32db6c1ce566ad550e3de64c83')

build() {
	cd $startdir/src/yenc-$pkgver
	python2 setup.py install --root=$startdir/pkg
}

