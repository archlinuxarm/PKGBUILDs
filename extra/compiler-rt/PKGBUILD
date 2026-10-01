# Maintainer: Evangelos Foutras <foutrelis@archlinux.org>
# Contributor: Jan Alexander Steffens (heftig) <heftig@archlinux.org>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - disable libfuzzer on ARMv7

pkgname=compiler-rt
pkgver=23.1.1
pkgrel=1
pkgdesc="Compiler runtime libraries for clang"
arch=('x86_64')
url="https://compiler-rt.llvm.org/"
license=('Apache-2.0 WITH LLVM-exception')
depends=('glibc' 'libgcc' 'libstdc++')
makedepends=('llvm' 'cmake' 'ninja' 'python')
# Build 32-bit compiler-rt libraries on x86_64 (FS#41911)
makedepends_x86_64=('lib32-gcc-libs')
options=('staticlibs' '!lto')
_source_base=https://github.com/llvm/llvm-project/releases/download/llvmorg-$pkgver
source=($_source_base/llvm-project-$pkgver.src.tar.xz{,.sig}
        0001-libcxx-chrono-fix-years-months-period.patch)
sha256sums=('ebe9be46fe8756d58c5b198ffad0fa2a766257add81a4dc52179bfacc7888ee6'
            'SKIP'
            'e6b4c72058966ab2af0c255cad22172ff89c04a4433de5b22b16923a94936b2a')

validpgpkeys=('474E22316ABF4785A88C6E8EA2C794A986419D8A'  # Tom Stellard <tstellar@redhat.com>
              'D574BD5D1D0E98895E3BF90044F2485E45D59042'  # Tobias Hieta <tobias@hieta.se>
              'FFB3368980F3E6BB5737145A316C56D064CACBA5'  # Douglas Yung <douglas.yung@sony.com>
              '71046D1E9C6656BDD61171873E83BABF4A4F9E85'  # Cullen Rhodes <cullen.rhodes@arm.com>
)

prepare() {
  cd llvm-project-$pkgver.src
  # https://github.com/llvm/llvm-project/issues/223223
  patch -Np1 -i "$srcdir"/0001-libcxx-chrono-fix-years-months-period.patch

  cd compiler-rt
  mkdir build
}

build() {
  cd llvm-project-$pkgver.src/compiler-rt/build

  local cmake_args=(
    -G Ninja
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=/usr
    -DCMAKE_SKIP_RPATH=ON
    -DCOMPILER_RT_INSTALL_PATH=/usr/lib/clang/${pkgver%%.*}
  )
  [[ $CARCH == "armv7h" ]] && cmake_args+=(-DCOMPILER_RT_BUILD_LIBFUZZER=OFF)
  cmake .. "${cmake_args[@]}"
  ninja
}

package() {
  cd llvm-project-$pkgver.src/compiler-rt/build

  DESTDIR="$pkgdir" ninja install
  install -Dm644 "$srcdir/llvm-project-$pkgver.src/compiler-rt/LICENSE.TXT" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

# vim:set ts=2 sw=2 et:
