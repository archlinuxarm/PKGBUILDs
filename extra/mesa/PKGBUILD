# $Id$
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, radeon, VMware svga).
#  - upstream patch for libatomic

pkgbase=mesa
pkgname=('libva-mesa-driver' 'mesa-vdpau' 'mesa')
pkgdesc="An open-source implementation of the OpenGL specification"
pkgver=18.0.3
pkgrel=4
arch=('x86_64')
makedepends=('python2-mako' 'libxml2' 'libx11' 'glproto' 'libdrm' 'dri2proto' 'dri3proto' 'presentproto' 
             'libxshmfence' 'libxxf86vm' 'libxdamage' 'libvdpau' 'libva' 'wayland' 'wayland-protocols'
             'elfutils' 'llvm' 'libomxil-bellagio' 'clang' 'libglvnd' 'lm_sensors' 'meson')
url="https://www.mesa3d.org/"
license=('custom')
source=(https://mesa.freedesktop.org/archive/mesa-${pkgver}.tar.xz{,.sig}
        LICENSE
        0001-glvnd-fix-gl.pc.patch
        0002-meson-Add-library-versions-to-swr-drivers.patch
        0003-meson-Version-libMesaOpenCL-like-autotools-does.patch
        0004-loader_dri3-Variant-2-Wait-for-pending-swaps-to-comp.patch
        "atomic.patch::https://cgit.freedesktop.org/mesa/mesa/patch/?id=498faea103aa7966b435f21d8ff5e36172389b1e")
sha512sums=('decd050bab049d17bcde3f832d4da0ffdb80f147c99377a162739bbe72fd6fd32b51e56e6fc66895b8c30fc19a1815bae164b21aa557816c3998ad18c1ffca2d'
            'SKIP'
            'f9f0d0ccf166fe6cb684478b6f1e1ab1f2850431c06aa041738563eb1808a004e52cdec823c103c9e180f03ffc083e95974d291353f0220fe52ae6d4897fecc7'
            '2f40198eff47664c831c56e8a63f60a4d1b815cf697e6bdb0be39e6d9c5df043857f6264b7cd2ccf46c07626186c565144e80f4214b5f7936ef7024c47201437'
            'c3f3baf8a5f480ce64b321c031e31c0d5819732ca34647ac545d0fd7fafa40ad4dcf1e1ec8d574754e0a44bf0cdc462ed8709c8d9b58a17e01c6ba5b4c5e91c6'
            'a2062f8a5259aabed1aa20df6a8510f0f3e914cb6bba72751249b3295285596bb7615063a7a7b7870f9f4489d0e6b774f0bced2bdde49a1aa9df6a44976462d1'
            '572901a1e9cacfacfc8c4cc3cd077a626d4aeda8c8a58f6085bae827cba8a2d4d99af1dafbb5a9296b6ebf3120e2b05a084fe1c96093074befe62597319384a1'
            '75cd21bccc84a6b6b0de39c6d209c8bee0e5143b486433184ca078e8bc6797d30746be3ce5f7a89eea9bc3c7e2d68880412511fd6b9946252c7c7638523c6caa')
validpgpkeys=('8703B6700E7EE06D7A39B8D6EDAE37B02CEB490D'  # Emil Velikov <emil.l.velikov@gmail.com>
              '946D09B5E4C9845E63075FF1D961C596A7203456'  # Andres Gomez <tanty@igalia.com>
              'E3E8F480C52ADD73B278EE78E1ECBE07D7D70895') # Juan Antonio Suárez Romero (Igalia, S.L.) <jasuarez@igalia.com>"
 
prepare() {
  cd mesa-${pkgver}

  # glvnd support patches - from Fedora
  # non-upstreamed ones
  patch -Np1 -i ../0001-glvnd-fix-gl.pc.patch

  # Upstreamed meson fixes
  patch -Np1 -i ../0002-meson-Add-library-versions-to-swr-drivers.patch
  patch -Np1 -i ../0003-meson-Version-libMesaOpenCL-like-autotools-does.patch

  # experimental patch, should fix FS#58549
  # variant 2 patch should fix FS#58605
  # see https://bugs.freedesktop.org/show_bug.cgi?id=106351
  # and https://patchwork.freedesktop.org/series/42687/
  patch -Np1 -i ../0004-loader_dri3-Variant-2-Wait-for-pending-swaps-to-comp.patch

  # disk cache: Link with -latomic if necessary
  patch -Np1 -i ../atomic.patch
}

