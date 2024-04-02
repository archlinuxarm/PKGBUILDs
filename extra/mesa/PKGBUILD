# Maintainer: Laurent Carlier <lordheavym@gmail.com>
# Maintainer: Felix Yan <felixonmars@archlinux.org>
# Maintainer: Jan Alexander Steffens (heftig) <heftig@archlinux.org>
# Contributor: Jan de Groot <jgc@archlinux.org>
# Contributor: Andreas Radke <andyrtr@archlinux.org>
# Contributor: Dan Johansen <strit@manjaro.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - Removed Gallium3D drivers/packages for chipsets that don't exist in our ARM devices (intel, VMware svga).
#  - added broadcom and panfrost vulkan packages
#  - enable lto for aarch64
#  ALARM: John Audia <therealgraysky@proton.me>
#  - added freedreno driver, PR#1973 and PR#1996

highmem=1

pkgbase=mesa
pkgname=(
  'vulkan-mesa-layers'
  'opencl-clover-mesa'
  'opencl-rusticl-mesa'
  'vulkan-nouveau'
  'vulkan-radeon'
  'vulkan-swrast'
  'vulkan-virtio'
  'vulkan-broadcom'
  'vulkan-panfrost'
  'vulkan-freedreno'
  'libva-mesa-driver'
  'mesa-vdpau'
  'mesa'
)
pkgver=24.0.4
pkgrel=2
epoch=1
pkgdesc="Open-source OpenGL drivers"
url="https://www.mesa3d.org/"
arch=('x86_64')
license=('MIT AND BSD-3-Clause AND SGI-B-2.0')
makedepends=(
  'clang'
  'expat'
  'gcc-libs'
  'glibc'
  'libdrm'
  'libelf'
  'libglvnd'
  'libva'
  'libvdpau'
  'libx11'
  'libxcb'
  'libxext'
  'libxfixes'
  'libxml2'
  'libxrandr'
  'libxshmfence'
  'libxxf86vm'
  'llvm'
  'llvm-libs'
  'lm_sensors'
  'rust'
  'spirv-llvm-translator'
  'spirv-tools'
  'systemd-libs'
  'vulkan-icd-loader'
  'wayland'
  'xcb-util-keysyms'
  'zlib'
  'zstd'

  # shared between mesa and lib32-mesa
  'clang'
  'cmake'
  'elfutils'
  'glslang'
  'libclc'
  'meson'
  'python-mako'
  'python-ply'
  'rust-bindgen'
  'wayland-protocols'
  'xorgproto'

  # valgrind deps
  'valgrind'

  # d3d12 deps
  'directx-headers'

  # gallium-omx deps
  'libomxil-bellagio'
)
source=(
  https://mesa.freedesktop.org/archive/mesa-${pkgver}.tar.xz{,.sig}
)
validpgpkeys=(
  '8703B6700E7EE06D7A39B8D6EDAE37B02CEB490D'  # Emil Velikov <emil.l.velikov@gmail.com>
  '946D09B5E4C9845E63075FF1D961C596A7203456'  # Andres Gomez <tanty@igalia.com>
  'E3E8F480C52ADD73B278EE78E1ECBE07D7D70895'  # Juan Antonio Suárez Romero (Igalia, S.L.) <jasuarez@igalia.com>
  'A5CC9FEC93F2F837CB044912336909B6B25FADFA'  # Juan A. Suarez Romero <jasuarez@igalia.com>
  '71C4B75620BC75708B4BDB254C95FAAB3EB073EC'  # Dylan Baker <dylan@pnwbakers.com>
  '57551DE15B968F6341C248F68D8E31AFC32428A6'  # Eric Engestrom <eric@engestrom.ch>
)

# Rust crates for NVK, used as Meson subprojects
declare -A _crates=(
   proc-macro2    1.0.70
   quote          1.0.33
   syn            2.0.39
   unicode-ident  1.0.12
)

