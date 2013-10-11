# Maintainer: Dobroslaw Kijowski [dobo] <dobo90_at_gmail.com>
# Contributor: Jerome Leclanche <jerome.leclanche+arch@gmail.com>

pkgname=lightdm-razor-greeter
pkgver=0.5.2
pkgrel=2
pkgdesc='The Razor-qt LightDM greeter'
url='http://razor-qt.org'
arch=(i686 x86_64)
license=(GPL2)
depends=(liblightdm-qt4 razor-qt)
makedepends=(cmake)
source=(http://razor-qt.org/downloads/razorqt-${pkgver}.tar.bz2
        liblightdm-qt-3.patch)
md5sums=(8b2da8ab69065926bfc998cf1960bffb
         cb8c2521110dae53e6211f484fc60992)

prepare() {
  patch -p0 < liblightdm-qt-3.patch
}

build() {
  cd ${srcdir}/razorqt-${pkgver}

  mkdir build
  cd build

  cmake ../ -DCMAKE_INSTALL_PREFIX=/usr -DQT_QMAKE_EXECUTABLE=qmake-qt4 -DLIB_SUFFIX="" \
    -DSPLIT_BUILD=ON -DMODULE_LIGHTDM=ON
  make
}

package() {
  cd ${srcdir}/razorqt-${pkgver}/build

  make DESTDIR=${pkgdir} install
}
