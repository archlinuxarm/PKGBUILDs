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
    pkgver=9.0.1
fi
pkgrel=1
arch=('i686' 'x86_64')
makedepends=('glproto>=1.4.16' 'libdrm>=2.4.39' 'libxxf86vm>=1.1.2' 'libxdamage>=1.1.3' 'expat>=2.1.0' 'libx11>=1.5.0' 'libxt>=1.1.3'
             'gcc-libs>=4.7.1-6' 'dri2proto>=2.8' 'python2' 'libxml2' 'imake' 'llvm' 'systemd' 'libvdpau>=0.5')
url="http://mesa3d.sourceforge.net"
license=('custom')
options=('!libtool')
source=(LICENSE)
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
         '97d6554c05ea7449398afe3a0ede7018')

build() {
    cd ${srcdir}/?esa-*

    [ "${CARCH}" = "armv7h" ] && CFLAGS=`echo $CFLAGS | sed -e 's/-O2/-O1/'` && CXXFLAGS="$CFLAGS"

    COMMONOPTS="--prefix=/usr \
    --sysconfdir=/etc \
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
    --enable-xa "

  if [ "${_git}" = "true" ]; then
    ./autogen.sh \
      $COMMONOPTS
  else
    autoreconf -vfi
    ./configure \
      $COMMONOPTS
  fi

  make
}

package_libglapi() {
  depends=('glibc')
  pkgdesc="free implementation of the GL API -- shared library. The Mesa GL API module is responsible for dispatching all the gl* functions"

  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" install

  install -m755 -d "${pkgdir}/usr/share/licenses/libglapi"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libglapi/"
}

package_libgl() {
  depends=('libdrm>=2.4.39' 'libxxf86vm>=1.1.2' 'libxdamage>=1.1.3' 'expat>=2.1.0' 'libglapi' 'gcc-libs')
  pkgdesc="Mesa 3-D graphics library and DRI software rasterizer"

  # fix linking because of splitted package
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" install

  # libGL & libdricore
  make -C ${srcdir}/?esa-*/src/glx DESTDIR="${pkgdir}" install
  make -C ${srcdir}/?esa-*/src/mesa/libdricore DESTDIR="${pkgdir}" install

  # fix linking because of splitted package - cleanup
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" uninstall


  make -C ${srcdir}/?esa-*/src/gallium/targets/dri-swrast DESTDIR="${pkgdir}" install

  # See FS#26284
  install -m755 -d "${pkgdir}/usr/lib/xorg/modules/extensions"
  ln -s libglx.xorg "${pkgdir}/usr/lib/xorg/modules/extensions/libglx.so"

  install -m755 -d "${pkgdir}/usr/share/licenses/libgl"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libgl/"
}

package_mesa() {
  # check also gl.pc
  depends=('libgl' 'libx11>=1.5.0' 'libxext>=1.3.1' 'libxdamage' 'libxfixes' 'libxcb' 'libxxf86vm')
  optdepends=('opengl-man-pages: for the OpenGL API man pages')
  pkgdesc="Mesa 3-D graphics libraries and include files"

  make -C ${srcdir}/?esa-*/src/mesa DESTDIR="${pkgdir}" install-glHEADERS
  make -C ${srcdir}/?esa-*/src/mesa/drivers/dri DESTDIR="${pkgdir}" install-driincludeHEADERS
  make -C ${srcdir}/?esa-*/src/mesa DESTDIR="${pkgdir}" install-pkgconfigDATA
  make -C ${srcdir}/?esa-*/src/mesa/drivers/dri DESTDIR="${pkgdir}" install-pkgconfigDATA
  make -C ${srcdir}/?esa-*/src/mesa/drivers/dri/common DESTDIR="${pkgdir}" install-sysconfDATA

  #make -C ${srcdir}/?esa-*/src/gallium/targets/xa-vmwgfx DESTDIR="${pkgdir}" install

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa/"
}

package_osmesa() {
  depends=('libglapi' 'gcc-libs')
  optdepends=('opengl-man-pages: for the OpenGL API man pages')
  pkgdesc="Mesa 3D off-screen rendering library"

  # fix linking because of splitted package
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" install

  make -C ${srcdir}/?esa-*/src/mesa/drivers/osmesa DESTDIR="${pkgdir}" install

  # fix linking because of splitted package - cleanup
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" uninstall

  install -m755 -d "${pkgdir}/usr/share/licenses/osmesa"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/osmesa/"
}

package_libgbm() {
  depends=('systemd' 'libglapi' 'libdrm')
  pkgdesc="Mesa gbm library"

  # fix linking because of splitted package
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" install

  make -C ${srcdir}/?esa-*/src/gbm DESTDIR="${pkgdir}" install

  # fix linking because of splitted package - cleanup
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" uninstall

  install -m755 -d "${pkgdir}/usr/share/licenses/libgbm"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libgbm/"
}

package_libgles() {
  depends=('libglapi' 'libdrm' 'khrplatform-devel')
  pkgdesc="Mesa GLES libraries and headers"

  # fix linking because of splitted package
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" install

  make -C ${srcdir}/?esa-*/src/mapi/es1api DESTDIR="${pkgdir}" install
  make -C ${srcdir}/?esa-*/src/mapi/es2api DESTDIR="${pkgdir}" install

  # fix linking because of splitted package - cleanup
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" uninstall

  install -m755 -d "${pkgdir}/usr/share/licenses/libgles"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libgles/"
}

package_libegl() {
  # check also egl.pc
  depends=('libx11' 'libxext' 'libxdamage' 'libxfixes' 'libxxf86vm' 'libxcb' 'libgbm' 'khrplatform-devel')
  pkgdesc="Mesa EGL libraries and headers"

  make -C ${srcdir}/?esa-*/src/gallium/targets/egl-static DESTDIR="${pkgdir}" install
  install -m755 -d "${pkgdir}/usr/share/doc/libegl"
  install -m644 ${srcdir}/?esa-*/docs/egl.html "${pkgdir}/usr/share/doc/libegl/"

  # fix linking because of splitted package
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" install
  make -C ${srcdir}/?esa-*/src/gbm DESTDIR="${pkgdir}" install

  make -C ${srcdir}/?esa-*/src/egl DESTDIR="${pkgdir}" install

  # fix linking because of splitted package - cleanup
  make -C ${srcdir}/?esa-*/src/gbm DESTDIR="${pkgdir}" uninstall
  make -C ${srcdir}/?esa-*/src/mapi/shared-glapi DESTDIR="${pkgdir}" uninstall

  install -m755 -d "${pkgdir}/usr/share/licenses/libegl"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libegl/"

  # fix file conflicts
  rm -rf ${pkgdir}/usr/include/KHR
}

package_khrplatform-devel() {
  pkgdesc="Khronos platform development package"

  install -m755 -d "${pkgdir}/usr/include/KHR"
  install -m644 ${srcdir}/?esa-*/include/KHR/khrplatform.h "${pkgdir}/usr/include/KHR/"

  install -m755 -d "${pkgdir}/usr/share/licenses/khrplatform-devel"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/khrplatform-devel/"
}
