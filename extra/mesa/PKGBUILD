# $Id$
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, radeon, VMware svga).
#  - Build v7h with -O1 instead of -O2
#  - Removed libgles, libegl and khrplatform-devel from conflicts for marvell-libgfx compatibility.

pkgbase=mesa
pkgname=('mesa' 'mesa-libgl')
pkgver=9.1.2
pkgrel=1
arch=('i686' 'x86_64')
makedepends=('python2' 'libxml2' 'libx11' 'glproto' 'libdrm' 'dri2proto' 'libxxf86vm' 'libxdamage'
             'libvdpau' 'wayland' 'systemd')
url="http://mesa3d.sourceforge.net"
license=('custom')
options=('!libtool')
source=(ftp://ftp.freedesktop.org/pub/mesa/${pkgver}/MesaLib-${pkgver}.tar.bz2
        #ftp://ftp.freedesktop.org/pub/mesa/9.1/MesaLib-9.1-rc2.tar.bz2 # for RC testing
        LICENSE)
md5sums=('df2aab86ff4a510ce5b0d074caa0a59f'
         'c3e45fe7287bbf8f620c209a872330dc'
         '5c65a0fe315dd347e09b1f2826a1df5a')

build() {
    cd ${srcdir}/?esa-*

    # pick 2 commits from master to
    # fix a nouveau crash: http://cgit.freedesktop.org/mesa/mesa/commit/?id=17f1cb1d99e66227d1e05925ef937643f5c1089a
    # and intel kwin slowness http://cgit.freedesktop.org/mesa/mesa/commit/?id=e062a4187d8ea518a39c913ae7562cf1d8ac3205
    #patch -Np1 -i ${srcdir}/git-fixes.patch

    autoreconf -vfi # our automake is far too new for their build system :)

    ./configure --prefix=/usr \
    --sysconfdir=/etc \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-gallium-drivers=swrast \
    --with-dri-drivers=swrast \
    --enable-gallium-llvm \
    --enable-egl \
    --enable-gallium-egl \
    --with-egl-platforms=x11,drm,wayland \
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
    # --with-llvm-shared-libs \ # enabling this would force us to move llvm-amdgpu-snapshot from community to extra, delay it until llvm 3.3 / Mesa 9.2/10.0

    make
    # fake installation
	mkdir $srcdir/fakeinstall
	make DESTDIR=${srcdir}/fakeinstall install
}

package_mesa() {
  pkgdesc="an open-source implementation of the OpenGL specification"
  depends=('libdrm' 'libvdpau' 'wayland' 'libxxf86vm' 'libxdamage' 'systemd')
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