for _crate in "${!_crates[@]}"; do
  source+=($_crate-${_crates[$_crate]}.tar.gz::https://crates.io/api/v1/crates/$_crate/${_crates[$_crate]}/download)
done

sha256sums=('90febd30a098cbcd97ff62ecc3dcf5c93d76f7fa314de944cfce81951ba745f0'
            'SKIP'
            '39278fbbf5fb4f646ce651690877f89d1c5811a3d4acb27700c1cb3cdb78fd3b'
            '3354b9ac3fae1ff6755cb6db53683adb661634f67557942dea4facebec0fee4b'
            '5267fca4496028628a95160fc423a33e8b2e6af8a5302579e322e4b520293cae'
            '23e78b90f2fcf45d3e842032ce32e3f2d1545ba6636271dcbf24fa306d87be7a')
b2sums=('6de755081f7e9dd9303af791e1a405203388787c294f8163c9d6598aa66eed1c001eeb03203c49ed8a264065458228efd849e6e59091a5963155ce8edc47c63f'
        'SKIP'
        'fff0dec06b21e391783cc136790238acb783780eaedcf14875a350e7ceb46fdc100c8b9e3f09fb7f4c2196c25d4c6b61e574c0dad762d94533b628faab68cf5c'
        '4cede03c08758ccd6bf53a0d0057d7542dfdd0c93d342e89f3b90460be85518a9fd24958d8b1da2b5a09b5ddbee8a4263982194158e171c2bba3e394d88d6dac'
        '77c4b166f1200e1ee2ab94a5014acd334c1fe4b7d72851d73768d491c56c6779a0882a304c1f30c88732a6168351f0f786b10516ae537cff993892a749175848'
        '2cff6626624d03f70f1662af45a8644c28a9f92e2dfe38999bef3ba4a4c1ce825ae598277e9cb7abd5585eebfb17b239effc8d0bbf1c6ac196499f0d288e5e01')

prepare() {
  cd mesa-$pkgver

  # Include package release in version string so Chromium invalidates
  # its GPU cache; otherwise it can cause pages to render incorrectly.
  # https://bugs.launchpad.net/ubuntu/+source/chromium-browser/+bug/2020604
  echo "$pkgver-arch$epoch.$pkgrel" >VERSION

  if [[ $CARCH == "armv7h" ]]; then
    sed -i "/c_cpp_args += '-mtls-dialect=gnu2'/d" meson.build
  fi
}

build() {
  case "${CARCH}" in
    armv7h)  GALLIUM=",etnaviv,kmsro,lima,panfrost,tegra,v3d,vc4" ;;
    aarch64) GALLIUM=",etnaviv,kmsro,lima,panfrost,svga,v3d,vc4" ;;
  esac

  local meson_options=(
    -D android-libbacktrace=disabled
    -D b_lto=$([[ $CARCH == aarch64 ]] && echo true || echo false)
    -D b_ndebug=true
    -D dri3=enabled
    -D egl=enabled
    -D gallium-drivers=r300,r600,radeonsi,freedreno,nouveau,swrast,virgl,zink,d3d12${GALLIUM}
    -D gallium-extra-hud=true
    -D gallium-nine=true
    -D gallium-omx=bellagio
    -D gallium-opencl=icd
    -D gallium-rusticl=true
    -D gallium-va=enabled
    -D gallium-vdpau=enabled
    -D gallium-xa=disabled
    -D gbm=enabled
    -D gles1=disabled
    -D gles2=enabled
    -D glvnd=true
    -D glx=dri
    -D intel-clc=disabled
    -D libunwind=disabled
    -D llvm=enabled
    -D lmsensors=enabled
    -D microsoft-clc=disabled
    -D osmesa=true
    -D platforms=x11,wayland
    -D rust_std=2021
    -D shared-glapi=enabled
    -D valgrind=enabled
    -D video-codecs=all
    -D vulkan-drivers=amd,swrast,broadcom,panfrost,virtio,freedreno,nouveau-experimental
    -D vulkan-layers=device-select,overlay
  )

  # Build only minimal debug info to reduce size
  CFLAGS+=' -g1'
  CXXFLAGS+=' -g1'

  # Inject subproject packages
  export MESON_PACKAGE_CACHE_DIR="$srcdir"

  arch-meson mesa-$pkgver build "${meson_options[@]}"
  meson configure build # Print config
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

_libdir=usr/lib

