# Maintainer: Thomas Bächler <thomas@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - disable distcc - configure checks for clang first

pkgname=v4l-utils
pkgver=1.24.1
pkgrel=2
pkgdesc="Userspace tools and conversion library for Video 4 Linux"
arch=('x86_64')
url="https://linuxtv.org/"
provides=("libv4l=$pkgver")
replaces=('libv4l')
conflicts=('libv4l')
backup=(etc/rc_maps.cfg)
license=('LGPL')
options=('!distcc')
depends=('hicolor-icon-theme' 'gcc-libs' 'libjpeg-turbo'  'systemd-libs' 'json-c')
makedepends=('qt5-base' 'alsa-lib')
optdepends=('qt5-base: for qv4l2 and qvidcap' 'alsa-lib: for qv4l2')
source=(https://linuxtv.org/downloads/v4l-utils/${pkgname}-${pkgver}.tar.bz2{,.asc})
sha256sums=('cbb7fe8a6307f5ce533a05cded70bb93c3ba06395ab9b6d007eb53b75d805f5b'
            'SKIP')
validpgpkeys=('05D0169C26E41593418129DF199A64FADFB500FF') # Gregor Jasny <gjasny@googlemail.com>

build() {
  cd "${srcdir}/${pkgname}-${pkgver}"

  ./configure --prefix=/usr --sysconfdir=/etc --sbindir=/usr/bin
  make
}

package() {
  cd "${srcdir}/${pkgname}-${pkgver}"
  MAKEFLAGS="-j1" make install DESTDIR="${pkgdir}/"
  rm "${pkgdir}/usr/bin/ivtv-ctl"
}
