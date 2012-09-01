# $Id: PKGBUILD 153287 2012-03-12 20:52:14Z andyrtr $
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, radeon, VMware svga).
#  - Build v7h with -O1 instead of -O2

pkgbase=mesa
pkgname=('mesa' 'libgl' 'osmesa' 'libglapi' 'libgbm' 'libgles' 'libegl' 'khrplatform-devel')

#_git=true
_gitdate=20111031
_git=false

if [ "${_git}" = "true" ]; then
    pkgver=7.10.99.git20110709
    #pkgver=7.11
  else
    pkgver=8.0.4
fi
pkgrel=3.1
arch=('i686' 'x86_64')
makedepends=('glproto>=1.4.15' 'libdrm>=2.4.30' 'libxxf86vm>=1.1.1' 'libxdamage>=1.1.3' 'expat>=2.0.1' 'libx11>=1.4.99.1' 'libxt>=1.1.1' 
             'gcc-libs>=4.7.1-5' 'dri2proto>=2.6' 'python2' 'libxml2' 'imake' 'llvm' 'systemd-tools')
url="http://mesa3d.sourceforge.net"
license=('custom')
source=(LICENSE
        mesa-8.0.3-llvm-3.1-fixes.patch)
if [ "${_git}" = "true" ]; then
	# mesa git shot from 7.11 branch - see for state: http://cgit.freedesktop.org/mesa/mesa/commit/?h=7.11&id=1ae00c5960af83bea9545a18a1754bad83d5cbd0
	#source=(${source[@]} 'ftp://ftp.archlinux.org/other/mesa/mesa-1ae00c5960af83bea9545a18a1754bad83d5cbd0.tar.bz2')
	source=(${source[@]} "MesaLib-git${_gitdate}.zip"::"http://cgit.freedesktop.org/mesa/mesa/snapshot/mesa-ef9f16f6322a89fb699fbe3da868b10f9acaef98.tar.bz2")
  else
	source=(${source[@]} "ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/MesaLib-${pkgver}.tar.bz2"
	#source=(${source[@]} "ftp://ftp.freedesktop.org/pub/mesa/8.0/MesaLib-8.0-rc2.tar.bz2"
	#source=(${source[@]} "MesaLib-git${_gitdate}.zip"::"http://cgit.freedesktop.org/mesa/mesa/snapshot/mesa-4464ee1a9aa3745109cee23531e3fb2323234d07.tar.bz2"
)
fi
md5sums=('5c65a0fe315dd347e09b1f2826a1df5a'
         'c452ed3392468170726c004c2f4e02ca'
         'd546f988adfdf986cff45b1efa2d8a46')

build() {
    cd ${srcdir}/?esa-*

    patch -Np1 -i "${srcdir}/mesa-8.0.3-llvm-3.1-fixes.patch"

    [ "${CARCH}" = "armv7h" ] && CFLAGS=`echo $CFLAGS | sed -e 's/-O2/-O1/'` && CXXFLAGS="$CFLAGS"

if [ "${_git}" = "true" ]; then
    autoreconf -vfi
    ./autogen.sh --prefix=/usr \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-gallium-drivers=swrast \
    --enable-gallium-llvm \
    --enable-egl \
    --enable-gallium-egl \
    --with-egl-platforms=x11,drm \
    --enable-shared-glapi \
    --enable-gbm \
    --enable-glx-tls \
    --enable-dri \
    --enable-glx \
    --enable-osmesa \
    --enable-gles1 \
    --enable-gles2 \
    --enable-texture-float \
    --enable-xa \
    --enable-shared-dricore
    
    #    --enable-gallium-svga \
    
  else
     autoreconf -vfi
    ./configure --prefix=/usr \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-dri-drivers=swrast \
    --with-gallium-drivers=swrast \
    --enable-gallium-llvm \
    --enable-egl \
    --enable-gallium-egl \
    --with-egl-platforms=x11,drm \
    --enable-shared-glapi \
    --enable-gbm \
    --enable-glx-tls \
    --enable-dri \
    --enable-glx \
    --enable-osmesa \
    --enable-gles1 \
    --enable-gles2 \
    --enable-texture-float \
    --enable-xa \
    --enable-shared-dricore
fi

  make
}

