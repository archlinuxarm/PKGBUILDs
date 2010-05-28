# Maintainer: Mike Staszel <mikestaszel@plugboxlinux.org>

# Plugbox Modifications:
# Removed ATI, SIS, TDFX

pkgbase="mesa"
pkgname=('mesa' 'libgl' 'unichrome-dri' 'mga-dri' 'savage-dri')
pkgver=7.7.1
pkgrel=1
arch=(arm)
makedepends=('glproto>=1.4.11' 'pkgconfig' 'libdrm>=2.4.19' 'libxxf86vm>=1.1.0' 'libxdamage>=1.1.2' 'expat>=2.0.1' 'libx11>=1.3.3' 'libxt>=1.0.7' 
'gcc-libs>=4.4.3' 'dri2proto=2.1' 'python')
url="http://mesa3d.sourceforge.net"
license=('custom')
options=(!makeflags)
source=(ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/MesaLib-${pkgver}.tar.bz2
	#ftp://ftp.freedesktop.org/pub/mesa/${pkgver/.0.902//}RC/MesaLib-${pkgver/0.902/1-rc2}.tar.bz2
        ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/MesaDemos-${pkgver}.tar.bz2
	#ftp://ftp.freedesktop.org/pub/mesa/${pkgver/.0.902//}RC/MesaDemos-${pkgver/0.902/1-rc2}.tar.bz2
        ftp://ftp.archlinux.org/other/mesa/gl-manpages-1.0.1.tar.bz2
        mesa-7.1-link-shared.patch
        intel-revert-vbl.patch
        LICENSE)
md5sums=('46664d99e03f1e3ac078a7fea02af115'
         'aeb39645d80d656e0adebaa09e5bcd03'
         '6ae05158e678f4594343f32c2ca50515'
         '8420bed348e7016ef03cac6545d68389'
         'a111f4dc82e894f8801bc3fa129af7af'
         '5c65a0fe315dd347e09b1f2826a1df5a')

build() {
  cd "${srcdir}/Mesa-${pkgver}"
  #cd "${srcdir}/Mesa-${pkgver/0.902/1-rc2}"
  patch -Np1 -i "${srcdir}/mesa-7.1-link-shared.patch" || return 1
  patch -Np1 -i "${srcdir}/intel-revert-vbl.patch" || return 1
  ./configure --prefix=/usr \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-dri-drivers=swrast,unichrome,mga,savage \
    --disable-gallium-intel \
    --enable-glx-tls \
    --with-driver=dri \
    --enable-xcb \
    --disable-glut --disable-gallium || return 1

#    --with-state-trackers=dri,egl \
#    --enable-gallium-nouveau \

#    --with-state-trackers=yes \   # gentoo: glx,dri,egl
#  --disable-gallium       build gallium [default=enabled]
#  --disable-gallium-intel build gallium intel [default=enabled]
#  --enable-gallium-radeon build gallium radeon [default=disabled]
#  --enable-gallium-nouveau build gallium nouveau [default=disabled]

#ls src/Mesa-7.6/src/mesa/drivers/dri/
#common  dri.pc.in  fb  ffb  gamma  glcore  i810  i915  i965  intel  mach64  mga  r128  r200  r300  r600  radeon  s3v  savage  sis  swrast  tdfx  trident  unichrome
#ls src/Mesa-7.6/src/gallium/drivers/
#cell  failover  i915simple  i965simple  identity  llvmpipe  nouveau  nv04  nv10  nv20  nv30  nv40  nv50  r300  softpipe  trace

    
  make || return 1

  cd "${srcdir}/gl-manpages-1.0.1"
  ./configure --prefix=/usr || return 1
  make || return 1
}

package_libgl() {
  depends=('libdrm>=2.4.17-2' 'libxxf86vm>=1.1.0' 'libxdamage>=1.1.2' 'expat>=2.0.1')
  pkgdesc="Mesa 3-D graphics library and DRI software rasterizer"

  cd "${srcdir}/Mesa-${pkgver}" || return 1
#  cd "${srcdir}/Mesa-${pkgver/0.902/1-rc2}" || return 1
  install -m755 -d "${pkgdir}/usr/lib" || return 1
  install -m755 -d "${pkgdir}/usr/lib/xorg/modules/extensions"

  bin/minstall lib/libGL.so* "${pkgdir}/usr/lib/" || return 1

  cd src/mesa/drivers/dri
  make -C swrast DESTDIR="${pkgdir}" install || return 1
  install -m755 libdricore.so "${pkgdir}/usr/lib/xorg/modules/dri/" || return 1
  ln -s libglx.xorg "${pkgdir}/usr/lib/xorg/modules/extensions/libglx.so" || return 1

  install -m755 -d "${pkgdir}/usr/share/licenses/libgl"
  install -m755 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libgl/" || return 1
}

package_mesa() {
  depends=('libgl' 'libx11>=1.3.2' 'libxt>=1.0.7' 'gcc-libs>=4.4.2' 'dri2proto=2.1' 'libdrm>=2.4.17-2')
  pkgdesc="Mesa 3-D graphics libraries and include files"

  cd "${srcdir}/Mesa-${pkgver}" || return 1
#  cd "${srcdir}/Mesa-${pkgver/0.902/1-rc2}" || return 1
  make DESTDIR="${pkgdir}" install || return 1
  install -m755 -d "${pkgdir}/usr/bin"
  install -m755 progs/xdemos/glx{gears,info} "${pkgdir}/usr/bin/" || return 1

  rm -f "${pkgdir}/usr/lib/libGL.so"*
  rm -rf "${pkgdir}/usr/lib/xorg"
  rm -f "${pkgdir}/usr/include/GL/glew.h"
  rm -f "${pkgdir}/usr/include/GL/glxew.h"
  rm -f "${pkgdir}/usr/include/GL/wglew.h"

  cd "${srcdir}/gl-manpages-1.0.1" || return 1
  make DESTDIR="${pkgdir}" install || return 1

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa"
  install -m755 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa/" || return 1
}

package_unichrome-dri() {
  depends=("libgl=${pkgver}")
  pkgdesc="Mesa DRI drivers for S3 Graphics/VIA Unichrome"

  cd "${srcdir}/Mesa-${pkgver}/src/mesa/drivers/dri" || return 1
#  cd "${srcdir}/Mesa-${pkgver/0.902/1-rc2}/src/mesa/drivers/dri" || return 1
  make -C unichrome DESTDIR="${pkgdir}" install || return 1
}

package_mga-dri() {
  depends=("libgl=${pkgver}")
  pkgdesc="Mesa DRI drivers for Matrox"
  conflicts=('xf86-video-mga<1.4.11')

  cd "${srcdir}/Mesa-${pkgver}/src/mesa/drivers/dri" || return 1
#  cd "${srcdir}/Mesa-${pkgver/0.902/1-rc2}/src/mesa/drivers/dri" || return 1
  make -C mga DESTDIR="${pkgdir}" install || return 1
}

package_savage-dri() {
  depends=("libgl=${pkgver}")
  pkgdesc="Mesa DRI drivers for S3 Sraphics/VIA Savage"
  conflicts=('xf86-video-savage<2.3.1')

  cd "${srcdir}/Mesa-${pkgver}/src/mesa/drivers/dri" || return 1
#  cd "${srcdir}/Mesa-${pkgver/0.902/1-rc2}/src/mesa/drivers/dri" || return 1
  make -C savage DESTDIR="${pkgdir}" install || return 1
}
