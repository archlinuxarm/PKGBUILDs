pkgname=kdeplasma-applets-quickaccess
pkgver=0.8.3
pkgrel=1
pkgdesc='KDE Plasma widget for quick access to folders.'
arch=('i686' 'x86_64')
url='http://kde-look.org/content/show.php?content=163309'
license=('GPL2')
conflicts=('quickaccess-plasmoid')
depends=('kdebase-workspace' 'kdebase-plasma')
makedepends=('cmake' 'automoc4')
source=('http://kde-look.org/CONTENT/content-files/163309-plasma-widget-quickaccess-0.8.3.tar.gz')
sha1sums=('d9174ca63e80cfc661fcdbc1d730a5986e7c8e2f')

build() {
  mkdir build
  cd build
  cmake ../plasma-widget-quickaccess-${pkgver} \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` \
          -DQT_QMAKE_EXECUTABLE=qmake-qt4
  make
}

package() {
  cd build
  make DESTDIR=$pkgdir install
}
