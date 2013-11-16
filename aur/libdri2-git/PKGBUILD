# Maintainer: Georges Discry <georges at discry dot be>

pkgname=libdri2-git
_gitname=libdri2
pkgver=11.4f1eef3
pkgrel=1
pkgdesc="Library for the DRI2 extension to the X Window System"
arch=('armv7h')
url="https://github.com/robclark/libdri2"
license=('MIT')
depends=('libdrm' 'libxext')
makedepends=('git' 'xextproto' 'xorg-server-devel')
options=('!libtool')
provides=('libdri2')
source=('git+https://github.com/robclark/libdri2.git')
md5sums=('SKIP')

pkgver() {
  cd "${SRCDEST}/${_gitname}"
  echo $(git rev-list --count master).$(git rev-parse --short master)
}

build() {
  cd "${srcdir}/${_gitname}"
  ./autogen.sh
  ./configure --prefix=/usr
  make
}

package() {
  cd "${srcdir}/${_gitname}"
  make DESTDIR="${pkgdir}/" install
  install -m755 -d "${pkgdir}/usr/share/licenses/${pkgname}"
  install -m644 COPYING "${pkgdir}/usr/share/licenses/${pkgname}/"
}
