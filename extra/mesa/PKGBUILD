# $Id: PKGBUILD 105443 2011-01-08 22:54:32Z andyrtr $
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# PlugApps: Kevin Mihelich <kevin@plugapps.com>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in plugs (intel, radeon, etc).
#  - Package for the swrast driver for those brave enough.

plugrel=1

pkgbase=mesa
pkgname=('mesa' 'libgl' 'swrast-dri')
pkgver=7.10
#pkgver=7.9.99.git20101230
pkgrel=1
arch=('i686' 'x86_64')
makedepends=('glproto>=1.4.12' 'pkgconfig' 'libdrm>=2.4.23' 'libxxf86vm>=1.1.0' 'libxdamage>=1.1.3' 'expat>=2.0.1' 'libx11>=1.3.5' 'libxt>=1.0.8' 
             'gcc-libs>=4.5' 'dri2proto=2.3' 'python2' 'talloc' 'libxml2' 'imake')
url="http://mesa3d.sourceforge.net"
license=('custom')
source=(ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/MesaLib-${pkgver}.tar.bz2
	# mesa git shot from 7.10 branch - see for state: http://cgit.freedesktop.org/mesa/mesa/commit/?h=7.10&id=aa196d047c2d42835f3f6f25ac304b312f33f4f9
	#ftp://ftp.archlinux.org/other/mesa/mesa-aa196d047c2d42835f3f6f25ac304b312f33f4f9.tar.bz2
        ftp://ftp.archlinux.org/other/mesa/gl-manpages-1.0.1.tar.bz2
        LICENSE)
md5sums=('33fb94eccc02cbb4d8d1365615e38e46'
         '6ae05158e678f4594343f32c2ca50515'
         '5c65a0fe315dd347e09b1f2826a1df5a')

build() {
  cd "${srcdir}/Mesa-${pkgver}"
#  cd ${srcdir}/mesa-*

  # required for git build
#  autoreconf -vfi

  # python2 build fixes
  sed -i -e "s|#![ ]*/usr/bin/python$|#!/usr/bin/python2|" \
         -e "s|#![ ]*/usr/bin/env python$|#!/usr/bin/env python2|" $(find $srcdir -name '*.py')
  sed -i -e "s|PYTHON2 = python|PYTHON2 = python2|" configs/{default,autoconf.in}
  sed -i -e "s|python|python2|" src/gallium/auxiliary/Makefile

#  ./autogen.sh --prefix=/usr \
  ./configure --prefix=/usr \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-dri-drivers="swrast" \
    --disable-egl \
    --enable-gallium-swrast \
    --enable-glx-tls \
    --with-driver=dri \
    --enable-xcb \
    --with-state-trackers=dri,glx \
    --disable-glut
  make

  cd "${srcdir}/gl-manpages-1.0.1"
  ./configure --prefix=/usr
  make
}

package_libgl() {
  depends=('libdrm>=2.4.22' 'libxxf86vm>=1.1.0' 'libxdamage>=1.1.3' 'expat>=2.0.1')
  pkgdesc="Mesa 3-D graphics library and DRI software rasterizer"

  cd "${srcdir}/Mesa-${pkgver}"
#  cd ${srcdir}/mesa-*
  install -m755 -d "${pkgdir}/usr/lib"
  install -m755 -d "${pkgdir}/usr/lib/xorg/modules/extensions"

  bin/minstall lib/libGL.so* "${pkgdir}/usr/lib/"

  cd src/mesa/drivers/dri
  make -C swrast DESTDIR="${pkgdir}" install
  ln -s libglx.xorg "${pkgdir}/usr/lib/xorg/modules/extensions/libglx.so"

  install -m755 -d "${pkgdir}/usr/share/licenses/libgl"
  install -m755 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libgl/"
}

package_mesa() {
  depends=('libgl' 'libx11>=1.3.5' 'libxt>=1.0.8' 'gcc-libs>=4.5' 'dri2proto=2.3' 'libdrm>=2.4.22' 'glproto>=1.4.12')
  pkgdesc="Mesa 3-D graphics libraries and include files"

  cd "${srcdir}/Mesa-${pkgver}"
#  cd ${srcdir}/mesa-*
  make DESTDIR="${pkgdir}" install

  rm -f "${pkgdir}/usr/lib/libGL.so"*
  rm -rf "${pkgdir}/usr/lib/xorg"
  rm -f "${pkgdir}/usr/include/GL/glew.h"
  rm -f "${pkgdir}/usr/include/GL/glxew.h"
  rm -f "${pkgdir}/usr/include/GL/wglew.h"
  rm -f "${pkgdir}/usr/include/GL/glut.h"

  cd "${srcdir}/gl-manpages-1.0.1"
  make DESTDIR="${pkgdir}" install

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa"
  install -m755 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa/"
}

package_swrast-dri() {
  depends=("libgl=${pkgver}")
  pkgdesc="Mesa DRI + Gallium3D swrast drivers"

  cd "${srcdir}/Mesa-${pkgver}/src/mesa/drivers/dri"
  make -C swrast DESTDIR="${pkgdir}" install
  # gallium3D driver for swrast
  make -C ${srcdir}/Mesa-${pkgver}/src/gallium/targets/dri-swrast DESTDIR="${pkgdir}" install
}
