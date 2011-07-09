# $Id: PKGBUILD 127561 2011-06-16 15:23:47Z andyrtr $
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@plugapps.com>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in plugs (intel, radeon, etc).
#  - Package for the swrast driver for those brave enough.

plugrel=1

pkgbase=mesa
pkgname=('mesa' 'libgl' 'libgles' 'libegl' 'swrast-dri')

#_git=true
_git=false

if [ "${_git}" = "true" ]; then
    pkgver=7.10.99.git20110612
  else
    pkgver=7.10.3
fi
pkgrel=1
arch=('i686' 'x86_64')
makedepends=('glproto>=1.4.12' 'pkgconfig' 'libdrm>=2.4.25' 'libxxf86vm>=1.1.1' 'libxdamage>=1.1.3' 'expat>=2.0.1' 'libx11>=1.4.3' 'libxt>=1.1.1' 
             'gcc-libs>=4.5' 'dri2proto=2.3' 'python2' 'libxml2' 'imake' 'llvm')
url="http://mesa3d.sourceforge.net"
license=('custom')
source=(LICENSE gnome-shell-shader-fix.patch nouveau-fix-header.patch)
if [ "${_git}" = "true" ]; then
	# mesa git shot from mastee (will become 7.11) branch - see for state: http://cgit.freedesktop.org/mesa/mesa/commit/?id=9a00dd974699e369b1eb292103fbde8bc6adfb87
	source=(${source[@]} 'ftp://ftp.archlinux.org/other/mesa/mesa-9a00dd974699e369b1eb292103fbde8bc6adfb87.tar.bz2')
  else
	source=(${source[@]} "ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/MesaLib-${pkgver}.zip"
)
fi
md5sums=('5c65a0fe315dd347e09b1f2826a1df5a'
         '3ec78f340f9387abd7a37b195e764cbf'
         '67c87b77cc2236b52a3b47dad3fbb5d4'
         '614d063ecd170940d9ae7b355d365d59')

build() {
if [ "${_git}" = "true" ]; then
    cd ${srcdir}/mesa-*   
    autoreconf -vfi
  else
    cd "${srcdir}/Mesa-${pkgver}" 
fi

if [ "${_git}" != "true" ]; then
#backport from master to fix gnome-shell shader
#https://bugs.freedesktop.org/show_bug.cgi?id=35714
patch -Np1 -i "${srcdir}/gnome-shell-shader-fix.patch"
patch -Np1 -i "${srcdir}/nouveau-fix-header.patch"
fi

if [ "${_git}" = "true" ]; then
    ./autogen.sh --prefix=/usr \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-dri-drivers="swrast" \
    --enable-gallium-swrast \
    --enable-glx-tls \
    --with-driver=dri \
    --enable-xcb \
    --with-state-trackers=dri,glx,egl \
    --disable-glut \
    --enable-gles1 \
    --enable-gles2 \
    --enable-egl \
    --enable-texture-float
    #    --enable-gallium-svga \

    # --enable-texture-float (enable floating-point textures and renderbuffers) - http://www.phoronix.com/scan.php?page=news_item&px=OTMzMg
    #The source code to implement ARB_texture_float extension is included and can be toggled on at compile time only by those who purchased a license from SGI, or are in a country where the patent does not apply.

    #--enable-shared-dricore - http://bugs.gentoo.org/show_bug.cgi?id=357177
    
  else
    ./configure --prefix=/usr \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-dri-drivers="swrast" \
    --enable-gallium-swrast \
    --enable-glx-tls \
    --with-driver=dri \
    --enable-xcb \
    --with-state-trackers=dri,glx \
    --disable-glut \
    --enable-gles1 \
    --enable-gles2 \
    --enable-egl \
    --disable-gallium-egl
fi

  make
}

package_libgl() {
  depends=('libdrm>=2.4.25' 'libxxf86vm>=1.1.1' 'libxdamage>=1.1.3' 'expat>=2.0.1')
  pkgdesc="Mesa 3-D graphics library and DRI software rasterizer"

if [ "${_git}" = "true" ]; then
    cd ${srcdir}/mesa-*   
  else
    cd "${srcdir}/Mesa-${pkgver}" 
fi
  install -m755 -d "${pkgdir}/usr/lib"
  install -m755 -d "${pkgdir}/usr/lib/xorg/modules/extensions"

  bin/minstall lib/libGL.so* "${pkgdir}/usr/lib/"

  cd src/mesa/drivers/dri
  #make -C swrast DESTDIR="${pkgdir}" install
if [ "${_git}" = "true" ]; then
    make -C ${srcdir}/mesa-*/src/gallium/targets/dri-swrast DESTDIR="${pkgdir}" install
  else
    make -C ${srcdir}/Mesa-${pkgver}/src/gallium/targets/dri-swrast DESTDIR="${pkgdir}" install
fi
  ln -s swrastg_dri.so "${pkgdir}/usr/lib/xorg/modules/dri/swrast_dri.so"
  ln -s libglx.xorg "${pkgdir}/usr/lib/xorg/modules/extensions/libglx.so"

  install -m755 -d "${pkgdir}/usr/share/licenses/libgl"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libgl/"
}

