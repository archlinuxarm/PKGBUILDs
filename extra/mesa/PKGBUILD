# $Id$
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, radeon, VMware svga).
#  - Removed libgles, libegl and khrplatform-devel from conflicts for marvell-libgfx compatibility.
#  - Moved .pc files to mesa-libgl that reference libraries in mesa-libgl

pkgbase=mesa
pkgname=('mesa' 'mesa-libgl' 'libva-mesa-driver')
pkgver=10.5.2
pkgrel=1
arch=('i686' 'x86_64')
makedepends=('python2-mako' 'libxml2' 'libx11' 'glproto' 'libdrm' 'dri2proto' 'dri3proto' 'presentproto'
             'libxshmfence' 'libxxf86vm' 'libxdamage' 'libvdpau' 'libva' 'wayland' 'elfutils' 'llvm'
             'systemd' 'libomxil-bellagio' 'clang')
url="http://mesa3d.sourceforge.net"
license=('custom')
options=('!libtool')
source=(ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/mesa-${pkgver}.tar.xz{,.sig}
        LICENSE)
sha256sums=('2f4b6fb77c3e7d6f861558d0884a3073f575e1e673dad8d1b0624e78e9c4dd44'
            'SKIP'
            '7fdc119cf53c8ca65396ea73f6d10af641ba41ea1dd2bd44a824726e01c8b3f2')
validpgpkeys=('8703B6700E7EE06D7A39B8D6EDAE37B02CEB490D') # Emil Velikov <emil.l.velikov@gmail.com>

build() {
  cd ${srcdir}/?esa-*

  #autoreconf -vfi # our automake is far too new for their build system :)

  ./configure --prefix=/usr \
    --sysconfdir=/etc \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-gallium-drivers=nouveau,swrast \
    --with-dri-drivers=nouveau,swrast \
    --with-egl-platforms=x11,drm,wayland \
    --enable-llvm-shared-libs \
    --enable-egl \
    --enable-gbm \
    --enable-gallium-llvm \
    --enable-shared-glapi \
    --enable-glx \
    --enable-glx-tls \
    --enable-dri \
    --enable-osmesa \
    --enable-gles1 \
    --enable-gles2 \
    --enable-texture-float \
    --enable-omx \
    --enable-nine \
    --with-clang-libdir=/usr/lib
    # --help

  make

  # fake installation
  mkdir $srcdir/fakeinstall
  make DESTDIR=${srcdir}/fakeinstall install
}

package_libva-mesa-driver() {
  pkgdesc="VA-API implementation for gallium"
  depends=('libdrm' 'libx11' 'llvm-libs' 'expat' 'elfutils')

  install -m755 -d ${pkgdir}/usr/lib
  mv -v ${srcdir}/fakeinstall/usr/lib/dri ${pkgdir}/usr/lib
   
  install -m755 -d "${pkgdir}/usr/share/licenses/libva-mesa-driver"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libva-mesa-driver/"
}
               
package_mesa() {
  pkgdesc="an open-source implementation of the OpenGL specification"
  depends=('libdrm' 'wayland' 'libxxf86vm' 'libxdamage' 'libxshmfence' 'systemd' 'elfutils' 
           'libomxil-bellagio' 'expat' 'libtxc_dxtn' 'llvm-libs')
  optdepends=('opengl-man-pages: for the OpenGL API man pages'
              'mesa-vdpau: for accelerated video playback'
              'libva-mesa-driver: for accelerated video playback')
  provides=('libglapi' 'osmesa' 'libgbm' 'libgles' 'libegl' 'khrplatform-devel'
            'ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri')
  conflicts=('libglapi' 'osmesa' 'libgbm' 'libgles' 'libegl' 'khrplatform-devel'
             'ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri')
  replaces=('libglapi' 'osmesa' 'libgbm' 'libgles' 'libegl' 'khrplatform-devel'
            'ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri')

  install -m755 -d ${pkgdir}/etc
  mv -v ${srcdir}/fakeinstall/etc/drirc ${pkgdir}/etc
  
  install -m755 -d ${pkgdir}/usr/lib/xorg/modules/dri
  # ati-dri, nouveau-dri, intel-dri, svga-dri, swrast
  mv -v ${srcdir}/fakeinstall/usr/lib/xorg/modules/dri/* ${pkgdir}/usr/lib/xorg/modules/dri

  mv -v ${srcdir}/fakeinstall/usr/lib/bellagio  ${pkgdir}/usr/lib
  mv -v ${srcdir}/fakeinstall/usr/lib/d3d  ${pkgdir}/usr/lib
  mv -v ${srcdir}/fakeinstall/usr/lib/*.so* ${pkgdir}/usr/lib/

  mv -v ${srcdir}/fakeinstall/usr/include ${pkgdir}/usr

  install -m755 -d ${pkgdir}/usr/lib/pkgconfig
  mv -v ${srcdir}/fakeinstall/usr/lib/pkgconfig/{osmesa,gbm,dri,wayland-egl}.pc ${pkgdir}/usr/lib/pkgconfig
  
  install -m755 -d ${pkgdir}/usr/lib/mesa
  # move libgl/EGL/glesv*.so to not conflict with blobs - may break .pc files ?
  mv -v ${pkgdir}/usr/lib/libGL.so* 	${pkgdir}/usr/lib/mesa/
  mv -v ${pkgdir}/usr/lib/libEGL.so* 	${pkgdir}/usr/lib/mesa/
  mv -v ${pkgdir}/usr/lib/libGLES*.so*	${pkgdir}/usr/lib/mesa/

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa/"
}

package_mesa-libgl() {
  pkgdesc="Mesa 3-D graphics library"
  depends=('mesa')
  provides=('libgl')
  replaces=('libgl')

  install -m755 -d ${pkgdir}/usr/lib
  mv -v ${srcdir}/fakeinstall/usr/lib/pkgconfig ${pkgdir}/usr/lib/
 
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

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa-libgl"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa-libgl/"
}