package_vulkan-mesa-layers() {
  pkgdesc="Mesa's Vulkan layers"
  depends=(
    'gcc-libs'
    'glibc'
    'libdrm'
    'libxcb'
    'wayland'

    'python'
  )
  conflicts=('vulkan-mesa-layer')
  replaces=('vulkan-mesa-layer')

  _install fakeinstall/usr/share/vulkan/explicit_layer.d
  _install fakeinstall/usr/share/vulkan/implicit_layer.d
  _install fakeinstall/$_libdir/libVkLayer_*.so
  _install fakeinstall/usr/bin/mesa-overlay-control.py

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_opencl-clover-mesa() {
  pkgdesc="Open-source OpenCL drivers - Clover variant"
  depends=(
    'clang'
    'expat'
    'gcc-libs'
    'glibc'
    'libdrm'
    'libelf'
    'llvm-libs'
    'spirv-llvm-translator'
    'spirv-tools'
    'zlib'
    'zstd'

    'libclc'
  )
  optdepends=('opencl-headers: headers necessary for OpenCL development')
  provides=('opencl-driver')
  replaces=("opencl-mesa<=23.1.4-1")
  conflicts=('opencl-mesa')

  _install fakeinstall/etc/OpenCL/vendors/mesa.icd
  _install fakeinstall/$_libdir/libMesaOpenCL*
  _install fakeinstall/$_libdir/gallium-pipe

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_opencl-rusticl-mesa() {
  pkgdesc="Open-source OpenCL drivers - RustICL variant"
  depends=(
    'clang'
    'expat'
    'gcc-libs'
    'glibc'
    'libdrm'
    'libelf'
    'llvm-libs'
    'spirv-llvm-translator'
    'spirv-tools'
    'zlib'
    'zstd'

    'libclc'
  )
  optdepends=('opencl-headers: headers necessary for OpenCL development')
  provides=('opencl-driver')
  replaces=("opencl-mesa<=23.1.4-1")
  conflicts=('opencl-mesa')

  _install fakeinstall/etc/OpenCL/vendors/rusticl.icd
  _install fakeinstall/$_libdir/libRusticlOpenCL*

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_vulkan-nouveau() {
  pkgdesc="Open-source Vulkan driver for Nvidia GPUs"
  depends=(
    'expat'
    'gcc-libs'
    'glibc'
    'libdrm'
    'libx11'
    'libxcb'
    'libxshmfence'
    'systemd-libs'
    'vulkan-icd-loader'
    'wayland'
    'xcb-util-keysyms'
    'zlib'
    'zstd'
  )
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/nouveau_*.json
  _install fakeinstall/$_libdir/libvulkan_nouveau*.so

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_vulkan-radeon() {
  pkgdesc="Open-source Vulkan driver for AMD GPUs"
  depends=(
    'expat'
    'gcc-libs'
    'glibc'
    'libdrm'
    'libelf'
    'libx11'
    'libxcb'
    'libxshmfence'
    'llvm-libs'
    'systemd-libs'
    'vulkan-icd-loader'
    'wayland'
    'xcb-util-keysyms'
    'zlib'
    'zstd'
  )
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/drirc.d/00-radv-defaults.conf
  _install fakeinstall/usr/share/vulkan/icd.d/radeon_icd*.json
  _install fakeinstall/$_libdir/libvulkan_radeon.so

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_vulkan-swrast() {
  pkgdesc="Open-source Vulkan driver for CPUs (Software Rasterizer)"
  depends=(
    'expat'
    'gcc-libs'
    'glibc'
    'libdrm'
    'libx11'
    'libxcb'
    'libxshmfence'
    'llvm-libs'
    'systemd-libs'
    'vulkan-icd-loader'
    'wayland'
    'xcb-util-keysyms'
    'zlib'
    'zstd'
  )
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  conflicts=('vulkan-mesa')
  replaces=('vulkan-mesa')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/lvp_icd*.json
  _install fakeinstall/$_libdir/libvulkan_lvp.so

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_vulkan-virtio() {
  pkgdesc="Open-source Vulkan driver for Virtio-GPU (Venus)"
  depends=(
    'expat'
    'gcc-libs'
    'glibc'
    'libdrm'
    'libx11'
    'libxcb'
    'libxshmfence'
    'systemd-libs'
    'vulkan-icd-loader'
    'wayland'
    'xcb-util-keysyms'
    'zlib'
    'zstd'
  )
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/virtio_icd*.json
  _install fakeinstall/$_libdir/libvulkan_virtio.so

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_vulkan-broadcom() {
  pkgdesc="Broadcom's Vulkan mesa driver"
  depends=(
    'wayland'
    'libx11'
    'libxshmfence'
    'libdrm'
  )
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/broadcom_icd*.json
  _install fakeinstall/$_libdir/libvulkan_broadcom.so

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_vulkan-panfrost() {
  pkgdesc="Panfrost Vulkan mesa driver"
  depends=(
    'wayland'
    'libx11'
    'libxshmfence'
    'libdrm'
  )
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/panfrost_icd*.json
  _install fakeinstall/$_libdir/libvulkan_panfrost.so

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_vulkan-freedreno() {
  pkgdesc="Freedreno Vulkan mesa driver"
  depends=('wayland' 'libx11' 'libxshmfence' 'libdrm')
  optdepends=('vulkan-mesa-layers: additional vulkan layers')
  provides=('vulkan-driver')

  _install fakeinstall/usr/share/vulkan/icd.d/freedreno_icd*.json
  _install fakeinstall/usr/lib/libvulkan_freedreno.so

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_libva-mesa-driver() {
  pkgdesc="Open-source VA-API drivers"
  depends=(
    'expat'
    'gcc-libs'
    'glibc'
    'libdrm'
    'libelf'
    'libx11'
    'libxcb'
    'libxshmfence'
    'llvm-libs'
    'zlib'
    'zstd'
  )
  provides=('libva-driver')

  _install fakeinstall/$_libdir/dri/*_drv_video.so

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_mesa-vdpau() {
  pkgdesc="Open-source VDPAU drivers"
  depends=(
    'expat'
    'gcc-libs'
    'glibc'
    'libdrm'
    'libelf'
    'libx11'
    'libxcb'
    'libxshmfence'
    'llvm-libs'
    'zlib'
    'zstd'
  )
  provides=('vdpau-driver')

  _install fakeinstall/$_libdir/vdpau

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

package_mesa() {
  depends=(
    'expat'
    'gcc-libs'
    'glibc'
    'libdrm'
    'libelf'
    'libglvnd'
    'libx11'
    'libxcb'
    'libxext'
    'libxfixes'
    'libxshmfence'
    'libxxf86vm'
    'llvm-libs'
    'lm_sensors'
    'wayland'
    'zlib'
    'zstd'

    'libomxil-bellagio'
  )
  optdepends=(
    'opengl-man-pages: for the OpenGL API man pages'
  )
  provides=(
    'mesa-libgl'
    'opengl-driver'
  )
  conflicts=('mesa-libgl')
  replaces=('mesa-libgl')

  _install fakeinstall/usr/share/drirc.d/00-mesa-defaults.conf
  _install fakeinstall/usr/share/glvnd/egl_vendor.d/50_mesa.json

  # ati-dri, nouveau-dri, intel-dri, svga-dri, swrast, swr
  _install fakeinstall/$_libdir/dri/*_dri.so

  _install fakeinstall/$_libdir/bellagio
  _install fakeinstall/$_libdir/d3d
  _install fakeinstall/$_libdir/lib{gbm,glapi}.so*
  _install fakeinstall/$_libdir/libOSMesa.so*

  _install fakeinstall/usr/include
  rm -f fakeinstall/$_libdir/pkgconfig/{egl,gl}.pc
  _install fakeinstall/$_libdir/pkgconfig

  # libglvnd support
  _install fakeinstall/$_libdir/libGLX_mesa.so*
  _install fakeinstall/$_libdir/libEGL_mesa.so*

  # indirect rendering
  ln -sr "$pkgdir"/$_libdir/libGLX_{mesa,indirect}.so.0

  # make sure there are no files left to install
  find fakeinstall -depth -print0 | xargs -0 rmdir

  install -Dm644 mesa-$pkgver/docs/license.rst -t "$pkgdir/usr/share/licenses/$pkgname"
}

# vim:set sw=2 sts=-1 et:
