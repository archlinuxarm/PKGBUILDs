# Maintainer: Laurent Carlier <lordheavym@gmail.com>
# Maintainer: Felix Yan <felixonmars@archlinux.org>
# Contributor: Jan de Groot <jgc@archlinux.org>
# Contributor: Andreas Radke <andyrtr@archlinux.org>
# Contributor: Dan Johansen <strit@manjaro.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, VMware svga).
#  - added broadcom and panfrost vulkan packages
#  - enable lto for aarch64

highmem=1

pkgbase=mesa
pkgname=('vulkan-mesa-layers' 'opencl-mesa' 'vulkan-radeon' 'vulkan-swrast' 'vulkan-virtio' 'vulkan-broadcom' 'vulkan-panfrost' 'libva-mesa-driver' 'mesa-vdpau' 'mesa')
pkgdesc="An open-source implementation of the OpenGL specification"
pkgver=22.3.4
pkgrel=1
arch=('x86_64')
makedepends=('python-mako' 'libxml2' 'libx11' 'xorgproto' 'libdrm' 'libxshmfence' 'libxxf86vm'
             'libxdamage' 'libvdpau' 'libva' 'wayland' 'wayland-protocols' 'zstd' 'elfutils' 'llvm'
             'libomxil-bellagio' 'libclc' 'clang' 'libglvnd' 'libunwind' 'lm_sensors' 'libxrandr'
             'systemd' 'valgrind' 'glslang' 'vulkan-icd-loader' 'directx-headers' 'cmake' 'meson')
makedepends+=('rust' 'rust-bindgen' 'spirv-tools' 'spirv-llvm-translator') # rusticl dependencies
url="https://www.mesa3d.org/"
license=('custom')
options=('!lto')
source=(https://mesa.freedesktop.org/archive/mesa-${pkgver}.tar.xz{,.sig}
        0001-anv-force-MEDIA_INTERFACE_DESCRIPTOR_LOAD-reemit-aft.patch
        0002-iris-Retry-DRM_IOCTL_I915_GEM_EXECBUFFER2-on-ENOMEM.patch
        0003-Revert-iris-Avoid-abort-if-kernel-can-t-allocate-mem.patch
        LICENSE)
sha512sums=('6af340153244d3e95d0e155a45d6db134335654d62590797ae0ef6ba44c2ccfe91ebf95f70ff82c67cee108ac35536767b1f6848d6d1129f52eb9e8414ee321d'
            'SKIP'
            '64f55c8fbb53c6cdf4a2898d8fc633e37a9f5b71284c222030914dedbddd0a712969378f3e7dc336c5e6349d5dd3d2e117e4ff5d83a9336847a20a1bd0dc09bb'
            'f0dfde13bd7c08ece286c04c67729ec86e60dd61730a947c7bef19605303f413e3a125eec6081a40ef8fc15b7571aef2b8e42fe0bece9c53de4e5774783e48b4'
            '6f5ff5185815c878999a4aa2f6b8251a68403c6755de9f58f89332251e04658729af7d9570761b2dc1367c5e33e2cce22b7e7199dcf6a64717e704034db6f8c0'
            'f9f0d0ccf166fe6cb684478b6f1e1ab1f2850431c06aa041738563eb1808a004e52cdec823c103c9e180f03ffc083e95974d291353f0220fe52ae6d4897fecc7')
validpgpkeys=('8703B6700E7EE06D7A39B8D6EDAE37B02CEB490D'  # Emil Velikov <emil.l.velikov@gmail.com>
              '946D09B5E4C9845E63075FF1D961C596A7203456'  # Andres Gomez <tanty@igalia.com>
              'E3E8F480C52ADD73B278EE78E1ECBE07D7D70895'  # Juan Antonio Suárez Romero (Igalia, S.L.) <jasuarez@igalia.com>
              'A5CC9FEC93F2F837CB044912336909B6B25FADFA'  # Juan A. Suarez Romero <jasuarez@igalia.com>
              '71C4B75620BC75708B4BDB254C95FAAB3EB073EC'  # Dylan Baker <dylan@pnwbakers.com>
              '57551DE15B968F6341C248F68D8E31AFC32428A6') # Eric Engestrom <eric@engestrom.ch>

prepare() {
  cd mesa-$pkgver

  # https://gitlab.freedesktop.org/mesa/mesa/-/issues/7111
  # https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/17247
  # https://github.com/HansKristian-Work/vkd3d-proton/issues/1200
  patch -Np1 -i ../0001-anv-force-MEDIA_INTERFACE_DESCRIPTOR_LOAD-reemit-aft.patch

  # https://gitlab.freedesktop.org/drm/intel/-/issues/6851
  # https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/20449
  patch -Np1 -i ../0002-iris-Retry-DRM_IOCTL_I915_GEM_EXECBUFFER2-on-ENOMEM.patch
  patch -Np1 -i ../0003-Revert-iris-Avoid-abort-if-kernel-can-t-allocate-mem.patch
}

build() {
  case "${CARCH}" in
    armv7h)  GALLIUM=",etnaviv,kmsro,lima,panfrost,tegra,v3d,vc4" ;;
    aarch64) GALLIUM=",asahi,etnaviv,kmsro,lima,panfrost,svga,v3d,vc4" ;;
  esac

  # Build only minimal debug info to reduce size
  CFLAGS+=' -g1'
  CXXFLAGS+=' -g1'

  arch-meson mesa-$pkgver build \
    -D b_ndebug=true \
    -D b_lto=$([[ $CARCH == aarch64 ]] && echo true || echo false)  \
    -D platforms=x11,wayland \
    -D gallium-drivers=r300,r600,radeonsi,freedreno,nouveau,swrast,virgl,zink,d3d12${GALLIUM} \
    -D vulkan-drivers=amd,swrast,broadcom,panfrost,virtio-experimental \
    -D vulkan-layers=device-select,overlay \
    -D dri3=enabled \
    -D egl=enabled \
    -D gallium-extra-hud=true \
    -D gallium-nine=true \
    -D gallium-rusticl=true \
    -D rust_std=2021 \
    -D gallium-omx=bellagio \
    -D gallium-opencl=icd \
    -D gallium-va=enabled \
    -D gallium-vdpau=enabled \
    -D gallium-xa=disabled \
    -D gbm=enabled \
    -D gles1=disabled \
    -D gles2=enabled \
    -D glvnd=true \
    -D glx=dri \
    -D libunwind=disabled \
    -D llvm=enabled \
    -D lmsensors=enabled \
    -D osmesa=true \
    -D shared-glapi=enabled \
    -D microsoft-clc=disabled \
    -D video-codecs=vc1dec,h264dec,h264enc,h265dec,h265enc \
    -D valgrind=enabled

  # Print config
  meson configure build

  ninja -C build
  meson compile -C build

  # fake installation to be seperated into packages
  # outside of fakeroot but mesa doesn't need to chown/mod
  DESTDIR="${srcdir}/fakeinstall" meson install -C build
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