build() {
  [[ $CARCH == "armv7h" ]] && GALLIUM=",etnaviv,imx"
  [[ $CARCH == "armv6h" || $CARCH == "armv7h" || $CARCH == "aarch64" ]] && GALLIUM+=",vc4"

  arch-meson mesa-$pkgver build \
    -D b_lto=false \
    -D b_ndebug=true \
    -D platforms=x11,wayland,drm,surfaceless \
    -D dri-drivers=nouveau \
    -D gallium-drivers=freedreno,nouveau,swrast,virgl${GALLIUM} \
    -D vulkan-drivers= \
    -D dri3=true \
    -D egl=true \
    -D gallium-extra-hud=true \
    -D gallium-nine=true \
    -D gallium-omx=true \
    -D gallium-va=true \
    -D gallium-vdpau=true \
    -D gallium-xa=false \
    -D gallium-xvmc=false \
    -D gbm=true \
    -D gles1=true \
    -D gles2=true \
    -D glvnd=true \
    -D glx=dri \
    -D libunwind=false \
    -D llvm=true \
    -D lmsensors=true \
    -D osmesa=gallium \
    -D shared-glapi=true \
    -D texture-float=true \
    -D valgrind=false

  # Print config
  meson configure build

  ninja -C build

  # fake installation to be seperated into packages
  # outside of fakeroot but mesa doesn't need to chown/mod
  DESTDIR="${srcdir}/fakeinstall" ninja -C build install
}

_install() {
  local src f dir
  for src; do
    f="${src#fakeinstall/}"
    dir="${pkgdir}/${f%/*}"
    install -m755 -d "${dir}"
    mv -v "${src}" "${dir}/"
  done
}

package_libva-mesa-driver() {
  pkgdesc="VA-API implementation for gallium"
  depends=('libdrm' 'libx11' 'llvm-libs' 'expat' 'libelf' 'libxshmfence')

  _install fakeinstall/usr/lib/dri/*_drv_video.so
   
  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_mesa-vdpau() {
  pkgdesc="Mesa VDPAU drivers"
  depends=('libdrm' 'libx11' 'llvm-libs' 'expat' 'libelf' 'libxshmfence')

  _install fakeinstall/usr/lib/vdpau
   
  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_mesa() {
  depends=('libdrm' 'wayland' 'libxxf86vm' 'libxdamage' 'libxshmfence' 'libelf' 
           'libomxil-bellagio' 'llvm-libs' 'lm_sensors' 'libglvnd')
  optdepends=('opengl-man-pages: for the OpenGL API man pages'
              'mesa-vdpau: for accelerated video playback'
              'libva-mesa-driver: for accelerated video playback')
  provides=('ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri' 'mesa-libgl' 'opengl-driver')
  conflicts=('ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri' 'mesa-libgl')
  replaces=('ati-dri' 'intel-dri' 'nouveau-dri' 'svga-dri' 'mesa-dri' 'mesa-libgl')
  backup=('etc/drirc')

  _install fakeinstall/etc/drirc
  _install fakeinstall/usr/share/glvnd/egl_vendor.d/50_mesa.json

  # ati-dri, nouveau-dri, intel-dri, svga-dri, swrast
  _install fakeinstall/usr/lib/dri/*_dri.so
   
  _install fakeinstall/usr/lib/bellagio
  _install fakeinstall/usr/lib/d3d
  _install fakeinstall/usr/lib/lib{gbm,glapi}.so*
  _install fakeinstall/usr/lib/libOSMesa.so*

  # in libglvnd
  rm -v fakeinstall/usr/lib/libGLESv{1_CM,2}.so*
  
  # in vulkan-headers
  rm -rfv fakeinstall/usr/include/vulkan

  # in wayland
  rm -v fakeinstall/usr/lib/libwayland-egl.so*
  rm -v fakeinstall/usr/lib/pkgconfig/wayland-egl.pc

  _install fakeinstall/usr/include
  _install fakeinstall/usr/lib/pkgconfig

  # libglvnd support
  _install fakeinstall/usr/lib/libGLX_mesa.so*
  _install fakeinstall/usr/lib/libEGL_mesa.so*

  # indirect rendering
  ln -s /usr/lib/libGLX_mesa.so.0 "${pkgdir}/usr/lib/libGLX_indirect.so.0"

  # make sure there are no files left to install
  find fakeinstall -depth -print0 | xargs -0 rmdir

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}
