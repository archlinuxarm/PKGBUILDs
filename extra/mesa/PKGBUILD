# $Id$
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, radeon, VMware svga).
#  - Removed libgles, libegl and khrplatform-devel from conflicts for marvell-libgfx compatibility.
#  - Moved .pc files to mesa-libgl that reference libraries in mesa-libgl
#  - Build vc4 gallium driver for v6/v7

pkgbase=mesa
pkgname=('mesa' 'mesa-libgl' 'libva-mesa-driver')
pkgver=12.0.1
pkgrel=7
arch=('i686' 'x86_64')
makedepends=('python2-mako' 'libxml2' 'libx11' 'glproto' 'libdrm' 'dri2proto' 'dri3proto' 'presentproto' 
             'libxshmfence' 'libxxf86vm' 'libxdamage' 'libvdpau' 'libva' 'wayland' 'elfutils' 'llvm'
             'systemd' 'libomxil-bellagio' 'libgcrypt' 'clang')
url="http://mesa3d.sourceforge.net"
license=('custom')
options=('!libtool')
source=(ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/mesa-${pkgver}.tar.xz{,.sig}
        LICENSE
        remove-libpthread-stubs.patch
        0001-st-mesa-fix-reference-counting-bug-in-st_vdpau.patch
        0002-vl-dri3-fix-a-memory-leak-from-front-buffer.patch
        0001-Mesa-dev-st_glsl_to_tgsi-overlord-fix.patch
        0001-i965-import-prime-buffers-in-the-current-context-not.patch)
sha256sums=('bab24fb79f78c876073527f515ed871fc9c81d816f66c8a0b051d8d653896389'
            'SKIP'
            '7fdc119cf53c8ca65396ea73f6d10af641ba41ea1dd2bd44a824726e01c8b3f2'
            'd82c329e89754266eb1538df29b94d33692a66e3b6882b2cee78f4d5aab4a39c'
            'ccc8ea7f4e38f2dc26fd29150929e943aac5bc9b56bd3eddec882c6ccd1d64a5'
            'f6c17257e96182ce51b85ef75cef4f6c205b00dfbf8fc1089cd77c4a3eda6981'
            'c1b650d2b3512d5f8f463f1974a28ed6b88e7e1936c5cfe1034ab97696d1de14'
            '6d6159b9080d75f653dbd43284b5138264612a7f4b895c37859f984d1ea2246d')
validpgpkeys=('8703B6700E7EE06D7A39B8D6EDAE37B02CEB490D') # Emil Velikov <emil.l.velikov@gmail.com>

prepare() {
  cd ${srcdir}/?esa-*

  # Now mesa checks for libpthread-stubs - so remove the check
  patch -Np1 -i ../remove-libpthread-stubs.patch

  # fix memory leaks - merged upstream
  patch -Np1 -i ../0001-st-mesa-fix-reference-counting-bug-in-st_vdpau.patch
  patch -Np1 -i ../0002-vl-dri3-fix-a-memory-leak-from-front-buffer.patch
  # fix rendering in overlord series games - merged upstream
  patch -Np1 -i ../0001-Mesa-dev-st_glsl_to_tgsi-overlord-fix.patch
  # fix https://bugs.freedesktop.org/show_bug.cgi?id=71759 / FS#50240
  # reverted because of it segfault X with modesetting/intel 
  #patch -Np1 -i ../0001-i965-import-prime-buffers-in-the-current-context-not.patch

  autoreconf -fiv
}

build() {
  cd ${srcdir}/?esa-*

  [[ $CARCH == "armv7h" || $CARCH == "armv6h" ]] && VC4=',vc4'

  ./configure --prefix=/usr \
    --sysconfdir=/etc \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-gallium-drivers=freedreno,nouveau,swrast,virgl${VC4} \
    --with-dri-drivers=nouveau,swrast \
    --with-egl-platforms=x11,drm,wayland \
    --with-sha1=libgcrypt \
    --enable-gallium-llvm \
    --enable-llvm-shared-libs \
    --enable-shared-glapi \
    --enable-egl \
    --enable-glx \
    --enable-glx-tls \
    --enable-gles1 \
    --enable-gles2 \
    --enable-gbm \
    --enable-dri \
    --enable-osmesa \
    --enable-texture-float \
    --enable-omx \
    --enable-nine \
    --with-clang-libdir=/usr/lib

  make

  # fake installation
  mkdir $srcdir/fakeinstall
  make DESTDIR=${srcdir}/fakeinstall install
}

