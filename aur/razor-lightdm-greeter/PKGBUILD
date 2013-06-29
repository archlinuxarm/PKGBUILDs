# Maintainer: Jerome Leclanche <jerome.leclanche+arch@gmail.com>

pkgname=razor-lightdm-greeter
pkgver=0.5.2
pkgrel=3
pkgdesc="The Razor-qt LightDM greeter"
url="http://razor-qt.org"
arch=('i686' 'x86_64')
license="GPL2"
depends=('qt4' 'liblightdm-qt4' 'razor-qt')
makedepends=('cmake')
source=("http://razor-qt.org/downloads/razorqt-${pkgver}.tar.bz2")
sha256sums=('ac8a890eba7a24a20a2c0ea7a5020c6001853997c1e1b1b927ff4700b0e0e1ad')

build() {
	cd "${srcdir}/razorqt-${pkgver}"
	cmake ./ -DCMAKE_INSTALL_PREFIX=/usr -DSPLIT_BUILD=On -DMODULE_LIGHTDM=On
	make
}

package() {
	cd "${srcdir}/razorqt-${pkgver}"
	make DESTDIR="${pkgdir}" install
}
