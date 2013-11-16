# Maintainer: smotocel69 <smotocel69@gmail.com>
# Contribuitor: Georges Discry <georges at discry dot be>

pkgname=xf86-video-fbturbo-git
_gitname=xf86-video-fbturbo
pkgver=188.c395869
pkgrel=3
pkgdesc="X.org MALI video driver"
arch=('armv7h')
url="https://github.com/ssvb/xf86-video-fbturbo"
license=('MIT')
depends=('libump-git')
makedepends=('git' 'xorg-server-devel' 'X-ABI-VIDEODRV_VERSION=14')
conflicts=('xorg-server<1.14.0' 'X-ABI-VIDEODRV_VERSION<14' 'X-ABI-VIDEODRV_VERSION>=15')
options=('!libtool')
groups=('xorg-drivers' 'xorg')
provides=('xf86-video-fbturbo')
source=('git+https://github.com/ssvb/xf86-video-fbturbo')
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
  mkdir -p "${pkgdir}/etc/X11/xorg.conf.d/"
  cp "xorg.conf" "${pkgdir}/etc/X11/xorg.conf.d/99-fbturbo.conf"
  make DESTDIR="${pkgdir}/" install
  install -m755 -d "${pkgdir}/usr/share/licenses/${pkgname}"
  install -m644 COPYING "${pkgdir}/usr/share/licenses/${pkgname}/"
}
