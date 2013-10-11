# Maintainer: Rolinh <robinDOThahlingATgw-computingDOTnet>
pkgname=dfc
pkgver=3.0.4
pkgrel=2
pkgdesc="Display file system space usage using graphs and colors"
arch=('x86_64' 'i686')
url="http://projects.gw-computing.net/projects/dfc"
license=('BSD')
depends=('glibc')
makedepends=('cmake' 'gettext')
conflicts=('dfc-git')
source=(http://projects.gw-computing.net/attachments/download/79/${pkgname}-${pkgver}.tar.gz)
install='dfc.install'
md5sums=('3b9edc197ad21e1dfe10128d130e626c')

build() {
  cd "${srcdir}/${pkgname}-${pkgver}"

  cmake . -DPREFIX=/usr -DSYSCONFDIR=/etc -DCMAKE_BUILD_TYPE=RELEASE
  make
}

package() {
  cd "${srcdir}/${pkgname}-${pkgver}"

  make DESTDIR="${pkgdir}" install

  install -Dm644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}

# vim:set ts=2 sw=2 et:
