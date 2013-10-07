# $Id$
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, radeon, VMware svga).
#  - Build v7h with -O1 instead of -O2
#  - Removed libgles, libegl and khrplatform-devel from conflicts for marvell-libgfx compatibility.

pkgbase=mesa
pkgname=('mesa' 'mesa-libgl')
pkgver=9.2.1
pkgrel=1
arch=('i686' 'x86_64')
makedepends=('python2' 'libxml2' 'libx11' 'glproto' 'libdrm' 'dri2proto' 'libxxf86vm' 'libxdamage'
             'libvdpau' 'wayland' 'elfutils' 'llvm' 'systemd')
url="http://mesa3d.sourceforge.net"
license=('custom')
options=('!libtool')
source=(ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/MesaLib-${pkgver}.tar.bz2
        LICENSE)
md5sums=('dd4c82667d9c19c28a553b12eba3f8a0'
         '5c65a0fe315dd347e09b1f2826a1df5a')

build() {
  cd ${srcdir}/?esa-*

  autoreconf -vfi # our automake is far too new for their build system :)

  ./configure --prefix=/usr \
    --sysconfdir=/etc \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-gallium-drivers=swrast \
    --with-dri-drivers=swrast \
    --with-egl-platforms=x11,drm,wayland \
    --with-llvm-shared-libs \
    --enable-gallium-llvm \
    --enable-egl \
    --enable-gallium-egl \
    --enable-shared-glapi \
    --enable-gbm \
    --enable-glx-tls \
    --enable-dri \
    --enable-glx \
    --enable-osmesa \
    --enable-gles1 \
    --enable-gles2 \
    --enable-texture-float \
    --enable-xa
    # --help

  make

  # fake installation
  mkdir $srcdir/fakeinstall
  make DESTDIR=${srcdir}/fakeinstall install
}

package_mesa() {
  pkgdesc="an open-source implementation of the OpenGL specification"
  depends=('libdrm' 'libvdpau' 'wayland' 'libxxf86vm' 'libxdamage' 'systemd' 'elfutils' 'llvm-libs')
  optdepends=('opengl-man-pages: for the OpenGL API man pages')
  provides=('libglapi' 'osmesa' 'libgbm' 'libgles' 'libegl' 'khrplatform-devel')
  conflicts=('libglapi' 'osmesa' 'libgbm')
  replaces=('libglapi' 'osmesa' 'libgbm' 'libgles' 'libegl' 'khrplatform-devel')

  mv -v ${srcdir}/fakeinstall/* ${pkgdir}
  # rename libgl.so to not conflict with blobs - may break gl.pc ?
  mv ${pkgdir}/usr/lib/libGL.so.1.2.0 	${pkgdir}/usr/lib/mesa-libGL.so.1.2.0
  rm ${pkgdir}/usr/lib/libGL.so{,.1}

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa/"
}

package_mesa-libgl() {
  pkgdesc="Mesa 3-D graphics library"
  depends=("mesa=${pkgver}")
  provides=("libgl=${pkgver}")
  replaces=('libgl')
 
  # See FS#26284
  install -m755 -d "${pkgdir}/usr/lib/xorg/modules/extensions"
  ln -s libglx.xorg "${pkgdir}/usr/lib/xorg/modules/extensions/libglx.so"

  ln -s mesa-libGL.so.1.2.0      ${pkgdir}/usr/lib/libGL.so
  ln -s mesa-libGL.so.1.2.0      ${pkgdir}/usr/lib/libGL.so.1
  ln -s mesa-libGL.so.1.2.0      ${pkgdir}/usr/lib/libGL.so.1.2.0

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa-libgl"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa-libgl/"
}