package_vulkan-mesa-layers() {
  pkgdesc="Mesa's Vulkan layers"
  depends=('libdrm' 'libxcb' 'wayland' 'python')
  conflicts=('vulkan-mesa-layer')
  replaces=('vulkan-mesa-layer')

  _install fakeinstall/usr/share/vulkan/explicit_layer.d
  _install fakeinstall/usr/share/vulkan/implicit_layer.d
  _install fakeinstall/usr/lib/libVkLayer_*.so
  _install fakeinstall/usr/bin/mesa-overlay-control.py

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_opencl-mesa() {
  pkgdesc="OpenCL support with clover and rusticl for mesa drivers"
  depends=('libdrm' 'libclc' 'clang' 'expat' 'spirv-llvm-translator')
  optdepends=('opencl-headers: headers necessary for OpenCL development')
  provides=('opencl-driver')

  _install fakeinstall/etc/OpenCL
  _install fakeinstall/usr/lib/lib*OpenCL*
  _install fakeinstall/usr/lib/gallium-pipe

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_vulkan-radeon() {
  pkgdesc="Radeon's Vulkan mesa driver"
  depends=('wayland' 'libx11' 'libxshmfence' 'libelf' 'libdrm' 'llvm-libs' 'systemd-libs')
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/drirc.d/00-radv-defaults.conf
  _install fakeinstall/usr/share/vulkan/icd.d/radeon_icd*.json
  _install fakeinstall/usr/lib/libvulkan_radeon.so

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_vulkan-swrast() {
  pkgdesc="Vulkan software rasteriser driver"
  depends=('wayland' 'libx11' 'libxshmfence' 'libdrm' 'zstd' 'llvm-libs' 'systemd-libs' 'libunwind')
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  conflicts=('vulkan-mesa')
  replaces=('vulkan-mesa')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/lvp_icd*.json
  _install fakeinstall/usr/lib/libvulkan_lvp.so

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_vulkan-virtio() {
  pkgdesc="Venus Vulkan mesa driver for Virtual Machines"
  depends=('wayland' 'libx11' 'libxshmfence' 'libdrm' 'zstd' 'systemd-libs')
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/virtio_icd*.json
  _install fakeinstall/usr/lib/libvulkan_virtio.so

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_vulkan-broadcom() {
  pkgdesc="Broadcom's Vulkan mesa driver"
  depends=('wayland' 'libx11' 'libxshmfence' 'libdrm')
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/broadcom_icd*.json
  _install fakeinstall/usr/lib/libvulkan_broadcom.so

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_vulkan-panfrost() {
  pkgdesc="Panfrost Vulkan mesa driver"
  depends=('wayland' 'libx11' 'libxshmfence' 'libdrm')
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/panfrost_icd*.json
  _install fakeinstall/usr/lib/libvulkan_panfrost.so

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_libva-mesa-driver() {
  pkgdesc="VA-API drivers"
  depends=('libdrm' 'libx11' 'llvm-libs' 'expat' 'libelf' 'libxshmfence')
  depends+=('libexpat.so')
  provides=('libva-driver')

  _install fakeinstall/usr/lib/dri/*_drv_video.so

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_mesa-vdpau() {
  pkgdesc="VDPAU drivers"
  depends=('libdrm' 'libx11' 'llvm-libs' 'expat' 'libelf' 'libxshmfence')
  depends+=('libexpat.so')
  provides=('vdpau-driver')

  _install fakeinstall/usr/lib/vdpau

  install -m644 -Dt "${pkgdir}/usr/share/licenses/${pkgname}" LICENSE
}

package_mesa() {
  depends=('libdrm' 'wayland' 'libxxf86vm' 'libxdamage' 'libxshmfence' 'libelf'
           'libomxil-bellagio' 'libunwind' 'llvm-libs' 'lm_sensors' 'libglvnd'
           'zstd' 'vulkan-icd-loader')
  depends+=('libsensors.so' 'libexpat.so' 'libvulkan.so')
  optdepends=('opengl-man-pages: for the OpenGL API man pages'
              'mesa-vdpau: for accelerated video playback'
              'libva-mesa-driver: for accelerated video playback')
  provides=('mesa-libgl' 'opengl-driver')
  conflicts=('mesa-libgl')
  replaces=('mesa-libgl')

  _install fakeinstall/usr/share/drirc.d/00-mesa-defaults.conf
  _install fakeinstall/usr/share/glvnd/egl_vendor.d/50_mesa.json

  # ati-dri, nouveau-dri, intel-dri, svga-dri, swrast, swr
  _install fakeinstall/usr/lib/dri/*_dri.so

  _install fakeinstall/usr/lib/bellagio
  _install fakeinstall/usr/lib/d3d
  _install fakeinstall/usr/lib/lib{gbm,glapi}.so*
  _install fakeinstall/usr/lib/libOSMesa.so*

  _install fakeinstall/usr/include
  rm -f fakeinstall/usr/lib/pkgconfig/{egl,gl}.pc
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