package_libgl() {
  depends=('libdrm>=2.4.31' 'libxxf86vm>=1.1.1' 'libxdamage>=1.1.3' 'expat>=2.0.1' 'libglapi' 'gcc-libs')
  pkgdesc="Mesa 3-D graphics library and DRI software rasterizer"
  #replaces=('unichrome-dri' 'mach64-dri' 'mga-dri' 'r128-dri' 'savage-dri' 'sis-dri' 'tdfx-dri')

  cd ${srcdir}/?esa-*
  install -m755 -d "${pkgdir}/usr/lib"
  install -m755 -d "${pkgdir}/usr/lib/xorg/modules/extensions"

  bin/minstall lib/libGL.so* "${pkgdir}/usr/lib/"
  bin/minstall lib/libdricore.so* "${pkgdir}/usr/lib/"
  bin/minstall lib/libglsl.so* "${pkgdir}/usr/lib/"

  cd src/mesa/drivers/dri
  make -C ${srcdir}/?esa-*/src/gallium/targets/dri-swrast DESTDIR="${pkgdir}" install

  ln -s libglx.xorg "${pkgdir}/usr/lib/xorg/modules/extensions/libglx.so"

  install -m755 -d "${pkgdir}/usr/share/licenses/libgl"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libgl/"
}

package_osmesa() {
  depends=('mesa')
  optdepends=('opengl-man-pages: for the OpenGL API man pages')
  pkgdesc="Mesa 3D off-screen rendering library"
  
  make -C ${srcdir}/?esa-*/src/mesa DESTDIR="${pkgdir}" install-osmesa
}

package_libglapi() {
  depends=('glibc')
  pkgdesc="free implementation of the GL API -- shared library. The Mesa GL API module is responsible for dispatching all the gl* functions"

  cd ${srcdir}/?esa-*   
  install -m755 -d "${pkgdir}/usr/lib"
  bin/minstall lib/libglapi.so* "${pkgdir}/usr/lib/"

  install -m755 -d "${pkgdir}/usr/share/licenses/libglapi"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libglapi/"
}

package_libgbm() {
  depends=('systemd-tools')
  pkgdesc="Mesa gbm library"

  cd ${srcdir}/?esa-*
  install -m755 -d ${pkgdir}/usr/{include,lib/gbm}
  bin/minstall lib/libgbm.so* "${pkgdir}/usr/lib/"
  bin/minstall src/gbm/main/gbm.h "${pkgdir}/usr/include/"
  bin/minstall lib/libgbm.so* "${pkgdir}/usr/lib/"
  bin/minstall lib/gbm/gbm_gallium_drm.so* "${pkgdir}/usr/lib/gbm/"
  install -m755 -d "${pkgdir}/usr/lib/pkgconfig"
  bin/minstall src/gbm/main/gbm.pc "${pkgdir}/usr/lib/pkgconfig/"

  install -m755 -d "${pkgdir}/usr/share/licenses/libgbm"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libgbm/"
}

package_libgles() {
  depends=('libglapi' 'khrplatform-devel')
  pkgdesc="Mesa GLES libraries and headers"

  cd ${srcdir}/?esa-*   
  install -m755 -d "${pkgdir}/usr/lib"
  install -m755 -d "${pkgdir}/usr/lib/pkgconfig"
  install -m755 -d "${pkgdir}/usr/include"
  install -m755 -d "${pkgdir}/usr/include/GLES"
  install -m755 -d "${pkgdir}/usr/include/GLES2"
  bin/minstall lib/libGLESv* "${pkgdir}/usr/lib/"
  bin/minstall src/mapi/es1api/glesv1_cm.pc "${pkgdir}/usr/lib/pkgconfig/"
  bin/minstall src/mapi/es2api/glesv2.pc "${pkgdir}/usr/lib/pkgconfig/"
  bin/minstall include/GLES/* "${pkgdir}/usr/include/GLES/"
  bin/minstall include/GLES2/* "${pkgdir}/usr/include/GLES2/"
  bin/minstall include/GLES2/* "${pkgdir}/usr/include/GLES2/"

  install -m755 -d "${pkgdir}/usr/share/licenses/libgles"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libgles/"
}

package_libegl() {
  depends=('libglapi' 'libdrm' 'libxext' 'libxfixes' 'libgbm' 'khrplatform-devel')
  pkgdesc="Mesa EGL libraries and headers"

  cd ${srcdir}/?esa-*   
  make -C src/gallium/targets/egl-static DESTDIR="${pkgdir}" install
  install -m755 -d "${pkgdir}/usr/lib"
  install -m755 -d "${pkgdir}/usr/lib/pkgconfig"
  install -m755 -d "${pkgdir}/usr/include"
  install -m755 -d "${pkgdir}/usr/include/"
  install -m755 -d "${pkgdir}/usr/include/EGL"
  install -m755 -d "${pkgdir}/usr/share"
  install -m755 -d "${pkgdir}/usr/share/doc"
  install -m755 -d "${pkgdir}/usr/share/doc/libegl"
  bin/minstall lib/libEGL.so* "${pkgdir}/usr/lib/"
  install -m755 -d "${pkgdir}/usr/lib/egl"
  bin/minstall lib/egl/* "${pkgdir}/usr/lib/egl/"
  bin/minstall src/egl/main/egl.pc "${pkgdir}/usr/lib/pkgconfig/"
  bin/minstall include/EGL/* "${pkgdir}/usr/include/EGL/"
  bin/minstall docs/egl.html "${pkgdir}/usr/share/doc/libegl/"

  install -m755 -d "${pkgdir}/usr/share/licenses/libegl"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libegl/"
}

package_khrplatform-devel() {
  #depends=('')
  pkgdesc="Khronos platform development package"

  cd ${srcdir}/?esa-*
  install -m755 -d "${pkgdir}/usr/include/KHR"
  bin/minstall include/KHR/khrplatform.h "${pkgdir}/usr/include/KHR/" 

  install -m755 -d "${pkgdir}/usr/share/licenses/khrplatform-devel"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/khrplatform-devel/"
}

package_mesa() {
  depends=('libgl' 'libx11>=1.4.3' 'libxt>=1.1.1' 'gcc-libs>=4.6' 'dri2proto>=2.6' 'glproto>=1.4.14') #dri2proto + glproto needed for gl.pc
  optdepends=('opengl-man-pages: for the OpenGL API man pages')
  pkgdesc="Mesa 3-D graphics libraries and include files"

  cd ${srcdir}/?esa-*   
  make DESTDIR="${pkgdir}" install

  rm -f "${pkgdir}/usr/lib/libGL.so"*
  rm -f "${pkgdir}/usr/lib/libglapi.so"*
  rm -f "${pkgdir}/usr/lib/libgbm.so"*
  rm -f "${pkgdir}/usr/lib/libGLESv"*
  rm -f "${pkgdir}/usr/lib/libEGL"*
  rm -rf "${pkgdir}/usr/lib/egl"
  rm -f "${pkgdir}/usr/lib/libOSMesa"*
  rm -rf "${pkgdir}/usr/lib/gbm"
  rm -f ${pkgdir}/usr/lib/pkgconfig/{glesv1_cm.pc,glesv2.pc,egl.pc,osmesa.pc,gbm.pc}
  rm -rf "${pkgdir}/usr/lib/xorg"
  rm -f "${pkgdir}/usr/include/GL/glew.h"
  rm -f "${pkgdir}/usr/include/GL/glxew.h"
  rm -f "${pkgdir}/usr/include/GL/wglew.h"
  rm -f "${pkgdir}/usr/include/GL/glut.h"
  rm -f "${pkgdir}/usr/include/gbm.h"
  rm -rf ${pkgdir}/usr/include/{GLES,GLES2,EGL,KHR}

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa/"
}
