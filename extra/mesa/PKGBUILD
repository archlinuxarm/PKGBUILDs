# $Id$
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, radeon, VMware svga).
#  - Build v7h with -O1 instead of -O2
#  - Removed libgles, libegl and khrplatform-devel from conflicts for marvell-libgfx compatibility.
#  - Moved .pc files to mesa-libgl that referencing libraries in mesa-libgl

pkgbase=mesa
pkgname=('mesa' 'mesa-libgl')
pkgver=10.2.7
pkgrel=3
arch=('i686' 'x86_64')
makedepends=('python2' 'libxml2' 'libx11' 'glproto' 'libdrm' 'dri2proto' 'dri3proto' 'presentproto' 
             'libxshmfence' 'libxxf86vm'  'libxdamage' 'libvdpau' 'wayland' 'elfutils' 'llvm' 'systemd'
             'libomxil-bellagio' 'clang')
url="http://mesa3d.sourceforge.net"
license=('custom')
options=('!libtool')
source=(ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/MesaLib-${pkgver}.tar.bz2{,.sig}
        llvm35.patch
        0001-gallivm-Disable-workaround-for-PR12833-on-LLVM-3.2.patch
        0002-gallivm-set-mcpu-when-initializing-llvm-execution-en.patch
        LICENSE)
sha256sums=('27b958063a4c002071f14ed45c7d2a1ee52cd85e4ac8876e8a1c273495a7d43f'
            'SKIP'
            'd3d433564cd21da8aa56a9ceccee6122d5991cae2bd1924173359f13bd38bd6f'
            '5d66636b06736027708ffa60afb92fc81f085df35b9d91ab7ac4107c8b52d500'
            '8dc0935e66669bc111e69a80057831aa1f675179ca689c1c044ab588587da010'
            '7fdc119cf53c8ca65396ea73f6d10af641ba41ea1dd2bd44a824726e01c8b3f2')

prepare() {
  cd ${srcdir}/?esa-*

  patch -Np1 -i ../llvm35.patch

  # https://bugs.freedesktop.org/show_bug.cgi?id=77493
  # https://bugs.freedesktop.org/show_bug.cgi?id=83735
  patch -Np1 -i ../0001-gallivm-Disable-workaround-for-PR12833-on-LLVM-3.2.patch
  patch -Np1 -i ../0002-gallivm-set-mcpu-when-initializing-llvm-execution-en.patch
}

build() {
  cd ${srcdir}/?esa-*

  autoreconf -vfi # our automake is far too new for their build system :)

  ./configure --prefix=/usr \
    --sysconfdir=/etc \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-gallium-drivers=swrast \
    --with-dri-drivers=swrast \
    --with-egl-platforms=x11,drm,wayland \
    --enable-llvm-shared-libs \
    --enable-egl \
    --disable-gallium-egl \
    --disable-gallium-gbm \
    --enable-gbm \
    --enable-gallium-llvm \
    --enable-shared-glapi \
    --enable-glx-tls \
    --enable-dri \
    --enable-glx \
    --enable-osmesa \
    --enable-gles1 \
    --enable-gles2 \
    --enable-texture-float \
    --enable-dri3 \
    --enable-omx \
    --with-clang-libdir=/usr/lib
    # --help

  make

  # fake installation
  mkdir $srcdir/fakeinstall
  make DESTDIR=${srcdir}/fakeinstall install
}

package_mesa() {
  pkgdesc="an open-source implementation of the OpenGL specification"
  depends=('libdrm' 'wayland' 'libxxf86vm' 'libxdamage' 'libxshmfence' 'systemd' 'elfutils' 'llvm-libs')
  optdepends=('opengl-man-pages: for the OpenGL API man pages')
  provides=('libglapi' 'osmesa' 'libgbm' 'libgles' 'libegl' 'khrplatform-devel')
  conflicts=('libglapi' 'osmesa' 'libgbm')
  replaces=('libglapi' 'osmesa' 'libgbm' 'libgles' 'libegl' 'khrplatform-devel')

  mv -v ${srcdir}/fakeinstall/* ${pkgdir}
  install -m755 -d ${pkgdir}/usr/lib/mesa
  # move libgl/EGL/glesv*.so to not conflict with blobs - may break .pc files ?
  mv -v ${pkgdir}/usr/lib/libGL.so* 	${pkgdir}/usr/lib/mesa/
  mv -v ${pkgdir}/usr/lib/libEGL.so* 	${pkgdir}/usr/lib/mesa/
  mv -v ${pkgdir}/usr/lib/libGLES*.so*	${pkgdir}/usr/lib/mesa/

  # remove .pc files that refer to libraries packaged in mesa-libgl
  rm ${pkgdir}/usr/lib/pkgconfig/{gl,gles{v1_cm,v2},egl}.pc

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

  ln -s /usr/lib/mesa/libGL.so.1.2.0 ${pkgdir}/usr/lib/libGL.so.1.2.0
  ln -s libGL.so.1.2.0	             ${pkgdir}/usr/lib/libGL.so.1
  ln -s libGL.so.1.2.0               ${pkgdir}/usr/lib/libGL.so

  ln -s /usr/lib/mesa/libEGL.so.1.0.0 ${pkgdir}/usr/lib/libEGL.so.1.0.0
  ln -s libEGL.so.1.0.0	              ${pkgdir}/usr/lib/libEGL.so.1
  ln -s libEGL.so.1.0.0               ${pkgdir}/usr/lib/libEGL.so

  ln -s /usr/lib/mesa/libGLESv1_CM.so.1.1.0 ${pkgdir}/usr/lib/libGLESv1_CM.so.1.1.0
  ln -s libGLESv1_CM.so.1.1.0	            ${pkgdir}/usr/lib/libGLESv1_CM.so.1
  ln -s libGLESv1_CM.so.1.1.0               ${pkgdir}/usr/lib/libGLESv1_CM.so

  ln -s /usr/lib/mesa/libGLESv2.so.2.0.0 ${pkgdir}/usr/lib/libGLESv2.so.2.0.0
  ln -s libGLESv2.so.2.0.0               ${pkgdir}/usr/lib/libGLESv2.so.2
  ln -s libGLESv2.so.2.0.0               ${pkgdir}/usr/lib/libGLESv2.so

  # install .pc files
  install -m755 -d "${pkgdir}/usr/lib/pkgconfig"
  install -m644 ${srcdir}/?esa-*/src/{mesa/gl.pc,mapi/{es1api/glesv1_cm.pc,es2api/glesv2.pc},egl/main/egl.pc} "${pkgdir}/usr/lib/pkgconfig"

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa-libgl"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa-libgl/"
}