package_libgles() {
  depends=('libgl')
  pkgdesc="Mesa GLES libraries and headers"

if [ "${_git}" = "true" ]; then
    cd ${srcdir}/mesa-*   
  else
    cd "${srcdir}/Mesa-${pkgver}" 
fi
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
  depends=('libgl')
  pkgdesc="Mesa libEGL libraries and headers"

if [ "${_git}" = "true" ]; then
    cd ${srcdir}/mesa-*  
    make -C src/gallium/targets/egl DESTDIR="${pkgdir}" install
  else
    cd "${srcdir}/Mesa-${pkgver}" 
fi
  install -m755 -d "${pkgdir}/usr/lib"
  install -m755 -d "${pkgdir}/usr/lib/pkgconfig"
  install -m755 -d "${pkgdir}/usr/include"
  install -m755 -d "${pkgdir}/usr/include/"
  install -m755 -d "${pkgdir}/usr/include/EGL"
  install -m755 -d "${pkgdir}/usr/include/KHR"
  install -m755 -d "${pkgdir}/usr/share"
  install -m755 -d "${pkgdir}/usr/share/doc"
  install -m755 -d "${pkgdir}/usr/share/doc/libegl"
  bin/minstall lib/libEGL.so* "${pkgdir}/usr/lib/"
if [ "${_git}" != "true" ]; then
  install -m755 -d "${pkgdir}/usr/lib/egl"
  bin/minstall lib/egl/* "${pkgdir}/usr/lib/egl/"
fi
  bin/minstall src/egl/main/egl.pc "${pkgdir}/usr/lib/pkgconfig/"
  bin/minstall include/EGL/* "${pkgdir}/usr/include/EGL/"
  bin/minstall include/KHR/khrplatform.h "${pkgdir}/usr/include/KHR/"
  bin/minstall docs/egl.html "${pkgdir}/usr/share/doc/libegl/"

  install -m755 -d "${pkgdir}/usr/share/licenses/libegl"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libegl/"
}

package_mesa() {
  depends=('libgl' 'libx11>=1.4.3' 'libxt>=1.1.1' 'gcc-libs>=4.5' 'dri2proto=2.3' 'libdrm>=2.4.25' 'glproto>=1.4.12')
  optdepends=('opengl-man-pages: for the OpenGL API man pages')
  pkgdesc="Mesa 3-D graphics libraries and include files"

if [ "${_git}" = "true" ]; then
    cd ${srcdir}/mesa-*   
  else
    cd "${srcdir}/Mesa-${pkgver}" 
fi
  make DESTDIR="${pkgdir}" install

  rm -f "${pkgdir}/usr/lib/libGL.so"*
  rm -f "${pkgdir}/usr/lib/libGLESv"*
  rm -f "${pkgdir}/usr/lib/libEGL"*
  rm -rf "${pkgdir}/usr/lib/egl"
  rm -f ${pkgdir}/usr/lib/pkgconfig/{glesv1_cm.pc,glesv2.pc,egl.pc}
  rm -rf "${pkgdir}/usr/lib/xorg"
  rm -f "${pkgdir}/usr/include/GL/glew.h"
  rm -f "${pkgdir}/usr/include/GL/glxew.h"
  rm -f "${pkgdir}/usr/include/GL/wglew.h"
  rm -f "${pkgdir}/usr/include/GL/glut.h"
  rm -rf ${pkgdir}/usr/include/{GLES,GLES2,EGL,KHR}

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa/"
}

package_swrast-dri() {
  depends=("libgl=${pkgver}")
  pkgdesc="Mesa DRI + Gallium3D swrast drivers"

if [ "${_git}" = "true" ]; then
    cd ${srcdir}/mesa-*/src/mesa/drivers/dri
  else
    cd "${srcdir}/Mesa-${pkgver}/src/mesa/drivers/dri"
fi
  make -C swrast DESTDIR="${pkgdir}" install
  # gallium3D driver for swrast
  make -C ${srcdir}/Mesa-${pkgver}/src/gallium/targets/dri-swrast DESTDIR="${pkgdir}" install
}
