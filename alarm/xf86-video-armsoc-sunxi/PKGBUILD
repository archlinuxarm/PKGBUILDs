# ARMSoC Mali FrameBuffer driver for SUNXI
# Maintainer: Andrea Scarpino <andrea@archlinux.org>

buildarch=8

pkgname=xf86-video-armsoc-sunxi
_commit=ee9f1d96193d4fc2887183b613f209b18a93e718
pkgver=1.4.0
pkgrel=1
pkgdesc='X.org graphics driver for ARM graphics (patched for sunxi)'
arch=('aarch64')
url='https://github.com/longsleep/xf86-video-armsoc'
license=('GPL2')
makedepends=('git' 'pkgconfig' 'xorg-server-devel' 'resourceproto' 'scrnsaverproto')
options=('!libtool')
conflicts=('xf86-video-armsoc')
provides=('xf86-video-armsoc')
source=("$pkgname::git://anongit.freedesktop.org/xorg/driver/xf86-video-armsoc#commit=${_commit}"
        '0001-sunxi_support.patch')
md5sums=('SKIP'
         'a2fb3344851407a02c9821f279b42e1a')

prepare() {
  cd $pkgname
  patch -p1 -i "${srcdir}/0001-sunxi_support.patch"
}

build() {
  cd $pkgname

  ./autogen.sh --prefix=/usr
  make
}

package() {
  cd $pkgname

  make DESTDIR="${pkgdir}" install
}