package_libva-mesa-driver() {
  pkgdesc="VA-API implementation for gallium"
  depends=('libdrm' 'libx11' 'llvm-libs' 'expat' 'elfutils')

  install -m755 -d ${pkgdir}/usr/lib
  cp -rv ${srcdir}/fakeinstall/usr/lib/dri ${pkgdir}/usr/lib
   
  install -m755 -d "${pkgdir}/usr/share/licenses/libva-mesa-driver"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libva-mesa-driver/"
}
               
package_mesa() {
  pkgdesc="an open-source implementation of the OpenGL specification"
  depends=('libdrm' 'wayland' 'libxxf86vm' 'libxdamage' 'libxshmfence' 'systemd' 'elfutils' 
           'libomxil-bellagio' 'expat' 'libgcrypt' 'libtxc_dxtn' 'llvm-libs')
  optdepends=('opengl-man-pages: for the OpenGL API man pages'
              'mesa-vdpau: for accelerated video playback'
              'libva-mesa-driver: for accelerated video playback')
  provides=('ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri')
  conflicts=('ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri')
  replaces=('ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri')

  install -m755 -d ${pkgdir}/etc
  cp -rv ${srcdir}/fakeinstall/etc/drirc ${pkgdir}/etc
  
  install -m755 -d ${pkgdir}/usr/lib/xorg/modules/dri
  # ati-dri, nouveau-dri, intel-dri, svga-dri, swrast
  cp -av ${srcdir}/fakeinstall/usr/lib/xorg/modules/dri/* ${pkgdir}/usr/lib/xorg/modules/dri
   
  cp -rv ${srcdir}/fakeinstall/usr/lib/bellagio  ${pkgdir}/usr/lib
  cp -rv ${srcdir}/fakeinstall/usr/lib/d3d  ${pkgdir}/usr/lib
  cp -rv ${srcdir}/fakeinstall/usr/lib/lib{gbm,glapi}.so* ${pkgdir}/usr/lib/
  cp -rv ${srcdir}/fakeinstall/usr/lib/libOSMesa.so* ${pkgdir}/usr/lib/
  cp -rv ${srcdir}/fakeinstall/usr/lib/libwayland*.so* ${pkgdir}/usr/lib/

  cp -rv ${srcdir}/fakeinstall/usr/include ${pkgdir}/usr
  cp -rv ${srcdir}/fakeinstall/usr/lib/pkgconfig ${pkgdir}/usr/lib/
  rm ${pkgdir}/usr/lib/pkgconfig/{egl,gl,glesv1_cm,glesv2}.pc

  # remove vulkan headers
  rm -rf ${pkgdir}/usr/include/vulkan

  install -m755 -d ${pkgdir}/usr/lib/mesa
  # move libgl/EGL/glesv*.so to not conflict with blobs - may break .pc files ?
  cp -rv ${srcdir}/fakeinstall/usr/lib/libGL.so* 	${pkgdir}/usr/lib/mesa/
  cp -rv ${srcdir}/fakeinstall/usr/lib/libEGL.so* 	${pkgdir}/usr/lib/mesa/
  cp -rv ${srcdir}/fakeinstall/usr/lib/libGLES*.so*	${pkgdir}/usr/lib/mesa/

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa/"
}

package_mesa-libgl() {
  pkgdesc="Mesa 3-D graphics library"
  depends=('mesa')
  provides=('libgl' 'libgles' 'libegl')
  conflicts=('libgl' 'libgles' 'libegl')

  install -m755 -d ${pkgdir}/usr/lib/pkgconfig
  cp ${srcdir}/fakeinstall/usr/lib/pkgconfig/{egl,gl,glesv1_cm,glesv2}.pc ${pkgdir}/usr/lib/pkgconfig
 
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
