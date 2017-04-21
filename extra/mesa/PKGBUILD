# $Id$
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, radeon, VMware svga).

pkgbase=mesa
pkgname=('mesa' 'libva-mesa-driver')
pkgver=17.0.4
pkgrel=2
arch=('i686' 'x86_64')
makedepends=('python2-mako' 'libxml2' 'libx11' 'glproto' 'libdrm' 'dri2proto' 'dri3proto' 'presentproto' 
             'libxshmfence' 'libxxf86vm' 'libxdamage' 'libvdpau' 'libva' 'wayland' 'elfutils' 'llvm'
             'libomxil-bellagio' 'clang' 'libglvnd')
url="http://mesa3d.sourceforge.net"
license=('custom')
source=(https://mesa.freedesktop.org/archive/mesa-${pkgver}.tar.xz{,.sig}
        LICENSE
        remove-libpthread-stubs.patch
        0001-EGL-Implement-the-libglvnd-interface-for-EGL-v2.patch
        0001-Fix-linkage-against-shared-glapi.patch
        0001-glapi-Link-with-glapi-when-built-shared.patch
        0002-fixup-EGL-Implement-the-libglvnd-interface-for-EGL-v.patch
        glvnd-fix-gl-dot-pc.patch)
sha256sums=('1269dc8545a193932a0779b2db5bce9be4a5f6813b98c38b93b372be8362a346'
            'SKIP'
            '7fdc119cf53c8ca65396ea73f6d10af641ba41ea1dd2bd44a824726e01c8b3f2'
            '75ab53ad44b95204c788a2988e97a5cb963bdbf6072a5466949a2afb79821c8f'
            '1d3475dc2f4f3e450cf313130d3ce965f933f396058828fa843c0df8115feeb9'
            'c68d1522f9bce4ce31c92aa7a688da49f13043f5bb2254795b76dea8f47130b7'
            '064dcd5a3ab1b7c23383e2cafbd37859e4c353f8839671d9695c6f7c2ef3260b'
            '81d0ced62f61677ea0cf5f69a491093409fa1370f2ef045c41106ca8bf9c46f6'
            '64a77944a28026b066c1682c7258d02289d257b24b6f173a9f7580c48beed966')
validpgpkeys=('8703B6700E7EE06D7A39B8D6EDAE37B02CEB490D') # Emil Velikov <emil.l.velikov@gmail.com>
validpgpkeys+=('946D09B5E4C9845E63075FF1D961C596A7203456') #  "Andres Gomez <tanty@igalia.com>"

prepare() {
  cd ${srcdir}/mesa-${pkgver}

  # Now mesa checks for libpthread-stubs - so remove the check
  patch -Np1 -i ../remove-libpthread-stubs.patch
  
  # glvnd support patches - from Fedora
  # https://patchwork.freedesktop.org/series/12354/, v3 & v4
  patch -Np1 -i ../0001-EGL-Implement-the-libglvnd-interface-for-EGL-v2.patch
  patch -Np1 -i ../0002-fixup-EGL-Implement-the-libglvnd-interface-for-EGL-v.patch
  # non-upstreamed ones
  patch -Np1 -i ../glvnd-fix-gl-dot-pc.patch
  patch -Np1 -i ../0001-Fix-linkage-against-shared-glapi.patch
  patch -Np1 -i ../0001-glapi-Link-with-glapi-when-built-shared.patch

  autoreconf -fiv
}

build() {
  cd ${srcdir}/mesa-${pkgver}

  [[ $CARCH == "armv7h" ]] && GALLIUM=",etnaviv,imx"

  ./configure --prefix=/usr \
    --sysconfdir=/etc \
    --with-dri-driverdir=/usr/lib/xorg/modules/dri \
    --with-gallium-drivers=freedreno,nouveau,swrast,virgl,vc4${GALLIUM} \
    --with-dri-drivers=nouveau,swrast \
    --with-egl-platforms=x11,drm,wayland \
    --disable-xvmc \
    --enable-gallium-llvm \
    --enable-llvm-shared-libs \
    --enable-shared-glapi \
    --enable-libglvnd \
    --enable-egl \
    --enable-glx \
    --enable-glx-tls \
    --enable-gles1 \
    --enable-gles2 \
    --enable-gbm \
    --enable-dri \
    --enable-gallium-osmesa \
    --enable-texture-float \
    --enable-omx \
    --enable-nine \
    --with-clang-libdir=/usr/lib
    #--with-sha1=libgcrypt \

  make

  # fake installation
  mkdir $srcdir/fakeinstall
  make DESTDIR=${srcdir}/fakeinstall install
}

package_libva-mesa-driver() {
  pkgdesc="VA-API implementation for gallium"
  depends=('libdrm' 'libx11' 'llvm-libs' 'expat' 'libelf' 'libxshmfence')

  install -m755 -d ${pkgdir}/usr/lib
  cp -rv ${srcdir}/fakeinstall/usr/lib/dri ${pkgdir}/usr/lib
   
  install -m755 -d "${pkgdir}/usr/share/licenses/libva-mesa-driver"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/libva-mesa-driver/"
}
               
package_mesa() {
  pkgdesc="an open-source implementation of the OpenGL specification"
  depends=('libdrm' 'wayland' 'libxxf86vm' 'libxdamage' 'libxshmfence' 'libelf' 
           'libomxil-bellagio' 'libtxc_dxtn' 'llvm-libs' 'libglvnd')
  optdepends=('opengl-man-pages: for the OpenGL API man pages'
              'mesa-vdpau: for accelerated video playback'
              'libva-mesa-driver: for accelerated video playback')
  provides=('ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri' 'mesa-libgl' 'opengl-driver')
  conflicts=('ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri' 'mesa-libgl')
  replaces=('ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri' 'mesa-libgl')

  install -m755 -d ${pkgdir}/etc
  cp -rv ${srcdir}/fakeinstall/etc/drirc ${pkgdir}/etc
  
  install -m755 -d ${pkgdir}/usr/share/glvnd/egl_vendor.d
  cp -rv ${srcdir}/fakeinstall/usr/share/glvnd/egl_vendor.d/50_mesa.json ${pkgdir}/usr/share/glvnd/egl_vendor.d/

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
  
  # remove vulkan headers
  rm -rf ${pkgdir}/usr/include/vulkan

  # libglvnd support
  cp -rv ${srcdir}/fakeinstall/usr/lib/libGLX_mesa.so* ${pkgdir}/usr/lib/
  cp -rv ${srcdir}/fakeinstall/usr/lib/libEGL_mesa.so* ${pkgdir}/usr/lib/
  # indirect rendering
  ln -s /usr/lib/libGLX_mesa.so.0 ${pkgdir}/usr/lib/libGLX_indirect.so.0

  install -m755 -d "${pkgdir}/usr/share/licenses/mesa"
  install -m644 "${srcdir}/LICENSE" "${pkgdir}/usr/share/licenses/mesa/"
}
