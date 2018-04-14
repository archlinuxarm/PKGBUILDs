# $Id$
# Maintainer: Jan de Groot <jgc@archlinux.org>
# Maintainer: Andreas Radke <andyrtr@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed DRI and Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, radeon, VMware svga).
#  - upstream patch for libatomic

pkgbase=mesa
pkgname=('libva-mesa-driver' 'mesa-vdpau' 'mesa')
pkgdesc="An open-source implementation of the OpenGL specification"
pkgver=18.0.0
pkgrel=4
arch=('x86_64')
makedepends=('python2-mako' 'libxml2' 'libx11' 'glproto' 'libdrm' 'dri2proto' 'dri3proto' 'presentproto' 
             'libxshmfence' 'libxxf86vm' 'libxdamage' 'libvdpau' 'libva' 'wayland' 'wayland-protocols'
             'elfutils' 'llvm' 'libomxil-bellagio' 'clang' 'libglvnd' 'lm_sensors' 'meson')
url="https://www.mesa3d.org/"
license=('custom')
source=(https://mesa.freedesktop.org/archive/mesa-${pkgver}.tar.xz{,.sig}
        "meson_get_version.py::https://cgit.freedesktop.org/mesa/mesa/plain/bin/meson_get_version.py?h=mesa-18.0.0"
        LICENSE
        0001-glvnd-fix-gl-dot-pc.patch
        fix-install.diff
        fix-versions.diff
        ndebug.diff
        "atomic.patch::https://cgit.freedesktop.org/mesa/mesa/patch/?id=498faea103aa7966b435f21d8ff5e36172389b1e")
sha512sums=('1494bb09357896a2505b3dcfec772268e28c765804f21e144948a314f1d79d99ff9f21062ef5860eb5a5a568b305a9c954585924a7ac8890fe1ebd8df3bcc396'
            'SKIP'
            'cdc608d7b7de9e6eb6f1b2b4faef4864ac213d379b9dedc7c06e71726c2a1b88a0035d6ec50812a14ba4639e100158c6dff3a1d9456ab36c0a52988287c0d4bd'
            'f9f0d0ccf166fe6cb684478b6f1e1ab1f2850431c06aa041738563eb1808a004e52cdec823c103c9e180f03ffc083e95974d291353f0220fe52ae6d4897fecc7'
            '75849eca72ca9d01c648d5ea4f6371f1b8737ca35b14be179e14c73cc51dca0739c333343cdc228a6d464135f4791bcdc21734e2debecd29d57023c8c088b028'
            'da32ac3b025282c584bfc962723151b6e11887e59e35086c616a987cb3a471051d60f2b303a91f37106ebb75621cbd9b3f560036f5beb88518cfe9d75c45ee03'
            '836f06af6feaa79a16bedd7a136d637b7f16e53d98b8b267554d98b5ff8c3fa45955b9e3ce0b8366d86860194e9147baf0257614fff85a471e2b90bbb3b1f5ab'
            '9ca216f8a84e767e4df2d02135004b173cb7905368573402cb8329e8e53101c5b519bf9b74cebfeab9ccd550b6c62c0fc88ec9a9e631023e011bb5634522c034'
            '75cd21bccc84a6b6b0de39c6d209c8bee0e5143b486433184ca078e8bc6797d30746be3ce5f7a89eea9bc3c7e2d68880412511fd6b9946252c7c7638523c6caa')
validpgpkeys=('8703B6700E7EE06D7A39B8D6EDAE37B02CEB490D'  # Emil Velikov <emil.l.velikov@gmail.com>
              '946D09B5E4C9845E63075FF1D961C596A7203456'  # Andres Gomez <tanty@igalia.com>
              'E3E8F480C52ADD73B278EE78E1ECBE07D7D70895') # Juan Antonio Suárez Romero (Igalia, S.L.) <jasuarez@igalia.com>"
 
prepare() {
  cd mesa-${pkgver}

  # glvnd support patches - from Fedora
  # non-upstreamed ones
  patch -Np1 -i ../0001-glvnd-fix-gl-dot-pc.patch

  # fix symlinks created for vdpau drivers to be relative
  # this is upstreamable
  patch -Np1 -i ../fix-install.diff

  # fix library versioning
  # this is upstreamable
  patch -Np1 -i ../fix-versions.diff

  # define NDEBUG for non-debug builds, like configure does
  # this is upstreamable
  patch -Np1 -i ../ndebug.diff

  # file missing from tarball
  cp ../meson_get_version.py bin/

  # disk cache: Link with -latomic if necessary
  patch -Np1 -i ../atomic.patch
}

build() {
  [[ $CARCH == "armv7h" ]] && GALLIUM=",etnaviv,imx"
  [[ $CARCH == "armv6h" || $CARCH == "armv7h" || $CARCH == "aarch64" ]] && GALLIUM+=",vc4"

  arch-meson mesa-$pkgver build \
    -D b_lto=false \
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
  _install fakeinstall/usr/lib/libwayland*.so*

  # in libglvnd
  rm -v fakeinstall/usr/lib/libGLESv{1_CM,2}.so*
  
  # in vulkan-headers
  rm -rfv fakeinstall/usr/include/vulkan

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
