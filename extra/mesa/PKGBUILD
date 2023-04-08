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
pkgver=23.0.2
pkgrel=2
arch=('x86_64')
makedepends=('python-mako' 'libxml2' 'libx11' 'xorgproto' 'libdrm' 'libxshmfence' 'libxxf86vm'
             'libxdamage' 'libvdpau' 'libva' 'wayland' 'wayland-protocols' 'zstd' 'elfutils' 'llvm'
             'libomxil-bellagio' 'libclc' 'clang' 'libglvnd' 'libunwind' 'lm_sensors' 'libxrandr'
             'systemd' 'valgrind' 'glslang' 'vulkan-icd-loader' 'directx-headers' 'cmake' 'meson')
makedepends+=('rust' 'rust-bindgen' 'spirv-tools' 'spirv-llvm-translator') # rusticl dependencies
url="https://www.mesa3d.org/"
license=('custom')
source=(https://mesa.freedesktop.org/archive/mesa-${pkgver}.tar.xz{,.sig}
        0001-intel-fs-fix-scheduling-of-HALT-instructions.patch
        0002-egl-wayland-Fix-destruction-of-event-queue-with-prox.patch
        0003-vulkan-wsi-wayland-Fix-destruction-of-event-queue-wi.patch
        LICENSE)
sha256sums=('1b7d3399fc6f16f030361f925d33ebc7600cbf98094582f54775b6a1180529e7'
            'SKIP'
            'dc6790b5be0e80c23e74ae18ca1a2b40f57f4211cc7b645bf22b63af3b790e40'
            'c25493de3e5930702acf833d182aeca0895d6a9d9e830dca15c42d130e25c41c'
            'db2be7ae0540d65e77449eda1af66200e27af382183fdcd0c87f99db3520b80a'
            '7052ba73bb07ea78873a2431ee4e828f4e72bda7d176d07f770fa48373dec537')
b2sums=('5a90fcd8b7096dde1e6c82b9bb5b00622cc1cf35e4308419441d3489d66ed322843db89f2f1162c8c30f6bcddbce867447f83425a29145bd42a742b773ec1258'
        'SKIP'
        '37d1d070c45c85bce8abe3524a3f8d9ac9ed6326a3eec653cd89fffce3630b08eb9b96b11aeb495488230449c99f9b508f73a15e53265d2b159286b0e2dda7cc'
        'ae891637318fdbb8dd58285098af7bea709fb032969a5671eb225a4a471bf9422fac2a6cb0fd188aad96ea5a03eb043f646f5d371dd655a100046adb1c91bd7d'
        'a7312e0931904e659b3d33fcb37b13bcadab943c6040dd2b58ea191db350b50c1ba588e334b7e59b513bd6155797e29dc1bd1a6a35a278b3824d06534f2c9d17'
        '1ecf007b82260710a7bf5048f47dd5d600c168824c02c595af654632326536a6527fbe0738670ee7b921dd85a70425108e0f471ba85a8e1ca47d294ad74b4adb')
validpgpkeys=('8703B6700E7EE06D7A39B8D6EDAE37B02CEB490D'  # Emil Velikov <emil.l.velikov@gmail.com>
              '946D09B5E4C9845E63075FF1D961C596A7203456'  # Andres Gomez <tanty@igalia.com>
              'E3E8F480C52ADD73B278EE78E1ECBE07D7D70895'  # Juan Antonio Suárez Romero (Igalia, S.L.) <jasuarez@igalia.com>
              'A5CC9FEC93F2F837CB044912336909B6B25FADFA'  # Juan A. Suarez Romero <jasuarez@igalia.com>
              '71C4B75620BC75708B4BDB254C95FAAB3EB073EC'  # Dylan Baker <dylan@pnwbakers.com>
              '57551DE15B968F6341C248F68D8E31AFC32428A6') # Eric Engestrom <eric@engestrom.ch>

prepare() {
  cd mesa-$pkgver

  # https://gitlab.freedesktop.org/mesa/mesa/-/issues/7110
  # https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/20765
  patch -Np1 -i ../0001-intel-fs-fix-scheduling-of-HALT-instructions.patch

  # https://bugs.archlinux.org/task/78137
  # https://bugzilla.mozilla.org/show_bug.cgi?id=1826583
  # https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/21646
  patch -Np1 -i ../0002-egl-wayland-Fix-destruction-of-event-queue-with-prox.patch

  # https://bugs.archlinux.org/task/78142
  # https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/21647
  patch -Np1 -i ../0003-vulkan-wsi-wayland-Fix-destruction-of-event-queue-wi.patch
}

build() {
  case "${CARCH}" in
    armv7h)  GALLIUM=",etnaviv,kmsro,lima,panfrost,tegra,v3d,vc4" ;;
    aarch64) GALLIUM=",etnaviv,kmsro,lima,panfrost,svga,v3d,vc4" ;;
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
  depends+=('libsensors.so' 'libexpat.so')
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
